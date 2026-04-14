#include "Commands/UnrealMCPCommonUtils.h"
#include "GameFramework/Actor.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "UObject/UObjectIterator.h"
#include "Engine/Selection.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintVariableNodeSpawner.h"
#include "BlueprintNodeBinder.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"

// JSON Utilities
TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateErrorResponse(const FString& Message)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), false);
    ResponseObject->SetStringField(TEXT("error"), Message);
    return ResponseObject;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
    TSharedPtr<FJsonObject> ResponseObject = MakeShared<FJsonObject>();
    ResponseObject->SetBoolField(TEXT("success"), true);
    
    if (Data.IsValid())
    {
        ResponseObject->SetObjectField(TEXT("data"), Data);
    }
    
    return ResponseObject;
}

void FUnrealMCPCommonUtils::GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((int32)Value->AsNumber());
        }
    }
}

void FUnrealMCPCommonUtils::GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray)
{
    OutArray.Reset();
    
    if (!JsonObject->HasField(FieldName))
    {
        return;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray))
    {
        for (const TSharedPtr<FJsonValue>& Value : *JsonArray)
        {
            OutArray.Add((float)Value->AsNumber());
        }
    }
}

FVector2D FUnrealMCPCommonUtils::GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector2D Result(0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 2)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
    }
    
    return Result;
}

FVector FUnrealMCPCommonUtils::GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FVector Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.X = (float)(*JsonArray)[0]->AsNumber();
        Result.Y = (float)(*JsonArray)[1]->AsNumber();
        Result.Z = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

FRotator FUnrealMCPCommonUtils::GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName)
{
    FRotator Result(0.0f, 0.0f, 0.0f);
    
    if (!JsonObject->HasField(FieldName))
    {
        return Result;
    }
    
    const TArray<TSharedPtr<FJsonValue>>* JsonArray;
    if (JsonObject->TryGetArrayField(FieldName, JsonArray) && JsonArray->Num() >= 3)
    {
        Result.Pitch = (float)(*JsonArray)[0]->AsNumber();
        Result.Yaw = (float)(*JsonArray)[1]->AsNumber();
        Result.Roll = (float)(*JsonArray)[2]->AsNumber();
    }
    
    return Result;
}

// Blueprint Utilities
UBlueprint* FUnrealMCPCommonUtils::FindBlueprint(const FString& BlueprintName)
{
    return FindBlueprintByName(BlueprintName);
}

UBlueprint* FUnrealMCPCommonUtils::FindBlueprintByName(const FString& BlueprintName)
{
    // 1. Try as full asset path first (e.g., "/Game/Blueprints/BP_MyActor")
    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *BlueprintName);
    if (Blueprint)
    {
        return Blueprint;
    }

    // 2. Try with .BlueprintName suffix (e.g., "/Game/Blueprints/BP_MyActor.BP_MyActor")
    FString PathWithSuffix = BlueprintName + TEXT(".") + FPaths::GetBaseFilename(BlueprintName);
    Blueprint = LoadObject<UBlueprint>(nullptr, *PathWithSuffix);
    if (Blueprint)
    {
        return Blueprint;
    }

    // 3. Search AssetRegistry by name
    IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
    TArray<FAssetData> FoundAssets;

    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;
    AssetRegistry->GetAssets(Filter, FoundAssets);

    for (const FAssetData& AssetData : FoundAssets)
    {
        if (AssetData.AssetName.ToString() == BlueprintName)
        {
            return Cast<UBlueprint>(AssetData.GetAsset());
        }
    }

    // 4. Legacy fallback: try /Game/Blueprints/ path
    FString LegacyPath = TEXT("/Game/Blueprints/") + BlueprintName;
    return LoadObject<UBlueprint>(nullptr, *LegacyPath);
}

UEdGraph* FUnrealMCPCommonUtils::FindOrCreateEventGraph(UBlueprint* Blueprint)
{
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Try to find the event graph
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName().Contains(TEXT("EventGraph")))
        {
            return Graph;
        }
    }
    
    // Create a new event graph if none exists
    UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(TEXT("EventGraph")), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
    FBlueprintEditorUtils::AddUbergraphPage(Blueprint, NewGraph);
    return NewGraph;
}

