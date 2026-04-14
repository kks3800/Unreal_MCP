// Copyright UnrealMCP Contributors. All Rights Reserved.

#include "MCPCore.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "WidgetBlueprint.h"
#include "EditorAssetLibrary.h"
#include "Engine/Blueprint.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "PCGCommon.h"
#include "PCGGraph.h"
#include "PCGNode.h"

#if WITH_EDITOR
#include "Editor.h"                           // for GEditor
#include "Subsystems/AssetEditorSubsystem.h"  // for UAssetEditorSubsystem
#endif

// ============================================================================
// FMCPCommandRegistry
// ============================================================================

TUniquePtr<FMCPCommandRegistry> FMCPCommandRegistry::Instance;

FMCPCommandRegistry& FMCPCommandRegistry::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeUnique<FMCPCommandRegistry>();
	}
	return *Instance;
}

void FMCPCommandRegistry::RegisterCommand(const FString& CommandName, FCommandHandler Handler)
{
	ensureMsgf(!CommandHandlers.Contains(CommandName),
		TEXT("MCP: Duplicate command registration: '%s'. Second registration wins."), *CommandName);
	CommandHandlers.Add(CommandName, MoveTemp(Handler));
}

TSharedPtr<FJsonObject> FMCPCommandRegistry::ExecuteCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params)
{
	if (FCommandHandler* Handler = CommandHandlers.Find(CommandName))
	{
		return (*Handler)(Params);
	}
	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown command: %s"), *CommandName));
}

bool FMCPCommandRegistry::HasCommand(const FString& CommandName) const
{
	return CommandHandlers.Contains(CommandName);
}

TArray<FString> FMCPCommandRegistry::GetRegisteredCommands() const
{
	TArray<FString> Commands;
	CommandHandlers.GetKeys(Commands);
	return Commands;
}

// ============================================================================
// FMCPWidgetContext
// ============================================================================

TUniquePtr<FMCPWidgetContext> FMCPWidgetContext::Instance;

FMCPWidgetContext& FMCPWidgetContext::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeUnique<FMCPWidgetContext>();
	}
	return *Instance;
}

bool FMCPWidgetContext::BeginEdit(const FString& WidgetName)
{
	// Already editing?
	if (ActiveBlueprint)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPWidgetContext: Already editing %s, ending previous session"), *BlueprintPath);
		EndEdit();
	}

	// Search for the widget blueprint
	TArray<FAssetData> AssetList;
	FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	
	// Try exact name first
	FString SearchPath = FString::Printf(TEXT("/Game/%s"), *WidgetName);
	FAssetData AssetData = AssetRegistry.Get().GetAssetByObjectPath(FSoftObjectPath(SearchPath + TEXT(".") + WidgetName));
	
	if (!AssetData.IsValid())
	{
		// Search by name
		AssetRegistry.Get().GetAssetsByClass(UWidgetBlueprint::StaticClass()->GetClassPathName(), AssetList);
		for (const FAssetData& Asset : AssetList)
		{
			if (Asset.AssetName.ToString() == WidgetName)
			{
				AssetData = Asset;
				break;
			}
		}
	}

	if (!AssetData.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("MCPWidgetContext: Blueprint not found: %s"), *WidgetName);
		return false;
	}

	ActiveBlueprint = Cast<UWidgetBlueprint>(AssetData.GetAsset());
	BlueprintPath = AssetData.GetSoftObjectPath().GetAssetPath().ToString();
	bIsDirty = false;

	UE_LOG(LogTemp, Log, TEXT("MCPWidgetContext: Begin editing %s"), *BlueprintPath);
	return true;
}

bool FMCPWidgetContext::EndEdit()
{
	if (!ActiveBlueprint)
	{
		return false;
	}

	if (bIsDirty)
	{
		UE_LOG(LogTemp, Log, TEXT("MCPWidgetContext: Compiling and saving %s"), *BlueprintPath);
		FKismetEditorUtilities::CompileBlueprint(ActiveBlueprint);
		UEditorAssetLibrary::SaveAsset(BlueprintPath, false);
	}

	ActiveBlueprint = nullptr;
	BlueprintPath.Empty();
	bIsDirty = false;
	return true;
}

