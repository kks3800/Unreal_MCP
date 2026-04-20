#include "UnrealMCPBridge.h"
#include "MCPCore.h"
#include "MCPServerRunnable.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/RunnableThread.h"
#include "Interfaces/IPv4/IPv4Address.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonWriter.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "JsonObjectConverter.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Async/Async.h"
// Add Blueprint related includes
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Factories/BlueprintFactory.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
// UE5.5 correct includes
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "UObject/Field.h"
#include "UObject/FieldPath.h"
// Blueprint Graph specific includes
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_CallFunction.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "GameFramework/InputSettings.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
// Include our new command handler classes
#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPBlueprintCommands.h"
#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPProjectCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "Commands/UnrealMCPUMGCommands.h"
#include "Commands/UnrealMCPMaterialCommands.h"
#include "Commands/UnrealMCPMetaSoundCommands.h"
#include "Commands/UnrealMCPNiagaraCommands.h"
#include "Commands/UnrealMCPAssetCommands.h"
#include "Commands/UnrealMCPBlueprintInspectCommands.h"
#include "Commands/UnrealMCPBlueprintSearchCommands.h"
#include "Commands/UnrealMCPBlueprintGraphCommands.h"
#include "Commands/UnrealMCPBlueprintMultigraphCommands.h"
#include "Commands/UnrealMCPBTAssetCommands.h"
#include "Commands/UnrealMCPBTNodeCommands.h"
#include "Commands/UnrealMCPBTStructureCommands.h"
#include "Commands/UnrealMCPBTRuntimeCommands.h"
#include "Commands/UnrealMCPEQSCommands.h"
#include "Commands/UnrealMCPPCGCommands.h"

// Live-edit visualization
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "IMaterialEditor.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialFunctionInterface.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialGraph/MaterialGraph.h"
#include "MaterialGraph/MaterialGraphNode.h"
#include "Misc/Paths.h"
#include "UObject/SoftObjectPath.h"

// Default settings
#define MCP_SERVER_HOST "127.0.0.1"
#define MCP_SERVER_PORT 55557

namespace BalloMCPLiveView
{
    /** Resolve an asset package path from either the command result or the params. */
    static FString ExtractAssetPath(const TSharedPtr<FJsonObject>& Params, const TSharedPtr<FJsonObject>& Result)
    {
        static const TCHAR* ResultKeys[] = {
            TEXT("material_path"), TEXT("function_path"), TEXT("instance_path"),
            TEXT("blueprint_path"), TEXT("asset_path"), TEXT("package_path"), TEXT("path")
        };
        if (Result.IsValid())
        {
            for (const TCHAR* Key : ResultKeys)
            {
                FString Value;
                if (Result->TryGetStringField(Key, Value) && Value.StartsWith(TEXT("/")))
                {
                    return Value;
                }
            }
        }
        if (!Params.IsValid())
        {
            return FString();
        }
        static const TCHAR* NameKeys[] = {
            TEXT("material_name"), TEXT("function_name"), TEXT("instance_name"),
            TEXT("blueprint_name"), TEXT("asset_name"), TEXT("name")
        };
        FString Name;
        for (const TCHAR* Key : NameKeys)
        {
            if (Params->TryGetStringField(Key, Name))
            {
                break;
            }
        }
        if (Name.IsEmpty())
        {
            return FString();
        }
        if (Name.StartsWith(TEXT("/")))
        {
            return Name;
        }
        FString BasePath;
        Params->TryGetStringField(TEXT("path"), BasePath);
        if (BasePath.IsEmpty())
        {
            return FString();
        }
        return BasePath / Name;
    }