// Blueprint node utilities
UK2Node_Event* FUnrealMCPCommonUtils::CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UBlueprint* Blueprint = FBlueprintEditorUtils::FindBlueprintForGraph(Graph);
    if (!Blueprint)
    {
        return nullptr;
    }
    
    // Check for existing event node with this exact name
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Using existing event node with name %s (ID: %s)"), 
                *EventName, *EventNode->NodeGuid.ToString());
            return EventNode;
        }
    }

    // No existing node found, create a new one
    UK2Node_Event* EventNode = nullptr;
    
    // Find the function to create the event
    UClass* BlueprintClass = Blueprint->GeneratedClass;
    UFunction* EventFunction = BlueprintClass->FindFunctionByName(FName(*EventName));
    
    if (EventFunction)
    {
        EventNode = NewObject<UK2Node_Event>(Graph);
        EventNode->EventReference.SetExternalMember(FName(*EventName), BlueprintClass);
        EventNode->NodePosX = Position.X;
        EventNode->NodePosY = Position.Y;
        Graph->AddNode(EventNode, true);
        EventNode->PostPlacedNewNode();
        EventNode->AllocateDefaultPins();
        UE_LOG(LogTemp, Display, TEXT("Created new event node with name %s (ID: %s)"), 
            *EventName, *EventNode->NodeGuid.ToString());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to find function for event name: %s"), *EventName);
    }
    
    return EventNode;
}

UK2Node_CallFunction* FUnrealMCPCommonUtils::CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position)
{
    if (!Graph || !Function)
    {
        return nullptr;
    }
    
    UK2Node_CallFunction* FunctionNode = NewObject<UK2Node_CallFunction>(Graph);
    FunctionNode->SetFromFunction(Function);
    FunctionNode->NodePosX = Position.X;
    FunctionNode->NodePosY = Position.Y;
    Graph->AddNode(FunctionNode, true);
    FunctionNode->CreateNewGuid();
    FunctionNode->PostPlacedNewNode();
    FunctionNode->AllocateDefaultPins();
    
    return FunctionNode;
}

// ---------------------------------------------------------------------------
// Variable node creation — uses the engine's UBlueprintVariableNodeSpawner,
// the same path the UMG/Kismet editor takes when a variable is dragged from
// the My Blueprint panel onto the graph. The spawner handles:
//   - Self member vars
//   - Parent-class inherited vars
//   - Component references
//   - bSelfContext computation for IsChildOf checks
//
// Previous implementation used UEdGraph::AddNode + VariableReference.SetSelfMember
// directly, which failed for variables that had been authored but not yet
// baked into Blueprint->GeneratedClass (i.e. every freshly-added variable
// before a full compile). We now look through all four scopes and fall back
// to a skeleton regeneration when necessary.
// ---------------------------------------------------------------------------

namespace
{
    /** Search for a variable's FProperty across every scope the engine checks. */
    FProperty* FindVariablePropertyInAnyScope(UBlueprint* Blueprint, FName VarName)
    {
        if (!Blueprint)
        {
            return nullptr;
        }

        // 1. Skeleton class — updated on every blueprint save, even before full compile.
        if (Blueprint->SkeletonGeneratedClass)
        {
            if (FProperty* P = FindFProperty<FProperty>(Blueprint->SkeletonGeneratedClass, VarName))
            {
                return P;
            }
        }

        // 2. Fully compiled generated class.
        if (Blueprint->GeneratedClass)
        {
            if (FProperty* P = FindFProperty<FProperty>(Blueprint->GeneratedClass, VarName))
            {
                return P;
            }
        }

        // 3. Parent class hierarchy (inherited vars, components, engine properties).
        if (UClass* Parent = Blueprint->ParentClass)
        {
            if (FProperty* P = FindFProperty<FProperty>(Parent, VarName))
            {
                return P;
            }
        }

        return nullptr;
    }

    /** Resolve a variable to its FProperty, forcing a skeleton regeneration if
     *  the variable was just authored but hasn't yet been reflected in any class. */
    FProperty* ResolveVariableProperty(UBlueprint* Blueprint, FName VarName)
    {
        if (FProperty* P = FindVariablePropertyInAnyScope(Blueprint, VarName))
        {
            return P;
        }

        // Variable may be in Blueprint->NewVariables but not yet in the skeleton.
        // Conform + refresh triggers a skeleton regeneration without a full compile.
        if (FBlueprintEditorUtils::FindNewVariableIndex(Blueprint, VarName) != INDEX_NONE)
        {
            FBlueprintEditorUtils::RefreshVariables(Blueprint);
            return FindVariablePropertyInAnyScope(Blueprint, VarName);
        }

        return nullptr;
    }