// ============================================================================
// FMCPBlueprintContext
// ============================================================================

TUniquePtr<FMCPBlueprintContext> FMCPBlueprintContext::Instance;

FMCPBlueprintContext& FMCPBlueprintContext::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeUnique<FMCPBlueprintContext>();
	}
	return *Instance;
}

bool FMCPBlueprintContext::BeginEdit(const FString& BlueprintName)
{
	// End any previous session
	if (ActiveBlueprint)
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPBlueprintContext: Already editing %s, ending previous session"), *BlueprintPath);
		EndEdit();
	}

	// Find the Blueprint using the shared utility
	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("MCPBlueprintContext: Blueprint not found: %s"), *BlueprintName);
		return false;
	}

	ActiveBlueprint = Blueprint;

	// Build the asset path for saving
	UPackage* Package = Blueprint->GetOutermost();
	if (Package)
	{
		BlueprintPath = Package->GetName();
	}

	bIsDirty = false;
	OperationLog.Empty();

	UE_LOG(LogTemp, Log, TEXT("MCPBlueprintContext: Begin editing %s (%s)"), *BlueprintName, *BlueprintPath);
	return true;
}

bool FMCPBlueprintContext::EndEdit(bool bAutoCompile, bool bAutoSave, const FString& AutoLayoutGraph)
{
	if (!ActiveBlueprint)
	{
		return false;
	}

	if (bIsDirty)
	{
		// Single structural modification notification
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(ActiveBlueprint);

		// Optional auto-layout
		if (!AutoLayoutGraph.IsEmpty())
		{
			UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(ActiveBlueprint, AutoLayoutGraph);
			if (Graph)
			{
				UE_LOG(LogTemp, Log, TEXT("MCPBlueprintContext: Auto-layout graph %s"), *AutoLayoutGraph);
				// Auto-layout uses simple column layout
				int32 CurrentX = 0;
				int32 CurrentY = 0;
				for (UEdGraphNode* Node : Graph->Nodes)
				{
					if (Node)
					{
						Node->NodePosX = CurrentX;
						Node->NodePosY = CurrentY;
						CurrentY += 200;
						if (CurrentY > 1200)
						{
							CurrentY = 0;
							CurrentX += 400;
						}
					}
				}
			}
		}

		// Optional compile
		if (bAutoCompile)
		{
			UE_LOG(LogTemp, Log, TEXT("MCPBlueprintContext: Compiling %s"), *BlueprintPath);
			FKismetEditorUtilities::CompileBlueprint(ActiveBlueprint);
		}

		// Optional save
		if (bAutoSave && !BlueprintPath.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("MCPBlueprintContext: Saving %s"), *BlueprintPath);
			UEditorAssetLibrary::SaveAsset(BlueprintPath, false);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("MCPBlueprintContext: End editing %s (%d operations logged)"),
		*BlueprintPath, OperationLog.Num());

	ActiveBlueprint = nullptr;
	BlueprintPath.Empty();
	bIsDirty = false;
	OperationLog.Empty();
	return true;
}

void FMCPBlueprintContext::LogOperation(const FString& OpName, bool bSuccess)
{
	OperationLog.Add(FString::Printf(TEXT("%s: %s"), *OpName, bSuccess ? TEXT("OK") : TEXT("FAILED")));
}

// ============================================================================
// FMCPPCGContext
// ============================================================================

TUniquePtr<FMCPPCGContext> FMCPPCGContext::Instance;

FMCPPCGContext& FMCPPCGContext::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeUnique<FMCPPCGContext>();
	}
	return *Instance;
}

bool FMCPPCGContext::BeginEdit(const FString& GraphPath)
{
	// Nested sessions are not supported: refuse rather than silently dropping the previous
	// graph's pending notifications. Caller must EndEdit first.
	if (ActiveGraph.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("MCPPCGContext: Already editing %s, refusing nested BeginEdit for %s"),
			*ActiveGraphPath, *GraphPath);
		return false;
	}

	UPCGGraph* Graph = LoadObject<UPCGGraph>(nullptr, *GraphPath);
	if (!Graph)
	{
		UE_LOG(LogTemp, Error, TEXT("MCPPCGContext: PCG graph not found: %s"), *GraphPath);
		return false;
	}

	ActiveGraph = Graph;
	// Normalize to the canonical package path so downstream path comparisons do not
	// trip on mixed input formats (matches FMCPBlueprintContext::BeginEdit).
	if (UPackage* Package = Graph->GetOutermost())
	{
		ActiveGraphPath = Package->GetName();
	}
	else
	{
		ActiveGraphPath = GraphPath;
	}
	IdToNode.Reset();
	NodeToId.Reset();
	NodeTypeCounters.Reset();