    /** Try to load the UObject referenced by a package path. Handles both `/Game/Foo/Bar` and `/Game/Foo/Bar.Bar`. */
    static UObject* TryLoadAsset(const FString& PackagePath)
    {
        if (PackagePath.IsEmpty())
        {
            return nullptr;
        }
        FString Normalized = PackagePath;
        if (!Normalized.Contains(TEXT(".")))
        {
            Normalized = PackagePath + TEXT(".") + FPaths::GetCleanFilename(PackagePath);
        }
        return FSoftObjectPath(Normalized).TryLoad();
    }

    /** Show a Slate toast in the upper-right corner announcing the MCP command. */
    static void ShowNotification(const FString& CommandType, const FString& AssetPath, bool bSuccess, const FString& ErrorMessage)
    {
        FNotificationInfo Info(FText::FromString(FString::Printf(TEXT("[MCP] %s"), *CommandType)));
        if (!AssetPath.IsEmpty())
        {
            Info.SubText = FText::FromString(AssetPath);
        }
        else if (!bSuccess && !ErrorMessage.IsEmpty())
        {
            Info.SubText = FText::FromString(ErrorMessage);
        }
        Info.ExpireDuration = bSuccess ? 2.5f : 5.0f;
        Info.bFireAndForget = true;
        Info.bUseSuccessFailIcons = true;
        Info.bUseThrobber = false;
        Info.bUseLargeFont = false;
        TSharedPtr<SNotificationItem> Item = FSlateNotificationManager::Get().AddNotification(Info);
        if (Item.IsValid())
        {
            Item->SetCompletionState(bSuccess ? SNotificationItem::CS_Success : SNotificationItem::CS_Fail);
        }
    }

    /** Open the asset editor for the resolved asset and refresh the graph view for materials/functions. */
    static void OpenAndRefreshEditor(const FString& AssetPath)
    {
        if (AssetPath.IsEmpty() || !GEditor)
        {
            return;
        }
        UObject* Asset = TryLoadAsset(AssetPath);
        if (!Asset)
        {
            return;
        }
        UAssetEditorSubsystem* Subsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
        if (!Subsystem)
        {
            return;
        }

        if (UMaterial* OriginalMaterial = Cast<UMaterial>(Asset))
        {
            // First-time open: the editor's InitMaterialEditor duplicates OriginalMaterial
            // into a UPreviewMaterial. From this point on, MCP's LoadMaterial routes edits
            // to the preview (see UnrealMCPMaterialCommands.cpp::LoadMaterial), so the open
            // editor displays the live changes — no close/reopen needed, no flicker.
            if (!Subsystem->FindEditorForAsset(OriginalMaterial, /*bFocusIfOpen=*/false))
            {
                Subsystem->OpenEditorForAsset(OriginalMaterial);
            }

            // Sync the graph: MCP's CreateMaterialExpressionEx adds to the preview's
            // expression collection but never creates the matching UMaterialGraphNode.
            // Walk the preview's expressions, add missing graph nodes, then notify the
            // SGraphEditor widget to repaint.
            if (IAssetEditorInstance* Instance = Subsystem->FindEditorForAsset(OriginalMaterial, false))
            {
                IMaterialEditor* MatEditor = static_cast<IMaterialEditor*>(Instance);
                if (UMaterial* Preview = Cast<UMaterial>(MatEditor->GetMaterialInterface()))
                {
                    if (UMaterialGraph* Graph = Preview->MaterialGraph)
                    {
                        TSet<UMaterialExpression*> AlreadyOnGraph;
                        AlreadyOnGraph.Reserve(Graph->Nodes.Num());
                        for (UEdGraphNode* GN : Graph->Nodes)
                        {
                            if (UMaterialGraphNode* MN = Cast<UMaterialGraphNode>(GN))
                            {
                                if (MN->MaterialExpression)
                                {
                                    AlreadyOnGraph.Add(MN->MaterialExpression);
                                }
                            }
                        }
                        if (UMaterialEditorOnlyData* EdOnly = Preview->GetEditorOnlyData())
                        {
                            const int32 Num = EdOnly->ExpressionCollection.Expressions.Num();
                            for (int32 Idx = 0; Idx < Num; ++Idx)
                            {
                                UMaterialExpression* Expr = EdOnly->ExpressionCollection.Expressions[Idx].Get();
                                if (Expr && !AlreadyOnGraph.Contains(Expr))
                                {
                                    Graph->AddExpression(Expr, /*bUserInvoked=*/false);
                                }
                            }
                        }
                        Graph->LinkGraphNodesFromMaterial();
                        Graph->NotifyGraphChanged();
                    }
                }
            }
        }
        else if (UMaterialFunction* Function = Cast<UMaterialFunction>(Asset))
        {
            Subsystem->OpenEditorForAsset(Function);
            UMaterialEditingLibrary::UpdateMaterialFunction(Function, nullptr);
        }
        else if (UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Asset))
        {
            Subsystem->OpenEditorForAsset(Instance);
            UMaterialEditingLibrary::UpdateMaterialInstance(Instance);
        }
        else
        {
            Subsystem->OpenEditorForAsset(Asset);
        }
    }
}