    /** Core spawn helper shared by Get/Set — invokes the engine's variable spawner. */
    UK2Node_Variable* SpawnVariableNode(
        UEdGraph* Graph,
        UBlueprint* Blueprint,
        const FString& VariableName,
        TSubclassOf<UK2Node_Variable> NodeClass,
        const FVector2D& Position)
    {
        if (!Graph || !Blueprint || !NodeClass)
        {
            return nullptr;
        }

        const FName VarName(*VariableName);
        FProperty* Property = ResolveVariableProperty(Blueprint, VarName);
        if (!Property)
        {
            return nullptr;
        }

        // OwnerClass selection: use the property's actual owner when available
        // (handles inherited vars); otherwise the blueprint's skeleton class.
        UClass* OwnerClass = Property->GetOwnerClass();
        if (!OwnerClass)
        {
            OwnerClass = Blueprint->SkeletonGeneratedClass;
        }

        UBlueprintVariableNodeSpawner* Spawner = UBlueprintVariableNodeSpawner::CreateFromMemberOrParam(
            NodeClass,
            Property,
            /*VarContext=*/ nullptr,   // only used for local variables; nullptr for member
            OwnerClass);

        if (!Spawner)
        {
            return nullptr;
        }

        IBlueprintNodeBinder::FBindingSet Bindings;
        UEdGraphNode* NewNode = Spawner->Invoke(Graph, Bindings, Position);
        return Cast<UK2Node_Variable>(NewNode);
    }
}

UK2Node_VariableGet* FUnrealMCPCommonUtils::CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    return Cast<UK2Node_VariableGet>(SpawnVariableNode(
        Graph, Blueprint, VariableName, UK2Node_VariableGet::StaticClass(), Position));
}

UK2Node_VariableSet* FUnrealMCPCommonUtils::CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position)
{
    return Cast<UK2Node_VariableSet>(SpawnVariableNode(
        Graph, Blueprint, VariableName, UK2Node_VariableSet::StaticClass(), Position));
}

UK2Node_InputAction* FUnrealMCPCommonUtils::CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_InputAction* InputActionNode = NewObject<UK2Node_InputAction>(Graph);
    InputActionNode->InputActionName = FName(*ActionName);
    InputActionNode->NodePosX = Position.X;
    InputActionNode->NodePosY = Position.Y;
    Graph->AddNode(InputActionNode, true);
    InputActionNode->CreateNewGuid();
    InputActionNode->PostPlacedNewNode();
    InputActionNode->AllocateDefaultPins();
    
    return InputActionNode;
}

UK2Node_Self* FUnrealMCPCommonUtils::CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position)
{
    if (!Graph)
    {
        return nullptr;
    }
    
    UK2Node_Self* SelfNode = NewObject<UK2Node_Self>(Graph);
    SelfNode->NodePosX = Position.X;
    SelfNode->NodePosY = Position.Y;
    Graph->AddNode(SelfNode, true);
    SelfNode->CreateNewGuid();
    SelfNode->PostPlacedNewNode();
    SelfNode->AllocateDefaultPins();
    
    return SelfNode;
}

bool FUnrealMCPCommonUtils::ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                           UEdGraphNode* TargetNode, const FString& TargetPinName)
{
    if (!Graph || !SourceNode || !TargetNode)
    {
        return false;
    }
    
    UEdGraphPin* SourcePin = FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FindPin(TargetNode, TargetPinName, EGPD_Input);
    
    if (SourcePin && TargetPin)
    {
        SourcePin->MakeLinkTo(TargetPin);
        return true;
    }
    
    return false;
}