#if WITH_EDITOR
	Graph->DisableNotificationsForEditor();
#endif

	UE_LOG(LogTemp, Log, TEXT("MCPPCGContext: Begin editing %s"), *ActiveGraphPath);
	return true;
}

bool FMCPPCGContext::EndEdit(bool bAutoLayout, bool bSave)
{
	UPCGGraph* Graph = ActiveGraph.Get();
	if (!Graph)
	{
		return false;
	}

	// Capture the path before we clear state so the save path is still valid after cleanup.
	const FString SavedPath = ActiveGraphPath;

#if WITH_EDITOR
	Graph->EnableNotificationsForEditor();
	Graph->ForceNotificationForEditor(EPCGChangeType::Structural);

	// Ensure the package is marked dirty after all mutations in this session.
	// The initial MarkPackageDirty() at graph-creation time may have been cleared
	// by notification suppression or an intervening clean pass, causing SaveAsset
	// below to silently no-op on a clean package.
	Graph->MarkPackageDirty();

	// If this graph is currently open in the PCG editor, close it. UPCGEditorGraph
	// is RF_Transient — never serialized — and is rebuilt from UPCGGraph::Nodes on
	// every open via FPCGEditor::Initialize -> ReconstructGraph. Closing the editor
	// window forces the user's next open to rebuild the view from the freshly
	// serialized data model.
	if (GEditor)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->CloseAllEditorsForAsset(Graph);
		}
	}
#endif

	// Auto-layout is a no-op here; the real implementation lands in Task 11.

	if (bSave && !SavedPath.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("MCPPCGContext: Saving %s"), *SavedPath);
		UEditorAssetLibrary::SaveAsset(SavedPath, false);
	}

	UE_LOG(LogTemp, Log, TEXT("MCPPCGContext: End editing %s"), *SavedPath);

	// Always clear state, even if save fails, so a stuck session cannot wedge the context.
	ActiveGraph.Reset();
	ActiveGraphPath.Empty();
	IdToNode.Reset();
	NodeToId.Reset();
	NodeTypeCounters.Reset();
	return true;
}

bool FMCPPCGContext::IsEditingGraph(const FString& GraphPath) const
{
	if (!ActiveGraph.IsValid())
	{
		return false;
	}

	// Normalize to package form: if the input has ".ObjectName", strip it.
	// This matches how BeginEdit stores ActiveGraphPath via Package->GetName().
	FString Normalized = GraphPath;
	int32 DotIndex = INDEX_NONE;
	if (Normalized.FindChar(TEXT('.'), DotIndex))
	{
		Normalized.LeftInline(DotIndex, EAllowShrinking::No);
	}
	return Normalized == ActiveGraphPath;
}

void FMCPPCGContext::RegisterNode(const FString& Id, UPCGNode* Node)
{
	if (Id.IsEmpty() || !Node)
	{
		return;
	}

	// Remove any stale forward/reverse entries so we never leave orphans on a re-register.
	// Without this, a collision on either key would leave the bidirectional invariant broken
	// (FindNode and GetNodeId would disagree about the mapping).
	if (const TWeakObjectPtr<UPCGNode>* ExistingNode = IdToNode.Find(Id))
	{
		NodeToId.Remove(*ExistingNode);
	}
	if (const FString* ExistingId = NodeToId.Find(Node))
	{
		IdToNode.Remove(*ExistingId);
	}

	IdToNode.Add(Id, Node);
	NodeToId.Add(Node, Id);
}

void FMCPPCGContext::UnregisterNode(const FString& Id)
{
	if (const TWeakObjectPtr<UPCGNode>* Node = IdToNode.Find(Id))
	{
		NodeToId.Remove(*Node);
		IdToNode.Remove(Id);
	}
}