UUnrealMCPBridge::UUnrealMCPBridge()
{
    EditorCommands = MakeShared<FUnrealMCPEditorCommands>();
    BlueprintCommands = MakeShared<FUnrealMCPBlueprintCommands>();
    BlueprintNodeCommands = MakeShared<FUnrealMCPBlueprintNodeCommands>();
    ProjectCommands = MakeShared<FUnrealMCPProjectCommands>();
    UMGCommands = MakeShared<FUnrealMCPUMGCommands>();
    MaterialCommands = MakeShared<FUnrealMCPMaterialCommands>();
    MetaSoundCommands = MakeShared<FUnrealMCPMetaSoundCommands>();
    NiagaraCommands = MakeShared<FUnrealMCPNiagaraCommands>();
    AssetCommands = MakeShared<FUnrealMCPAssetCommands>();
    BlueprintInspectCommands = MakeShared<FUnrealMCPBlueprintInspectCommands>();
    BlueprintSearchCommands = MakeShared<FUnrealMCPBlueprintSearchCommands>();
    BlueprintGraphCommands = MakeShared<FUnrealMCPBlueprintGraphCommands>();
    BlueprintMultigraphCommands = MakeShared<FUnrealMCPBlueprintMultigraphCommands>();
    BTAssetCommands = MakeShared<FUnrealMCPBTAssetCommands>();
    BTNodeCommands = MakeShared<FUnrealMCPBTNodeCommands>();
    BTStructureCommands = MakeShared<FUnrealMCPBTStructureCommands>();
    BTRuntimeCommands = MakeShared<FUnrealMCPBTRuntimeCommands>();
    EQSCommands = MakeShared<FUnrealMCPEQSCommands>();
    PCGCommands = MakeShared<FUnrealMCPPCGCommands>();
    InputCommands = MakeShared<FUnrealMCPInputCommands>();
}

UUnrealMCPBridge::~UUnrealMCPBridge()
{
    EditorCommands.Reset();
    BlueprintCommands.Reset();
    BlueprintNodeCommands.Reset();
    ProjectCommands.Reset();
    UMGCommands.Reset();
    MaterialCommands.Reset();
    MetaSoundCommands.Reset();
    NiagaraCommands.Reset();
    AssetCommands.Reset();
    BlueprintInspectCommands.Reset();
    BlueprintSearchCommands.Reset();
    BlueprintGraphCommands.Reset();
    BlueprintMultigraphCommands.Reset();
    BTAssetCommands.Reset();
    BTNodeCommands.Reset();
    BTStructureCommands.Reset();
    BTRuntimeCommands.Reset();
    EQSCommands.Reset();
    PCGCommands.Reset();
    InputCommands.Reset();
}