UEdGraphPin* FUnrealMCPCommonUtils::FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction)
{
    if (!Node)
    {
        return nullptr;
    }
    
    // Log all pins for debugging
    UE_LOG(LogTemp, Display, TEXT("FindPin: Looking for pin '%s' (Direction: %d) in node '%s'"), 
           *PinName, (int32)Direction, *Node->GetName());
    
    for (UEdGraphPin* Pin : Node->Pins)
    {
        UE_LOG(LogTemp, Display, TEXT("  - Available pin: '%s', Direction: %d, Category: %s"), 
               *Pin->PinName.ToString(), (int32)Pin->Direction, *Pin->PinType.PinCategory.ToString());
    }
    
    // First try exact match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString() == PinName && (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found exact matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If no exact match and we're looking for a component reference, try case-insensitive match
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->PinName.ToString().Equals(PinName, ESearchCase::IgnoreCase) && 
            (Direction == EGPD_MAX || Pin->Direction == Direction))
        {
            UE_LOG(LogTemp, Display, TEXT("  - Found case-insensitive matching pin: '%s'"), *Pin->PinName.ToString());
            return Pin;
        }
    }
    
    // If we're looking for a component output and didn't find it by name, try to find the first data output pin
    if (Direction == EGPD_Output && Cast<UK2Node_VariableGet>(Node) != nullptr)
    {
        for (UEdGraphPin* Pin : Node->Pins)
        {
            if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
            {
                UE_LOG(LogTemp, Display, TEXT("  - Found fallback data output pin: '%s'"), *Pin->PinName.ToString());
                return Pin;
            }
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("  - No matching pin found for '%s'"), *PinName);
    return nullptr;
}

// Actor utilities
TSharedPtr<FJsonValue> FUnrealMCPCommonUtils::ActorToJson(AActor* Actor)
{
    if (!Actor)
    {
        return MakeShared<FJsonValueNull>();
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);
    
    return MakeShared<FJsonValueObject>(ActorObject);
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::ActorToJsonObject(AActor* Actor, bool bDetailed)
{
    if (!Actor)
    {
        return nullptr;
    }
    
    TSharedPtr<FJsonObject> ActorObject = MakeShared<FJsonObject>();
    ActorObject->SetStringField(TEXT("name"), Actor->GetName());
    ActorObject->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
    
    FVector Location = Actor->GetActorLocation();
    TArray<TSharedPtr<FJsonValue>> LocationArray;
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.X));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Y));
    LocationArray.Add(MakeShared<FJsonValueNumber>(Location.Z));
    ActorObject->SetArrayField(TEXT("location"), LocationArray);
    
    FRotator Rotation = Actor->GetActorRotation();
    TArray<TSharedPtr<FJsonValue>> RotationArray;
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Pitch));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Yaw));
    RotationArray.Add(MakeShared<FJsonValueNumber>(Rotation.Roll));
    ActorObject->SetArrayField(TEXT("rotation"), RotationArray);
    
    FVector Scale = Actor->GetActorScale3D();
    TArray<TSharedPtr<FJsonValue>> ScaleArray;
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.X));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Y));
    ScaleArray.Add(MakeShared<FJsonValueNumber>(Scale.Z));
    ActorObject->SetArrayField(TEXT("scale"), ScaleArray);

    // Outliner folder path
    ActorObject->SetStringField(TEXT("folder_path"), Actor->GetFolderPath().ToString());

    return ActorObject;
}

UK2Node_Event* FUnrealMCPCommonUtils::FindExistingEventNode(UEdGraph* Graph, const FString& EventName)
{
    if (!Graph)
    {
        return nullptr;
    }

    // Look for existing event nodes
    for (UEdGraphNode* Node : Graph->Nodes)
    {
        UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
        if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
        {
            UE_LOG(LogTemp, Display, TEXT("Found existing event node with name: %s"), *EventName);
            return EventNode;
        }
    }

    return nullptr;
}

// Blueprint graph utilities
UEdGraph* FUnrealMCPCommonUtils::FindGraphByName(UBlueprint* Blueprint, const FString& GraphName)
{
    if (!Blueprint)
    {
        return nullptr;
    }

    // Search UbergraphPages (EventGraph, etc.)
    for (UEdGraph* Graph : Blueprint->UbergraphPages)
    {
        if (Graph->GetName() == GraphName || Graph->GetName().Contains(GraphName))
        {
            return Graph;
        }
    }

    // Search FunctionGraphs
    for (UEdGraph* Graph : Blueprint->FunctionGraphs)
    {
        if (Graph->GetName() == GraphName)
        {
            return Graph;
        }
    }

    // Search MacroGraphs
    for (UEdGraph* Graph : Blueprint->MacroGraphs)
    {
        if (Graph->GetName() == GraphName)
        {
            return Graph;
        }
    }

    return nullptr;
}