UPCGNode* FMCPPCGContext::FindNode(const FString& Id) const
{
	// Reserved ids map to the graph's auto-created IO nodes. Handling it here
	// means every downstream command (delete/move/connect/disconnect) gets this
	// translation for free without scattering $input/$output checks.
	if (Id == TEXT("$input") || Id == TEXT("$output"))
	{
		UPCGGraph* Graph = ActiveGraph.Get();
		if (!Graph)
		{
			return nullptr;
		}
		return (Id == TEXT("$input")) ? Graph->GetInputNode() : Graph->GetOutputNode();
	}

	if (const TWeakObjectPtr<UPCGNode>* Found = IdToNode.Find(Id))
	{
		return Found->Get();
	}
	return nullptr;
}

FString FMCPPCGContext::GetNodeId(UPCGNode* Node) const
{
	if (!Node)
	{
		return FString();
	}

	if (const FString* Found = NodeToId.Find(Node))
	{
		return *Found;
	}
	return FString();
}

FString FMCPPCGContext::GenerateNodeId(const FString& TypeShortName)
{
	// Counter is post-incremented so the first SurfaceSampler in a session gets
	// SurfaceSampler_0, matching zero-based indexing the clients expect.
	int32& Counter = NodeTypeCounters.FindOrAdd(TypeShortName);
	const FString Id = FString::Printf(TEXT("%s_%d"), *TypeShortName, Counter);
	++Counter;
	return Id;
}

// ============================================================================
// FMCPCommonUIConfig
// ============================================================================

TUniquePtr<FMCPCommonUIConfig> FMCPCommonUIConfig::Instance;

FMCPCommonUIConfig& FMCPCommonUIConfig::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeUnique<FMCPCommonUIConfig>();
		Instance->InitializeDefaults();
	}
	return *Instance;
}

void FMCPCommonUIConfig::SetWidgetPath(const FString& TypeName, const FString& BlueprintPath)
{
	WidgetPaths.Add(TypeName, BlueprintPath);
}

FString FMCPCommonUIConfig::GetWidgetPath(const FString& TypeName) const
{
	if (const FString* Path = WidgetPaths.Find(TypeName))
	{
		return *Path;
	}
	return FString();
}

bool FMCPCommonUIConfig::HasWidgetType(const FString& TypeName) const
{
	return WidgetPaths.Contains(TypeName);
}

TArray<FString> FMCPCommonUIConfig::GetConfiguredTypes() const
{
	TArray<FString> Types;
	WidgetPaths.GetKeys(Types);
	return Types;
}

void FMCPCommonUIConfig::InitializeDefaults()
{
	// Intentionally empty. Common UI widget template paths are project-specific
	// — there is no universally-correct default. Users configure per-project via
	// set_common_ui_config(type_name, blueprint_path) before calling the
	// template-backed tools like add_common_button / add_common_text.
	// Tools that don't require a user-supplied template (e.g. add_common_button_base
	// which instantiates the engine UCommonButtonBase class directly) work without
	// any configuration.
}

// ============================================================================
// FMCPStylePresets
// ============================================================================

TUniquePtr<FMCPStylePresets> FMCPStylePresets::Instance;

FMCPStylePresets& FMCPStylePresets::Get()
{
	if (!Instance.IsValid())
	{
		Instance = MakeUnique<FMCPStylePresets>();
	}
	return *Instance;
}

void FMCPStylePresets::CreatePreset(const FString& PresetName, const TSharedPtr<FJsonObject>& StyleData)
{
	Presets.Add(PresetName, StyleData);
}

TSharedPtr<FJsonObject> FMCPStylePresets::GetPreset(const FString& PresetName) const
{
	if (const TSharedPtr<FJsonObject>* Found = Presets.Find(PresetName))
	{
		return *Found;
	}
	return nullptr;
}

bool FMCPStylePresets::HasPreset(const FString& PresetName) const
{
	return Presets.Contains(PresetName);
}

TArray<FString> FMCPStylePresets::GetPresetNames() const
{
	TArray<FString> Names;
	Presets.GetKeys(Names);
	return Names;
}

void FMCPStylePresets::DeletePreset(const FString& PresetName)
{
	Presets.Remove(PresetName);
}