// Initialize subsystem
void UUnrealMCPBridge::Initialize(FSubsystemCollectionBase& Collection)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Initializing"));
    
    bIsRunning = false;
    ListenerSocket = nullptr;
    ConnectionSocket = nullptr;
    ServerThread = nullptr;
    Port = MCP_SERVER_PORT;
    FIPv4Address::Parse(MCP_SERVER_HOST, ServerAddress);

    // Register all commands with the registry
    RegisterAllCommands();

    // Start the server automatically
    StartServer();
}

// Clean up resources when subsystem is destroyed
void UUnrealMCPBridge::Deinitialize()
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Shutting down"));
    StopServer();
}

// Start the MCP server
void UUnrealMCPBridge::StartServer()
{
    if (bIsRunning)
    {
        UE_LOG(LogTemp, Warning, TEXT("UnrealMCPBridge: Server is already running"));
        return;
    }

    // Create socket subsystem
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (!SocketSubsystem)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to get socket subsystem"));
        return;
    }

    // Create listener socket
    TSharedPtr<FSocket> NewListenerSocket = MakeShareable(SocketSubsystem->CreateSocket(NAME_Stream, TEXT("UnrealMCPListener"), false));
    if (!NewListenerSocket.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create listener socket"));
        return;
    }

    // Allow address reuse for quick restarts
    NewListenerSocket->SetReuseAddr(true);
    NewListenerSocket->SetNonBlocking(true);

    // Bind to address
    FIPv4Endpoint Endpoint(ServerAddress, Port);
    if (!NewListenerSocket->Bind(*Endpoint.ToInternetAddr()))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to bind listener socket to %s:%d"), *ServerAddress.ToString(), Port);
        return;
    }

    // Start listening
    if (!NewListenerSocket->Listen(5))
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to start listening"));
        return;
    }

    ListenerSocket = NewListenerSocket;
    bIsRunning = true;
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server started on %s:%d"), *ServerAddress.ToString(), Port);

    // Start server thread
    ServerThread = FRunnableThread::Create(
        new FMCPServerRunnable(this, ListenerSocket),
        TEXT("UnrealMCPServerThread"),
        0, TPri_Normal
    );

    if (!ServerThread)
    {
        UE_LOG(LogTemp, Error, TEXT("UnrealMCPBridge: Failed to create server thread"));
        StopServer();
        return;
    }
}

// Stop the MCP server
void UUnrealMCPBridge::StopServer()
{
    if (!bIsRunning)
    {
        return;
    }

    bIsRunning = false;

    // Clean up thread
    if (ServerThread)
    {
        ServerThread->Kill(true);
        delete ServerThread;
        ServerThread = nullptr;
    }

    // Close sockets
    if (ConnectionSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ConnectionSocket.Get());
        ConnectionSocket.Reset();
    }

    if (ListenerSocket.IsValid())
    {
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket.Get());
        ListenerSocket.Reset();
    }

    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Server stopped"));
}

// Register all commands from all handlers with the command registry
void UUnrealMCPBridge::RegisterAllCommands()
{
    FMCPCommandRegistry& Registry = FMCPCommandRegistry::Get();

    // Built-in ping command
    Registry.RegisterCommand(TEXT("ping"),
        [](const TSharedPtr<FJsonObject>& Params) {
            TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
            Result->SetStringField(TEXT("message"), TEXT("pong"));
            return Result;
        });

    // Register commands from each handler
    EditorCommands->RegisterCommands(Registry);
    BlueprintCommands->RegisterCommands(Registry);
    BlueprintNodeCommands->RegisterCommands(Registry);
    BlueprintGraphCommands->RegisterCommands(Registry);
    BlueprintMultigraphCommands->RegisterCommands(Registry);
    BlueprintInspectCommands->RegisterCommands(Registry);
    BlueprintSearchCommands->RegisterCommands(Registry);
    ProjectCommands->RegisterCommands(Registry);
    UMGCommands->RegisterCommands(Registry);
    MaterialCommands->RegisterCommands(Registry);
    MetaSoundCommands->RegisterCommands(Registry);
    NiagaraCommands->RegisterCommands(Registry);
    AssetCommands->RegisterCommands(Registry);
    BTAssetCommands->RegisterCommands(Registry);
    BTNodeCommands->RegisterCommands(Registry);
    BTStructureCommands->RegisterCommands(Registry);
    BTRuntimeCommands->RegisterCommands(Registry);
    EQSCommands->RegisterCommands(Registry);
    PCGCommands->RegisterCommands(Registry);
    InputCommands->RegisterCommands(Registry);

    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Registered %d commands"), Registry.GetRegisteredCommands().Num());
}