UEdGraphNode* FUnrealMCPCommonUtils::FindNodeByGuid(UEdGraph* Graph, const FString& GuidString)
{
    if (!Graph)
    {
        return nullptr;
    }

    for (UEdGraphNode* Node : Graph->Nodes)
    {
        if (Node->NodeGuid.ToString() == GuidString)
        {
            return Node;
        }
    }
    return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::PinToJson(UEdGraphPin* Pin, bool bIncludeConnections)
{
    if (!Pin)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
    PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
    PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
    PinObj->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());

    if (!Pin->PinType.PinSubCategory.IsNone())
    {
        PinObj->SetStringField(TEXT("sub_category"), Pin->PinType.PinSubCategory.ToString());
    }

    if (Pin->PinType.PinSubCategoryObject.IsValid())
    {
        PinObj->SetStringField(TEXT("sub_category_object"), Pin->PinType.PinSubCategoryObject->GetName());
    }

    // Container type
    if (Pin->PinType.ContainerType != EPinContainerType::None)
    {
        FString ContainerStr;
        switch (Pin->PinType.ContainerType)
        {
            case EPinContainerType::Array: ContainerStr = TEXT("Array"); break;
            case EPinContainerType::Set: ContainerStr = TEXT("Set"); break;
            case EPinContainerType::Map: ContainerStr = TEXT("Map"); break;
            default: ContainerStr = TEXT("None"); break;
        }
        PinObj->SetStringField(TEXT("container_type"), ContainerStr);
    }

    PinObj->SetBoolField(TEXT("is_reference"), Pin->PinType.bIsReference);
    PinObj->SetBoolField(TEXT("is_const"), Pin->PinType.bIsConst);
    PinObj->SetBoolField(TEXT("hidden"), Pin->bHidden);

    // Default value
    if (!Pin->DefaultValue.IsEmpty())
    {
        PinObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
    }
    if (Pin->DefaultObject)
    {
        PinObj->SetStringField(TEXT("default_object"), Pin->DefaultObject->GetPathName());
    }
    if (!Pin->DefaultTextValue.IsEmpty())
    {
        PinObj->SetStringField(TEXT("default_text_value"), Pin->DefaultTextValue.ToString());
    }

    // Connections
    if (bIncludeConnections && Pin->LinkedTo.Num() > 0)
    {
        TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
        for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
        {
            TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
            ConnObj->SetStringField(TEXT("node_guid"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
            ConnObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
            ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
        }
        PinObj->SetArrayField(TEXT("connections"), ConnectionsArray);
    }

    PinObj->SetBoolField(TEXT("connected"), Pin->LinkedTo.Num() > 0);

    return PinObj;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::NodeToJson(UEdGraphNode* Node, bool bIncludePins)
{
    if (!Node)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
    NodeObj->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
    NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
    NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
    NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
    NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);
    NodeObj->SetStringField(TEXT("comment"), Node->NodeComment);

    if (bIncludePins)
    {
        TArray<TSharedPtr<FJsonValue>> PinsArray;
        for (UEdGraphPin* Pin : Node->Pins)
        {
            TSharedPtr<FJsonObject> PinJson = PinToJson(Pin, true);
            if (PinJson.IsValid())
            {
                PinsArray.Add(MakeShared<FJsonValueObject>(PinJson));
            }
        }
        NodeObj->SetArrayField(TEXT("pins"), PinsArray);
    }

    return NodeObj;
}

TSharedPtr<FJsonObject> FUnrealMCPCommonUtils::NodeToCompactJson(UEdGraphNode* Node)
{
    if (!Node)
    {
        return nullptr;
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetStringField(TEXT("node_id"), Node->NodeGuid.ToString());
    ResultObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

    TArray<TSharedPtr<FJsonValue>> PinsArray;
    for (UEdGraphPin* Pin : Node->Pins)
    {
        if (Pin->bHidden)
        {
            continue;
        }

        TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
        PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
        PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));

        FString TypeStr = Pin->PinType.PinCategory.ToString();
        if (!Pin->PinType.PinSubCategory.IsNone())
        {
            TypeStr += TEXT("/") + Pin->PinType.PinSubCategory.ToString();
        }
        PinObj->SetStringField(TEXT("type"), TypeStr);

        PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
    }
    ResultObj->SetArrayField(TEXT("pins"), PinsArray);

    return ResultObj;
}

bool FUnrealMCPCommonUtils::SetObjectProperty(UObject* Object, const FString& PropertyName,
                                     const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage)
{
    if (!Object)
    {
        OutErrorMessage = TEXT("Invalid object");
        return false;
    }

    FProperty* Property = Object->GetClass()->FindPropertyByName(*PropertyName);
    if (!Property)
    {
        OutErrorMessage = FString::Printf(TEXT("Property not found: %s"), *PropertyName);
        return false;
    }

    void* PropertyAddr = Property->ContainerPtrToValuePtr<void>(Object);
    
    // Handle different property types
    if (Property->IsA<FBoolProperty>())
    {
        ((FBoolProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsBool());
        return true;
    }
    else if (Property->IsA<FIntProperty>())
    {
        int32 IntValue = static_cast<int32>(Value->AsNumber());
        FIntProperty* IntProperty = CastField<FIntProperty>(Property);
        if (IntProperty)
        {
            IntProperty->SetPropertyValue_InContainer(Object, IntValue);
            return true;
        }
    }
    else if (Property->IsA<FFloatProperty>())
    {
        ((FFloatProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsNumber());
        return true;
    }
    else if (Property->IsA<FStrProperty>())
    {
        ((FStrProperty*)Property)->SetPropertyValue(PropertyAddr, Value->AsString());
        return true;
    }
    else if (Property->IsA<FByteProperty>())
    {
        FByteProperty* ByteProp = CastField<FByteProperty>(Property);
        UEnum* EnumDef = ByteProp ? ByteProp->GetIntPropertyEnum() : nullptr;
        
        // If this is a TEnumAsByte property (has associated enum)
        if (EnumDef)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
                ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %d"), 
                      *PropertyName, ByteValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    uint8 ByteValue = FCString::Atoi(*EnumValueName);
                    ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %d"), 
                          *PropertyName, *EnumValueName, ByteValue);
                    return true;
                }
                
                // Handle qualified enum names (e.g., "Player0" or "EAutoReceiveInput::Player0")
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    ByteProp->SetPropertyValue(PropertyAddr, static_cast<uint8>(EnumValue));
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
        else
        {
            // Regular byte property
            uint8 ByteValue = static_cast<uint8>(Value->AsNumber());
            ByteProp->SetPropertyValue(PropertyAddr, ByteValue);
            return true;
        }
    }
    else if (Property->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProp = CastField<FEnumProperty>(Property);
        UEnum* EnumDef = EnumProp ? EnumProp->GetEnum() : nullptr;
        FNumericProperty* UnderlyingNumericProp = EnumProp ? EnumProp->GetUnderlyingProperty() : nullptr;
        
        if (EnumDef && UnderlyingNumericProp)
        {
            // Handle numeric value
            if (Value->Type == EJson::Number)
            {
                int64 EnumValue = static_cast<int64>(Value->AsNumber());
                UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                
                UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric value: %lld"), 
                      *PropertyName, EnumValue);
                return true;
            }
            // Handle string enum value
            else if (Value->Type == EJson::String)
            {
                FString EnumValueName = Value->AsString();
                
                // Try to convert numeric string to number first
                if (EnumValueName.IsNumeric())
                {
                    int64 EnumValue = FCString::Atoi64(*EnumValueName);
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to numeric string value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                
                // Handle qualified enum names
                if (EnumValueName.Contains(TEXT("::")))
                {
                    EnumValueName.Split(TEXT("::"), nullptr, &EnumValueName);
                }
                
                int64 EnumValue = EnumDef->GetValueByNameString(EnumValueName);
                if (EnumValue == INDEX_NONE)
                {
                    // Try with full name as fallback
                    EnumValue = EnumDef->GetValueByNameString(Value->AsString());
                }
                
                if (EnumValue != INDEX_NONE)
                {
                    UnderlyingNumericProp->SetIntPropertyValue(PropertyAddr, EnumValue);
                    
                    UE_LOG(LogTemp, Display, TEXT("Setting enum property %s to name value: %s -> %lld"), 
                          *PropertyName, *EnumValueName, EnumValue);
                    return true;
                }
                else
                {
                    // Log all possible enum values for debugging
                    UE_LOG(LogTemp, Warning, TEXT("Could not find enum value for '%s'. Available options:"), *EnumValueName);
                    for (int32 i = 0; i < EnumDef->NumEnums(); i++)
                    {
                        UE_LOG(LogTemp, Warning, TEXT("  - %s (value: %d)"), 
                               *EnumDef->GetNameStringByIndex(i), EnumDef->GetValueByIndex(i));
                    }
                    
                    OutErrorMessage = FString::Printf(TEXT("Could not find enum value for '%s'"), *EnumValueName);
                    return false;
                }
            }
        }
    }
    
    OutErrorMessage = FString::Printf(TEXT("Unsupported property type: %s for property %s"), 
                                    *Property->GetClass()->GetName(), *PropertyName);
    return false;
} 