// Execute a command received from a client
FString UUnrealMCPBridge::ExecuteCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    UE_LOG(LogTemp, Display, TEXT("UnrealMCPBridge: Executing command: %s"), *CommandType);
    
    // Create a promise to wait for the result
    TPromise<FString> Promise;
    TFuture<FString> Future = Promise.GetFuture();
    
    // Queue execution on Game Thread
    AsyncTask(ENamedThreads::GameThread, [this, CommandType, Params, Promise = MoveTemp(Promise)]() mutable
    {
        TSharedPtr<FJsonObject> ResponseJson = MakeShareable(new FJsonObject);
        TSharedPtr<FJsonObject> ResultJson;
        bool bCommandSucceeded = true;
        FString CommandErrorMessage;

        try
        {
            // Dispatch via command registry (TMap lookup)
            FMCPCommandRegistry& Registry = FMCPCommandRegistry::Get();
            if (Registry.HasCommand(CommandType))
            {
                ResultJson = Registry.ExecuteCommand(CommandType, Params);
            }
            else
            {
                bCommandSucceeded = false;
                CommandErrorMessage = FString::Printf(TEXT("Unknown command: %s"), *CommandType);
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), CommandErrorMessage);

                BalloMCPLiveView::ShowNotification(CommandType, FString(), false, CommandErrorMessage);

                FString ResultString;
                TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
                FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
                Promise.SetValue(ResultString);
                return;
            }

            // Check if the result contains an error
            if (ResultJson->HasField(TEXT("success")))
            {
                bCommandSucceeded = ResultJson->GetBoolField(TEXT("success"));
                if (!bCommandSucceeded && ResultJson->HasField(TEXT("error")))
                {
                    CommandErrorMessage = ResultJson->GetStringField(TEXT("error"));
                }
            }

            if (bCommandSucceeded)
            {
                // Set success status and include the result
                ResponseJson->SetStringField(TEXT("status"), TEXT("success"));
                ResponseJson->SetObjectField(TEXT("result"), ResultJson);
            }
            else
            {
                // Set error status but ALWAYS include the full result for batch detail
                ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
                ResponseJson->SetStringField(TEXT("error"), CommandErrorMessage);
                ResponseJson->SetObjectField(TEXT("result"), ResultJson);
            }
        }
        catch (const std::exception& e)
        {
            bCommandSucceeded = false;
            CommandErrorMessage = UTF8_TO_TCHAR(e.what());
            ResponseJson->SetStringField(TEXT("status"), TEXT("error"));
            ResponseJson->SetStringField(TEXT("error"), CommandErrorMessage);
        }

        // Live-edit visualization: toast + auto-open the affected asset editor
        const FString AssetPath = BalloMCPLiveView::ExtractAssetPath(Params, ResultJson);
        BalloMCPLiveView::ShowNotification(CommandType, AssetPath, bCommandSucceeded, CommandErrorMessage);
        if (bCommandSucceeded)
        {
            BalloMCPLiveView::OpenAndRefreshEditor(AssetPath);
        }

        FString ResultString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ResultString);
        FJsonSerializer::Serialize(ResponseJson.ToSharedRef(), Writer);
        Promise.SetValue(ResultString);
    });
    
    return Future.Get();
}