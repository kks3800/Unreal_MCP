// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPPCGCommands.h"
#include "Commands/UnrealMCPPCGPropertyMarshaler.h"
#include "MCPCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

// PCG includes
#include "PCGGraph.h"
#include "PCGNode.h"
#include "PCGSettings.h"
#include "PCGPin.h"
#include "PCGEdge.h"

// Engine EdGraph for comment-box frames stored in UPCGGraph::ExtraEditorNodes
#include "EdGraphNode_Comment.h"

//=============================================================================
// Internal helpers (file-local)
//=============================================================================

namespace
{
	/**
	 * Strip the UE reflection decoration off a UPCGSettings subclass name to
	 * produce the short handle used by MCP clients. e.g.
	 *   PCGSurfaceSamplerSettings -> "SurfaceSampler"
	 *   PCGBlueprintSettings      -> "Blueprint"
	 *   PCGSomethingElse          -> "SomethingElse"   (no trailing "Settings")
	 * The transform must be invertible by ResolveSettingsClass.
	 *
	 * Note: UClass::GetName() on UObjects drops the leading 'U' from the C++
	 * identifier, so the input is "PCGFooSettings" not "UPCGFooSettings" —
	 * strip "PCG", not "UPCG".
	 */
	FString StripPCGSettingsTypeName(const FString& ClassName)
	{
		FString Result = ClassName;
		Result.RemoveFromStart(TEXT("PCG"));
		Result.RemoveFromEnd(TEXT("Settings"));
		return Result;
	}

	/**
	 * Inverse of StripPCGSettingsTypeName: given a short handle like
	 * "SurfaceSampler", walks all UPCGSettings subclasses and returns the one
	 * whose stripped name matches. Returns nullptr if none matches. Used by
	 * add_pcg_node and get_pcg_node_schema so callers can reference nodes by
	 * short type name instead of full UClass path.
	 */
	UClass* ResolveSettingsClass(const FString& ShortName)
	{
		if (ShortName.IsEmpty())
		{
			return nullptr;
		}

		TArray<UClass*> DerivedClasses;
		GetDerivedClasses(UPCGSettings::StaticClass(), DerivedClasses, /*bRecursive=*/true);
		for (UClass* Cls : DerivedClasses)
		{
			if (!Cls || Cls->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
			{
				continue;
			}
			if (StripPCGSettingsTypeName(Cls->GetName()) == ShortName)
			{
				return Cls;
			}
		}
		return nullptr;
	}
}

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("pcg_ping")) { return HandlePCGPing(Params); }

	// Graph Asset CRUD
	if (CommandType == TEXT("create_pcg_graph")) { return HandleCreatePCGGraph(Params); }
	if (CommandType == TEXT("delete_pcg_graph")) { return HandleDeletePCGGraph(Params); }
	if (CommandType == TEXT("duplicate_pcg_graph")) { return HandleDuplicatePCGGraph(Params); }
	if (CommandType == TEXT("rename_pcg_graph")) { return HandleRenamePCGGraph(Params); }
	if (CommandType == TEXT("list_pcg_graphs")) { return HandleListPCGGraphs(Params); }
	if (CommandType == TEXT("save_pcg_graph")) { return HandleSavePCGGraph(Params); }

	// Node-type discovery
	if (CommandType == TEXT("list_pcg_node_types")) { return HandleListPCGNodeTypes(Params); }
	if (CommandType == TEXT("get_pcg_node_schema")) { return HandleGetPCGNodeSchema(Params); }

	// Batch session
	if (CommandType == TEXT("begin_pcg_edit")) { return HandleBeginPCGEdit(Params); }
	if (CommandType == TEXT("end_pcg_edit")) { return HandleEndPCGEdit(Params); }

	// Node construction
	if (CommandType == TEXT("add_pcg_node")) { return HandleAddPCGNode(Params); }
	if (CommandType == TEXT("delete_pcg_node")) { return HandleDeletePCGNode(Params); }
	if (CommandType == TEXT("move_pcg_node")) { return HandleMovePCGNode(Params); }

	// Property set/get
	if (CommandType == TEXT("set_pcg_node_property")) { return HandleSetPCGNodeProperty(Params); }
	if (CommandType == TEXT("get_pcg_node_property")) { return HandleGetPCGNodeProperty(Params); }

	// Array item manipulation
	if (CommandType == TEXT("add_pcg_array_item")) { return HandleAddPCGArrayItem(Params); }
	if (CommandType == TEXT("remove_pcg_array_item")) { return HandleRemovePCGArrayItem(Params); }
	if (CommandType == TEXT("clear_pcg_array")) { return HandleClearPCGArray(Params); }

	// Edges
	if (CommandType == TEXT("connect_pcg_nodes")) { return HandleConnectPCGNodes(Params); }
	if (CommandType == TEXT("disconnect_pcg_pins")) { return HandleDisconnectPCGPins(Params); }

	// Layout
	if (CommandType == TEXT("auto_layout_pcg_graph")) { return HandleAutoLayoutPCGGraph(Params); }

	// Introspection
	if (CommandType == TEXT("get_pcg_graph_snapshot")) { return HandleGetPCGGraphSnapshot(Params); }
	if (CommandType == TEXT("get_pcg_node_info")) { return HandleGetPCGNodeInfo(Params); }
	if (CommandType == TEXT("list_pcg_node_pins")) { return HandleListPCGNodePins(Params); }

	// Display / annotation
	if (CommandType == TEXT("set_pcg_node_title")) { return HandleSetPCGNodeTitle(Params); }
	if (CommandType == TEXT("set_pcg_node_comment")) { return HandleSetPCGNodeComment(Params); }
	if (CommandType == TEXT("add_pcg_comment_box")) { return HandleAddPCGCommentBox(Params); }
	if (CommandType == TEXT("frame_pcg_nodes")) { return HandleFramePCGNodes(Params); }

	return CreateErrorResponse(FString::Printf(TEXT("Unknown PCG command: %s"), *CommandType));
}

//=============================================================================
// Response Helpers
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), true);
	if (Data.IsValid())
	{
		for (const auto& Pair : Data->Values)
		{
			Response->SetField(Pair.Key, Pair.Value);
		}
	}
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

//=============================================================================
// Handlers
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandlePCGPing(const TSharedPtr<FJsonObject>& Params)
{
	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetBoolField(TEXT("pong"), true);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleCreatePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphName;
	if (!Params->TryGetStringField(TEXT("name"), GraphName))
	{
		return CreateErrorResponse(TEXT("Missing 'name' parameter"));
	}

	FString PackagePath = TEXT("/Game/PCG");
	Params->TryGetStringField(TEXT("path"), PackagePath);

	const FString FullObjectPath = FString::Printf(TEXT("%s/%s.%s"), *PackagePath, *GraphName, *GraphName);
	if (UEditorAssetLibrary::DoesAssetExist(FullObjectPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("PCG graph already exists at %s"), *FullObjectPath));
	}

	UPackage* Package = CreatePackage(*FString::Printf(TEXT("%s/%s"), *PackagePath, *GraphName));
	if (!Package)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to create package for %s"), *FullObjectPath));
	}

	UPCGGraph* NewGraph = NewObject<UPCGGraph>(Package, UPCGGraph::StaticClass(), FName(*GraphName),
		RF_Public | RF_Standalone);
	if (!NewGraph)
	{
		return CreateErrorResponse(TEXT("NewObject<UPCGGraph> returned null"));
	}

	FAssetRegistryModule::AssetCreated(NewGraph);
	NewGraph->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("graph_path"), FullObjectPath);
	Data->SetStringField(TEXT("name"), GraphName);
	Data->SetStringField(TEXT("message"), TEXT("PCG graph created"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleDeletePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("path"), AssetPath))
	{
		return CreateErrorResponse(TEXT("Missing 'path' parameter"));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	// Class-type guard: refuse to delete assets that aren't PCG graphs. Prevents
	// a mis-typed path from destroying an unrelated asset (e.g. a blueprint).
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
	UPCGGraph* GraphToDelete = Cast<UPCGGraph>(LoadedAsset);
	if (!GraphToDelete)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Asset at %s is not a UPCGGraph"), *AssetPath));
	}

	// Refuse to delete a graph that is the active edit target — the in-memory
	// session would dangle on a destroyed UObject and the next mutation would
	// crash the editor. Caller must end_pcg_edit first.
	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (Context.IsEditing() && Context.GetActiveGraph() == GraphToDelete)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Cannot delete %s while it is the active PCG edit session target; call end_pcg_edit first"),
				*AssetPath));
	}

	// Close any asset editor for this graph before deletion. UPCGEditorGraph is
	// transient and rebuilt on open, but leaving the editor window referencing
	// a soon-to-be-destroyed UPCGGraph is a known crash trigger in 5.3.
#if WITH_EDITOR
	if (GEditor)
	{
		if (UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
		{
			AssetEditorSubsystem->CloseAllEditorsForAsset(GraphToDelete);
		}
	}
#endif

	if (!UEditorAssetLibrary::DeleteAsset(AssetPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to delete asset: %s"), *AssetPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("deleted"), AssetPath);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleDuplicatePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString SrcPath;
	if (!Params->TryGetStringField(TEXT("src_path"), SrcPath))
	{
		return CreateErrorResponse(TEXT("Missing 'src_path' parameter"));
	}

	FString DstPath;
	if (!Params->TryGetStringField(TEXT("dst_path"), DstPath))
	{
		return CreateErrorResponse(TEXT("Missing 'dst_path' parameter"));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(SrcPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Source asset not found: %s"), *SrcPath));
	}

	if (UEditorAssetLibrary::DoesAssetExist(DstPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Destination asset already exists: %s"), *DstPath));
	}

	// Class-type guard: only allow duplicating PCG graphs through this command.
	UObject* SrcAsset = UEditorAssetLibrary::LoadAsset(SrcPath);
	if (!Cast<UPCGGraph>(SrcAsset))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Source asset at %s is not a UPCGGraph"), *SrcPath));
	}

	UObject* Duplicated = UEditorAssetLibrary::DuplicateAsset(SrcPath, DstPath);
	if (!Duplicated)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to duplicate %s -> %s"), *SrcPath, *DstPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("src_path"), SrcPath);
	Data->SetStringField(TEXT("dst_path"), DstPath);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleRenamePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString OldPath;
	if (!Params->TryGetStringField(TEXT("old_path"), OldPath))
	{
		return CreateErrorResponse(TEXT("Missing 'old_path' parameter"));
	}

	FString NewName;
	if (!Params->TryGetStringField(TEXT("new_name"), NewName))
	{
		return CreateErrorResponse(TEXT("Missing 'new_name' parameter"));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(OldPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *OldPath));
	}

	// Class-type guard: ensure we're renaming an actual PCG graph.
	UObject* SrcAsset = UEditorAssetLibrary::LoadAsset(OldPath);
	if (!Cast<UPCGGraph>(SrcAsset))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Asset at %s is not a UPCGGraph"), *OldPath));
	}

	// OldPath is an object path like /Game/PCG/Foo.Foo. Strip the object suffix to
	// get the package name, then take its parent directory to locate the target
	// package path for the rename.
	const FString OldPackageName = FPackageName::ObjectPathToPackageName(OldPath);
	const FString ParentPath = FPackageName::GetLongPackagePath(OldPackageName);
	const FString NewFullObjectPath =
		FString::Printf(TEXT("%s/%s.%s"), *ParentPath, *NewName, *NewName);

	if (NewFullObjectPath == OldPath)
	{
		return CreateErrorResponse(TEXT("new_name matches current name; nothing to rename"));
	}

	if (UEditorAssetLibrary::DoesAssetExist(NewFullObjectPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Destination asset already exists: %s"), *NewFullObjectPath));
	}

	if (!UEditorAssetLibrary::RenameAsset(OldPath, NewFullObjectPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to rename %s -> %s"), *OldPath, *NewFullObjectPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("old_path"), OldPath);
	Data->SetStringField(TEXT("new_path"), NewFullObjectPath);
	Data->SetStringField(TEXT("new_name"), NewName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleListPCGGraphs(const TSharedPtr<FJsonObject>& Params)
{
	FString PathFilter;
	Params->TryGetStringField(TEXT("path_filter"), PathFilter);

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// UE 5.7: FARFilter uses ClassPaths (TArray<FTopLevelAssetPath>), not the
	// deprecated ClassNames. GetClassPathName() returns the right type directly.
	FARFilter Filter;
	Filter.ClassPaths.Add(UPCGGraph::StaticClass()->GetClassPathName());
	Filter.bRecursiveClasses = true;
	if (!PathFilter.IsEmpty())
	{
		Filter.PackagePaths.Add(FName(*PathFilter));
		Filter.bRecursivePaths = true;
	}

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssets(Filter, AssetList);

	TArray<TSharedPtr<FJsonValue>> GraphArray;
	for (const FAssetData& Asset : AssetList)
	{
		TSharedPtr<FJsonObject> GraphInfo = MakeShared<FJsonObject>();
		GraphInfo->SetStringField(TEXT("path"), Asset.GetObjectPathString());
		GraphInfo->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		GraphInfo->SetStringField(TEXT("package"), Asset.PackagePath.ToString());
		GraphArray.Add(MakeShared<FJsonValueObject>(GraphInfo));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetArrayField(TEXT("pcg_graphs"), GraphArray);
	Data->SetNumberField(TEXT("count"), GraphArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleSavePCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath;
	if (!Params->TryGetStringField(TEXT("path"), AssetPath))
	{
		return CreateErrorResponse(TEXT("Missing 'path' parameter"));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	// Class-type guard: avoid saving assets the caller didn't intend to save.
	UObject* LoadedAsset = UEditorAssetLibrary::LoadAsset(AssetPath);
	if (!Cast<UPCGGraph>(LoadedAsset))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Asset at %s is not a UPCGGraph"), *AssetPath));
	}

	// Pass bOnlyIfIsDirty=false so callers can use this as an explicit flush
	// after a batch of graph edits even if the dirty flag was already cleared.
	if (!UEditorAssetLibrary::SaveAsset(AssetPath, false))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to save asset: %s"), *AssetPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("saved"), AssetPath);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleListPCGNodeTypes(const TSharedPtr<FJsonObject>& Params)
{
	// Optional category filter against EPCGSettingsType enum value names
	// (e.g. "Sampler", "Filter", "Spawner"). Case-sensitive to match enum identifiers.
	FString CategoryFilter;
	const bool bHasCategoryFilter = Params.IsValid() && Params->TryGetStringField(TEXT("category"), CategoryFilter);

	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(UPCGSettings::StaticClass(), DerivedClasses, /*bRecursive=*/true);

	UEnum* SettingsTypeEnum = StaticEnum<EPCGSettingsType>();

	TArray<TSharedPtr<FJsonValue>> NodeTypeArray;
	for (UClass* Cls : DerivedClasses)
	{
		// Skip abstract/deprecated/reinstanced-for-hot-reload placeholders — they
		// cannot be instantiated as real nodes so listing them would mislead callers.
		if (!Cls || Cls->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists))
		{
			continue;
		}

		const UPCGSettings* CDO = Cast<const UPCGSettings>(Cls->GetDefaultObject());
		if (!CDO || !CDO->bExposeToLibrary)
		{
			continue;
		}

		const EPCGSettingsType Category = CDO->GetType();
		FString CategoryName;
		if (SettingsTypeEnum)
		{
			CategoryName = SettingsTypeEnum->GetNameStringByValue(static_cast<int64>(Category));
		}

		if (bHasCategoryFilter && CategoryName != CategoryFilter)
		{
			continue;
		}

		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("type_name"), StripPCGSettingsTypeName(Cls->GetName()));
		Entry->SetStringField(TEXT("class_path"), Cls->GetClassPathName().ToString());
		Entry->SetStringField(TEXT("category"), CategoryName);
		Entry->SetStringField(TEXT("display_name"), CDO->GetDefaultNodeTitle().ToString());
		NodeTypeArray.Add(MakeShared<FJsonValueObject>(Entry));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetArrayField(TEXT("node_types"), NodeTypeArray);
	Data->SetNumberField(TEXT("count"), NodeTypeArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleGetPCGNodeSchema(const TSharedPtr<FJsonObject>& Params)
{
	FString NodeType;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
	}

	UClass* SettingsClass = ResolveSettingsClass(NodeType);
	if (!SettingsClass)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Unknown PCG node type: %s"), *NodeType));
	}

	const UPCGSettings* CDO = Cast<const UPCGSettings>(SettingsClass->GetDefaultObject());
	if (!CDO)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to resolve CDO for %s"), *NodeType));
	}

	// Walk UPROPERTY fields on the settings class to expose the schema.
	// Categories/tooltips come from UPROPERTY metadata — the same source the editor
	// details panel reads — so schema output tracks what a human would see.
	TArray<TSharedPtr<FJsonValue>> PropertyArray;
	for (TFieldIterator<FProperty> It(SettingsClass); It; ++It)
	{
		FProperty* Property = *It;
		if (!Property)
		{
			continue;
		}

		// Only surface properties a user (or AI) is meant to touch. Skip transient,
		// deprecated, and private-script fields — they mirror details-panel filtering.
		if (Property->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_NativeAccessSpecifierPrivate))
		{
			continue;
		}

		TSharedPtr<FJsonObject> PropEntry = MakeShared<FJsonObject>();
		PropEntry->SetStringField(TEXT("name"), Property->GetName());
		PropEntry->SetStringField(TEXT("type"), Property->GetCPPType(nullptr, 0));

		// Scalar default values: round-trip through ExportTextItem_InContainer. For
		// non-scalar (struct/array/map/object) we emit "<complex>" — full fidelity
		// for complex defaults lands with the marshaler expansion in Tasks 12-14.
		FString DefaultValue;
		if (Property->IsA<FBoolProperty>() ||
			Property->IsA<FNumericProperty>() ||
			Property->IsA<FStrProperty>() ||
			Property->IsA<FNameProperty>() ||
			Property->IsA<FTextProperty>() ||
			Property->IsA<FEnumProperty>() ||
			Property->IsA<FByteProperty>())
		{
			Property->ExportTextItem_InContainer(DefaultValue, CDO, nullptr, nullptr, PPF_None);
		}
		else
		{
			DefaultValue = TEXT("<complex>");
		}
		PropEntry->SetStringField(TEXT("default"), DefaultValue);

		PropEntry->SetBoolField(TEXT("overridable"),
			Property->HasMetaData(TEXT("PCG_Overridable")));

#if WITH_EDITOR
		PropEntry->SetStringField(TEXT("category"), Property->GetMetaData(TEXT("Category")));
		PropEntry->SetStringField(TEXT("tooltip"), Property->GetMetaData(TEXT("ToolTip")));
#else
		PropEntry->SetStringField(TEXT("category"), FString());
		PropEntry->SetStringField(TEXT("tooltip"), FString());
#endif
		PropertyArray.Add(MakeShared<FJsonValueObject>(PropEntry));
	}

	// Instantiate a transient UPCGSettings subclass to read dynamic pin properties.
	// Many subclasses compute pins from their own state in InputPinProperties() /
	// OutputPinProperties(), so reading from a CDO can lie — a transient instance
	// gives the default layout a freshly-created node would have.
	UPCGSettings* Transient = NewObject<UPCGSettings>(GetTransientPackage(), SettingsClass);


#if ENGINE_MINOR_VERSION >= 5
	auto PinStatusToString = [](EPCGPinStatus Status) -> FString
	{
		switch (Status)
		{
		case EPCGPinStatus::Normal:              return TEXT("Normal");
		case EPCGPinStatus::Required:            return TEXT("Required");
		case EPCGPinStatus::Advanced:            return TEXT("Advanced");
		case EPCGPinStatus::OverrideOrUserParam: return TEXT("OverrideOrUserParam");
		}
		return TEXT("Unknown");
	};

	auto BuildPinArray = [&PinStatusToString](const TArray<FPCGPinProperties>& Pins) -> TArray<TSharedPtr<FJsonValue>>
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const FPCGPinProperties& Pin : Pins)
		{
			TSharedPtr<FJsonObject> PinEntry = MakeShared<FJsonObject>();
			PinEntry->SetStringField(TEXT("label"), Pin.Label.ToString());
			PinEntry->SetStringField(TEXT("allowed_types"), Pin.AllowedTypes.ToString());
			PinEntry->SetBoolField(TEXT("allow_multiple"), Pin.AllowsMultipleConnections());
			PinEntry->SetStringField(TEXT("status"), PinStatusToString(Pin.PinStatus));
			Out.Add(MakeShared<FJsonValueObject>(PinEntry));
		}
		return Out;
	};
#else
	auto BuildPinArray = [](const TArray<FPCGPinProperties>& Pins) -> TArray<TSharedPtr<FJsonValue>>
	{
		TArray<TSharedPtr<FJsonValue>> Out;
		for (const FPCGPinProperties& Pin : Pins)
		{
			TSharedPtr<FJsonObject> PinEntry = MakeShared<FJsonObject>();
			PinEntry->SetStringField(TEXT("label"), Pin.Label.ToString());
			PinEntry->SetNumberField(TEXT("allowed_types"), static_cast<int64>(Pin.AllowedTypes));
			Out.Add(MakeShared<FJsonValueObject>(PinEntry));
		}
		return Out;
	};
#endif

	TSharedPtr<FJsonObject> PinsObject = MakeShared<FJsonObject>();
#if ENGINE_MINOR_VERSION >= 5
	if (Transient)
	{
		PinsObject->SetArrayField(TEXT("input"), BuildPinArray(Transient->InputPinProperties()));
		PinsObject->SetArrayField(TEXT("output"), BuildPinArray(Transient->OutputPinProperties()));
	}
	else
#endif
	{
		PinsObject->SetArrayField(TEXT("input"), TArray<TSharedPtr<FJsonValue>>());
		PinsObject->SetArrayField(TEXT("output"), TArray<TSharedPtr<FJsonValue>>());
	}

	FString CategoryName;
	if (UEnum* SettingsTypeEnum = StaticEnum<EPCGSettingsType>())
	{
		CategoryName = SettingsTypeEnum->GetNameStringByValue(static_cast<int64>(CDO->GetType()));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("type_name"), StripPCGSettingsTypeName(SettingsClass->GetName()));
	Data->SetStringField(TEXT("class_path"), SettingsClass->GetClassPathName().ToString());
	Data->SetStringField(TEXT("category"), CategoryName);
	Data->SetStringField(TEXT("display_name"), CDO->GetDefaultNodeTitle().ToString());
	Data->SetObjectField(TEXT("pins"), PinsObject);
	Data->SetArrayField(TEXT("properties"), PropertyArray);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleBeginPCGEdit(const TSharedPtr<FJsonObject>& Params)
{
	FString GraphPath;
	if (!Params.IsValid() || !Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();

	// Nested sessions are not supported — surface the currently-active graph in
	// the error so the caller knows exactly what they need to EndEdit first.
	if (Context.IsEditing())
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("A PCG edit session is already active for %s; call end_pcg_edit first"),
				*Context.GetActiveGraphPath()));
	}

	if (!Context.BeginEdit(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to begin PCG edit for %s"), *GraphPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("graph_path"), Context.GetActiveGraphPath());
	Data->SetStringField(TEXT("message"), TEXT("PCG edit session started"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleEndPCGEdit(const TSharedPtr<FJsonObject>& Params)
{
	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditing())
	{
		return CreateErrorResponse(TEXT("No PCG edit session is active"));
	}

	bool bAutoLayout = false;
	bool bSave = false;
	if (Params.IsValid())
	{
		Params->TryGetBoolField(TEXT("auto_layout"), bAutoLayout);
		Params->TryGetBoolField(TEXT("save"), bSave);
	}

	// Capture the path BEFORE EndEdit clears session state so the response still
	// reports which graph was closed.
	const FString ClosedPath = Context.GetActiveGraphPath();

	if (!Context.EndEdit(bAutoLayout, bSave))
	{
		return CreateErrorResponse(TEXT("Failed to end PCG edit session"));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("graph_path"), ClosedPath);
	Data->SetBoolField(TEXT("auto_layout"), bAutoLayout);
	Data->SetBoolField(TEXT("saved"), bSave);
	Data->SetStringField(TEXT("message"), TEXT("PCG edit session ended"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleAddPCGNode(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeType;
	if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
	}

	// Must be in an active session so edits flow through the batched-notification
	// path. Graph path must match to prevent cross-session writes. IsEditingGraph
	// tolerates both canonical (/Game/Foo/Bar.Bar) and package-only paths.
	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGGraph* Graph = Context.GetActiveGraph();
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Active PCG graph pointer is null"));
	}

	UClass* SettingsClass = ResolveSettingsClass(NodeType);
	if (!SettingsClass)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Unknown PCG node type: %s"), *NodeType));
	}

	// Caller-supplied id wins; otherwise synthesize "<type>_<counter>".
	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId) || NodeId.IsEmpty())
	{
		NodeId = Context.GenerateNodeId(NodeType);
	}

	// Reject duplicate ids inside a single session so FindNode stays deterministic.
	if (Context.FindNode(NodeId) != nullptr)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("node_id %s is already registered in this session"), *NodeId));
	}

	// Position is optional — default to origin. Clients that care about layout pass
	// explicit coordinates; auto_layout_pcg_graph (Task 11) handles the lazy case.
	int32 PosX = 0;
	int32 PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray = nullptr;
	if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray && PositionArray->Num() >= 2)
	{
		PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
	}

	// AddNodeOfType creates the node, its default settings object, and wires them
	// together. OutSettings is an output param giving us the concrete settings
	// instance we then feed to the property marshaler.
	UPCGSettings* OutSettings = nullptr;
	UPCGNode* NewNode = Graph->AddNodeOfType(SettingsClass, OutSettings);
	if (!NewNode || !OutSettings)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("AddNodeOfType failed for %s"), *NodeType));
	}

	NewNode->SetNodePosition(PosX, PosY);

	// Apply supplied scalar properties. Unsupported types (struct/array/map/etc.)
	// are collected in skipped_properties rather than failing node creation —
	// clients that pass a mixed dict still get a usable node.
	TArray<TSharedPtr<FJsonValue>> AppliedProps;
	TArray<TSharedPtr<FJsonValue>> SkippedProps;

	const TSharedPtr<FJsonObject>* PropertiesObject = nullptr;
	if (Params->TryGetObjectField(TEXT("properties"), PropertiesObject) && PropertiesObject && PropertiesObject->IsValid())
	{
		UClass* ActualClass = OutSettings->GetClass();
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : (*PropertiesObject)->Values)
		{
			FProperty* Property = ActualClass->FindPropertyByName(FName(*Pair.Key));
			if (!Property)
			{
				TSharedPtr<FJsonObject> SkipEntry = MakeShared<FJsonObject>();
				SkipEntry->SetStringField(TEXT("name"), Pair.Key);
				SkipEntry->SetStringField(TEXT("reason"), TEXT("Property not found on settings class"));
				SkippedProps.Add(MakeShared<FJsonValueObject>(SkipEntry));
				continue;
			}

			const FString Error = FUnrealMCPPCGPropertyMarshaler::ApplyJsonValueToProperty(
				OutSettings, Property, Pair.Value);
			if (Error.IsEmpty())
			{
				AppliedProps.Add(MakeShared<FJsonValueString>(Pair.Key));
			}
			else
			{
				TSharedPtr<FJsonObject> SkipEntry = MakeShared<FJsonObject>();
				SkipEntry->SetStringField(TEXT("name"), Pair.Key);
				SkipEntry->SetStringField(TEXT("reason"), Error);
				SkippedProps.Add(MakeShared<FJsonValueObject>(SkipEntry));
			}
		}
	}

	// Required after in-place settings mutation so the node recomputes pin layout
	// (dynamic-pin nodes like Attribute Set From Loose read their own properties).
	NewNode->UpdateAfterSettingsChangeDuringCreation();

	Context.RegisterNode(NodeId, NewNode);

	TArray<TSharedPtr<FJsonValue>> PositionJson;
	PositionJson.Add(MakeShared<FJsonValueNumber>(PosX));
	PositionJson.Add(MakeShared<FJsonValueNumber>(PosY));

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("node_type"), NodeType);
	Data->SetArrayField(TEXT("position"), PositionJson);
	Data->SetArrayField(TEXT("applied_properties"), AppliedProps);
	Data->SetArrayField(TEXT("skipped_properties"), SkippedProps);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleDeletePCGNode(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGGraph* Graph = Context.GetActiveGraph();
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Active PCG graph pointer is null"));
	}

	// Refuse to delete the auto-created IO nodes — RemoveNode would succeed but
	// the graph would be left in a structurally-broken state.
	if (NodeId == TEXT("$input") || NodeId == TEXT("$output"))
	{
		return CreateErrorResponse(TEXT("Cannot delete reserved input/output nodes"));
	}

	UPCGNode* Node = Context.FindNode(NodeId);
	if (!Node)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node id not found in session: %s"), *NodeId));
	}

	Graph->RemoveNode(Node);
	Context.UnregisterNode(NodeId);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("deleted_node_id"), NodeId);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleMovePCGNode(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	double X = 0.0;
	double Y = 0.0;
	if (!Params->TryGetNumberField(TEXT("x"), X) || !Params->TryGetNumberField(TEXT("y"), Y))
	{
		return CreateErrorResponse(TEXT("Missing 'x' or 'y' parameter"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGNode* Node = Context.FindNode(NodeId);
	if (!Node)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node id not found in session: %s"), *NodeId));
	}

	const int32 PosX = static_cast<int32>(X);
	const int32 PosY = static_cast<int32>(Y);
	Node->SetNodePosition(PosX, PosY);

	TArray<TSharedPtr<FJsonValue>> PositionJson;
	PositionJson.Add(MakeShared<FJsonValueNumber>(PosX));
	PositionJson.Add(MakeShared<FJsonValueNumber>(PosY));

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetArrayField(TEXT("position"), PositionJson);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleSetPCGNodeProperty(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FString PropertyPath;
	if (!Params->TryGetStringField(TEXT("property_path"), PropertyPath))
	{
		return CreateErrorResponse(TEXT("Missing 'property_path' parameter"));
	}

	const TSharedPtr<FJsonValue> ValueField = Params->TryGetField(TEXT("value"));
	if (!ValueField.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGNode* Node = Context.FindNode(NodeId);
	if (!Node)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node id not found in session: %s"), *NodeId));
	}

	UPCGSettings* Settings = Node->GetSettings();
	if (!Settings)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node %s has no settings object"), *NodeId));
	}

	TArray<FPCGPropertyPathStep> Steps;
	FString ParseError;
	if (!FPCGPropertyPathResolver::Parse(PropertyPath, Steps, ParseError))
	{
		return CreateErrorResponse(ParseError);
	}

	void* LeafContainer = nullptr;
	FProperty* LeafProperty = nullptr;
	FString ResolveError;
	if (!FPCGPropertyPathResolver::Resolve(
			Settings, Settings->GetClass(), Steps, /*bStopAtArrayLeaf=*/false,
			LeafContainer, LeafProperty, ResolveError))
	{
		return CreateErrorResponse(ResolveError);
	}

	const FString ApplyError = FUnrealMCPPCGPropertyMarshaler::ApplyJsonValueToProperty(
		LeafContainer, LeafProperty, ValueField);
	if (!ApplyError.IsEmpty())
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to set '%s': %s"), *PropertyPath, *ApplyError));
	}

	// Dynamic-pin nodes recompute their pin layout from settings state, so the
	// node needs to know its settings changed. Same requirement as add_pcg_node.
	Node->UpdateAfterSettingsChangeDuringCreation();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("property_path"), PropertyPath);
	Data->SetBoolField(TEXT("applied"), true);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleGetPCGNodeProperty(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	FString PropertyPath;
	if (!Params->TryGetStringField(TEXT("property_path"), PropertyPath))
	{
		return CreateErrorResponse(TEXT("Missing 'property_path' parameter"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGNode* Node = Context.FindNode(NodeId);
	if (!Node)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node id not found in session: %s"), *NodeId));
	}

	UPCGSettings* Settings = Node->GetSettings();
	if (!Settings)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node %s has no settings object"), *NodeId));
	}

	TArray<FPCGPropertyPathStep> Steps;
	FString ParseError;
	if (!FPCGPropertyPathResolver::Parse(PropertyPath, Steps, ParseError))
	{
		return CreateErrorResponse(ParseError);
	}

	void* LeafContainer = nullptr;
	FProperty* LeafProperty = nullptr;
	FString ResolveError;
	if (!FPCGPropertyPathResolver::Resolve(
			Settings, Settings->GetClass(), Steps, /*bStopAtArrayLeaf=*/false,
			LeafContainer, LeafProperty, ResolveError))
	{
		return CreateErrorResponse(ResolveError);
	}

	// Inverse marshaler (Task 16) produces a JSON value matching the input form
	// of set_pcg_node_property so the set -> get -> set round-trip is stable.
	const TSharedPtr<FJsonValue> Serialized =
		FUnrealMCPPCGPropertyMarshaler::SerializePropertyToJson(LeafContainer, LeafProperty);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("property_path"), PropertyPath);
	Data->SetField(TEXT("value"), Serialized.IsValid() ? Serialized : MakeShared<FJsonValueNull>());
	return CreateSuccessResponse(Data);
}

namespace
{
	// File-local error-response builder — anonymous-namespace siblings of the
	// class handlers cannot call the private CreateErrorResponse, so we ship a
	// free function that emits the same {success:false, error:Msg} shape.
	TSharedPtr<FJsonObject> MakePCGErrorResponse(const FString& ErrorMessage)
	{
		TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
		Response->SetBoolField(TEXT("success"), false);
		Response->SetStringField(TEXT("error"), ErrorMessage);
		return Response;
	}

	// Shared prologue for the three array-op handlers. Resolves the session,
	// node, settings, and the target FArrayProperty in one pass so each handler
	// only needs the final helper + element count. Returning false populates
	// OutErrorResponse so the caller can forward it directly.
	bool ResolvePCGArrayTarget(
		const TSharedPtr<FJsonObject>& Params,
		FString& OutNodeId,
		FString& OutArrayPath,
		UPCGNode*& OutNode,
		FArrayProperty*& OutArrayProp,
		void*& OutArrayContainer,
		TSharedPtr<FJsonObject>& OutErrorResponse)
	{
		OutNode = nullptr;
		OutArrayProp = nullptr;
		OutArrayContainer = nullptr;

		if (!Params.IsValid())
		{
			OutErrorResponse = MakePCGErrorResponse(TEXT("Missing parameters"));
			return false;
		}

		FString GraphPath;
		if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
		{
			OutErrorResponse = MakePCGErrorResponse(TEXT("Missing 'graph_path' parameter"));
			return false;
		}

		if (!Params->TryGetStringField(TEXT("node_id"), OutNodeId))
		{
			OutErrorResponse = MakePCGErrorResponse(TEXT("Missing 'node_id' parameter"));
			return false;
		}

		if (!Params->TryGetStringField(TEXT("array_path"), OutArrayPath))
		{
			OutErrorResponse = MakePCGErrorResponse(TEXT("Missing 'array_path' parameter"));
			return false;
		}

		FMCPPCGContext& Context = FMCPPCGContext::Get();
		if (!Context.IsEditingGraph(GraphPath))
		{
			OutErrorResponse = MakePCGErrorResponse(
				FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
								*GraphPath));
			return false;
		}

		OutNode = Context.FindNode(OutNodeId);
		if (!OutNode)
		{
			OutErrorResponse = MakePCGErrorResponse(
				FString::Printf(TEXT("Node id not found in session: %s"), *OutNodeId));
			return false;
		}

		UPCGSettings* Settings = OutNode->GetSettings();
		if (!Settings)
		{
			OutErrorResponse = MakePCGErrorResponse(
				FString::Printf(TEXT("Node %s has no settings object"), *OutNodeId));
			return false;
		}

		TArray<FPCGPropertyPathStep> Steps;
		FString ParseError;
		if (!FPCGPropertyPathResolver::Parse(OutArrayPath, Steps, ParseError))
		{
			OutErrorResponse = MakePCGErrorResponse(ParseError);
			return false;
		}

		// Use bStopAtArrayLeaf so the resolver hands us the array property
		// itself rather than trying to descend into its elements. Any array-op
		// that targeted a specific index would be expressed via Path[n] and
		// would need different handling (not this helper's job).
		void* LeafContainer = nullptr;
		FProperty* LeafProperty = nullptr;
		FString ResolveError;
		if (!FPCGPropertyPathResolver::Resolve(
				Settings, Settings->GetClass(), Steps, /*bStopAtArrayLeaf=*/true,
				LeafContainer, LeafProperty, ResolveError))
		{
			OutErrorResponse = MakePCGErrorResponse(ResolveError);
			return false;
		}

		FArrayProperty* ArrayProp = CastField<FArrayProperty>(LeafProperty);
		if (!ArrayProp)
		{
			OutErrorResponse = MakePCGErrorResponse(
				FString::Printf(TEXT("Path '%s' resolved to %s, not an array property"),
					*OutArrayPath, *LeafProperty->GetClass()->GetName()));
			return false;
		}

		OutArrayProp = ArrayProp;
		OutArrayContainer = LeafContainer;
		return true;
	}
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleAddPCGArrayItem(const TSharedPtr<FJsonObject>& Params)
{
	FString NodeId;
	FString ArrayPath;
	UPCGNode* Node = nullptr;
	FArrayProperty* ArrayProp = nullptr;
	void* ArrayContainer = nullptr;
	TSharedPtr<FJsonObject> ErrorResponse;
	if (!ResolvePCGArrayTarget(Params, NodeId, ArrayPath, Node, ArrayProp, ArrayContainer, ErrorResponse))
	{
		return ErrorResponse;
	}

	// The resolver returns LeafContainer as the struct/object that OWNS the
	// array property, so ContainerPtrToValuePtr gives us the FScriptArray body.
	void* ArrayAddr = ArrayProp->ContainerPtrToValuePtr<void>(ArrayContainer);
	FScriptArrayHelper ArrayHelper(ArrayProp, ArrayAddr);

	// AddValue constructs a default-initialized element and returns its index.
	// Default construction runs the inner property's InitializeValue, which is
	// the same initial state a newly-spawned element would have in the editor.
	const int32 NewIndex = ArrayHelper.AddValue();

	// Optional 'value' field marshaled into the new element. The element
	// pointer is treated as the container for Inner's recursive call — the
	// Inner property's offset is 0 relative to the element buffer.
	const TSharedPtr<FJsonValue> ValueField = Params->TryGetField(TEXT("value"));
	if (ValueField.IsValid())
	{
		uint8* ElementPtr = ArrayHelper.GetRawPtr(NewIndex);
		const FString ApplyError = FUnrealMCPPCGPropertyMarshaler::ApplyJsonValueToProperty(
			ElementPtr, ArrayProp->Inner, ValueField);
		if (!ApplyError.IsEmpty())
		{
			// Roll back the addition so the array state matches what the caller
			// expected to see on failure. A partial struct with some fields
			// applied would be more surprising than an untouched array.
			ArrayHelper.RemoveValues(NewIndex, 1);
			return CreateErrorResponse(
				FString::Printf(TEXT("Failed to marshal value into new element: %s"), *ApplyError));
		}
	}

	Node->UpdateAfterSettingsChangeDuringCreation();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("array_path"), ArrayPath);
	Data->SetNumberField(TEXT("new_index"), NewIndex);
	Data->SetNumberField(TEXT("new_size"), ArrayHelper.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleRemovePCGArrayItem(const TSharedPtr<FJsonObject>& Params)
{
	FString NodeId;
	FString ArrayPath;
	UPCGNode* Node = nullptr;
	FArrayProperty* ArrayProp = nullptr;
	void* ArrayContainer = nullptr;
	TSharedPtr<FJsonObject> ErrorResponse;
	if (!ResolvePCGArrayTarget(Params, NodeId, ArrayPath, Node, ArrayProp, ArrayContainer, ErrorResponse))
	{
		return ErrorResponse;
	}

	int32 Index = INDEX_NONE;
	if (!Params->TryGetNumberField(TEXT("index"), Index))
	{
		return CreateErrorResponse(TEXT("Missing 'index' parameter"));
	}

	void* ArrayAddr = ArrayProp->ContainerPtrToValuePtr<void>(ArrayContainer);
	FScriptArrayHelper ArrayHelper(ArrayProp, ArrayAddr);
	if (!ArrayHelper.IsValidIndex(Index))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Index %d out of range (array size %d)"), Index, ArrayHelper.Num()));
	}

	ArrayHelper.RemoveValues(Index, 1);
	Node->UpdateAfterSettingsChangeDuringCreation();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("array_path"), ArrayPath);
	Data->SetNumberField(TEXT("removed_index"), Index);
	Data->SetNumberField(TEXT("new_size"), ArrayHelper.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleClearPCGArray(const TSharedPtr<FJsonObject>& Params)
{
	FString NodeId;
	FString ArrayPath;
	UPCGNode* Node = nullptr;
	FArrayProperty* ArrayProp = nullptr;
	void* ArrayContainer = nullptr;
	TSharedPtr<FJsonObject> ErrorResponse;
	if (!ResolvePCGArrayTarget(Params, NodeId, ArrayPath, Node, ArrayProp, ArrayContainer, ErrorResponse))
	{
		return ErrorResponse;
	}

	void* ArrayAddr = ArrayProp->ContainerPtrToValuePtr<void>(ArrayContainer);
	FScriptArrayHelper ArrayHelper(ArrayProp, ArrayAddr);
	ArrayHelper.EmptyValues();

	Node->UpdateAfterSettingsChangeDuringCreation();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("array_path"), ArrayPath);
	Data->SetBoolField(TEXT("cleared"), true);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleConnectPCGNodes(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString FromNodeId;
	FString ToNodeId;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'from_node' parameter"));
	}
	if (!Params->TryGetStringField(TEXT("to_node"), ToNodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'to_node' parameter"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGGraph* Graph = Context.GetActiveGraph();
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Active PCG graph pointer is null"));
	}

	UPCGNode* FromNode = Context.FindNode(FromNodeId);
	UPCGNode* ToNode = Context.FindNode(ToNodeId);
	if (!FromNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("from_node not found: %s"), *FromNodeId));
	}
	if (!ToNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("to_node not found: %s"), *ToNodeId));
	}

	// Resolve from-pin: caller override first, else pick the first unconnected
	// output pin. "Unconnected" here means no existing edges; single-connection
	// pins that are already wired up get skipped so we don't clobber prior edges.
	FString FromPinLabel;
	const UPCGPin* FromPin = nullptr;
	if (Params->TryGetStringField(TEXT("from_pin"), FromPinLabel) && !FromPinLabel.IsEmpty())
	{
		FromPin = FromNode->GetOutputPin(FName(*FromPinLabel));
		if (!FromPin)
		{
			return CreateErrorResponse(
				FString::Printf(TEXT("from_pin '%s' not found on node %s"), *FromPinLabel, *FromNodeId));
		}
	}
	else
	{
		for (const TObjectPtr<UPCGPin>& Candidate : FromNode->GetOutputPins())
		{
			if (!Candidate)
			{
				continue;
			}
			// Prefer an unconnected pin but fall back to any multi-connection pin
			// if the only outputs are already wired — common on fan-out patterns.
#if ENGINE_MINOR_VERSION >= 5
			if (!Candidate->IsConnected() || Candidate->AllowsMultipleConnections())
#else
			if (!Candidate->IsConnected())
#endif
			{
				FromPin = Candidate;
				FromPinLabel = Candidate->Properties.Label.ToString();
				break;
			}
		}
		if (!FromPin)
		{
			return CreateErrorResponse(
				FString::Printf(TEXT("No suitable output pin found on %s"), *FromNodeId));
		}
	}

	// Resolve to-pin: caller override first, else first compatible input pin that
	// either allows multiple connections or has no existing edges.
	FString ToPinLabel;
	const UPCGPin* ToPin = nullptr;
	if (Params->TryGetStringField(TEXT("to_pin"), ToPinLabel) && !ToPinLabel.IsEmpty())
	{
		ToPin = ToNode->GetInputPin(FName(*ToPinLabel));
		if (!ToPin)
		{
			return CreateErrorResponse(
				FString::Printf(TEXT("to_pin '%s' not found on node %s"), *ToPinLabel, *ToNodeId));
		}
	}
	else
	{
		for (const TObjectPtr<UPCGPin>& Candidate : ToNode->GetInputPins())
		{
			if (!Candidate)
			{
				continue;
			}
			if (!Candidate->IsCompatible(FromPin))
			{
				continue;
			}

#if ENGINE_MINOR_VERSION >= 5
			if (Candidate->IsConnected() && !Candidate->AllowsMultipleConnections())
#else
			if (Candidate->IsConnected())
#endif
			{
				continue;
			}
			ToPin = Candidate;
			ToPinLabel = Candidate->Properties.Label.ToString();
			break;
		}
		if (!ToPin)
		{
			return CreateErrorResponse(
				FString::Printf(TEXT("No compatible input pin found on %s"), *ToNodeId));
		}
	}

	// UPCGGraph::AddLabeledEdge returns true when it had to BREAK an existing
	// edge to make room for the new one, false otherwise. The caller-facing
	// naming below reflects that semantic rather than the misleading "connected"
	// label the raw return value implies.
	const bool bBrokeExistingEdge = Graph->AddLabeledEdge(
		FromNode, FName(*FromPinLabel), ToNode, FName(*ToPinLabel));

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("from_node"), FromNodeId);
	Data->SetStringField(TEXT("from_pin"), FromPinLabel);
	Data->SetStringField(TEXT("to_node"), ToNodeId);
	Data->SetStringField(TEXT("to_pin"), ToPinLabel);
	// edge_created is always true on the success path — the handler returns an
	// error response earlier if either pin cannot be resolved.
	Data->SetBoolField(TEXT("edge_created"), true);
	Data->SetBoolField(TEXT("broke_existing_edge"), bBrokeExistingEdge);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleDisconnectPCGPins(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString FromNodeId;
	FString ToNodeId;
	FString FromPinLabel;
	FString ToPinLabel;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNodeId) ||
		!Params->TryGetStringField(TEXT("to_node"), ToNodeId) ||
		!Params->TryGetStringField(TEXT("from_pin"), FromPinLabel) ||
		!Params->TryGetStringField(TEXT("to_pin"), ToPinLabel))
	{
		return CreateErrorResponse(TEXT("Missing one of: from_node, to_node, from_pin, to_pin"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGGraph* Graph = Context.GetActiveGraph();
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Active PCG graph pointer is null"));
	}

	UPCGNode* FromNode = Context.FindNode(FromNodeId);
	UPCGNode* ToNode = Context.FindNode(ToNodeId);
	if (!FromNode || !ToNode)
	{
		return CreateErrorResponse(TEXT("from_node or to_node not found in session"));
	}

	const bool bDisconnected = Graph->RemoveEdge(
		FromNode, FName(*FromPinLabel), ToNode, FName(*ToPinLabel));

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("from_node"), FromNodeId);
	Data->SetStringField(TEXT("from_pin"), FromPinLabel);
	Data->SetStringField(TEXT("to_node"), ToNodeId);
	Data->SetStringField(TEXT("to_pin"), ToPinLabel);
	Data->SetBoolField(TEXT("disconnected"), bDisconnected);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleAutoLayoutPCGGraph(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGGraph* Graph = Context.GetActiveGraph();
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Active PCG graph pointer is null"));
	}

	// Build a working set of every node in the graph plus the IO nodes. The IO
	// nodes are reported by GetNodes() as well on some PCG versions but we seed
	// them explicitly so seed-nodes anchor the BFS regardless.
	TArray<UPCGNode*> AllNodes;
	if (UPCGNode* InputNode = Graph->GetInputNode())
	{
		AllNodes.AddUnique(InputNode);
	}
	for (UPCGNode* Node : Graph->GetNodes())
	{
		if (Node)
		{
			AllNodes.AddUnique(Node);
		}
	}
	if (UPCGNode* OutputNode = Graph->GetOutputNode())
	{
		AllNodes.AddUnique(OutputNode);
	}

	if (AllNodes.Num() == 0)
	{
		TSharedPtr<FJsonObject> EmptyData = MakeShared<FJsonObject>();
		EmptyData->SetNumberField(TEXT("node_count"), 0);
		EmptyData->SetNumberField(TEXT("column_count"), 0);
		return CreateSuccessResponse(EmptyData);
	}

	// Longest-path layering: column(N) = 1 + max(column(upstream)). Iterate until
	// fixpoint with a cycle guard so a pathological graph can't spin forever.
	TMap<UPCGNode*, int32> ColumnOf;
	ColumnOf.Reserve(AllNodes.Num());
	for (UPCGNode* Node : AllNodes)
	{
		ColumnOf.Add(Node, 0);
	}

	const int32 MaxIterations = AllNodes.Num() + 2;
	for (int32 Iter = 0; Iter < MaxIterations; ++Iter)
	{
		bool bChanged = false;
		for (UPCGNode* Node : AllNodes)
		{
			int32 BestColumn = 0;
			for (const TObjectPtr<UPCGPin>& InputPin : Node->GetInputPins())
			{
				if (!InputPin)
				{
					continue;
				}
				for (const TObjectPtr<UPCGEdge>& Edge : InputPin->Edges)
				{
					if (!Edge || !Edge->InputPin || !Edge->InputPin->Node)
					{
						continue;
					}
					UPCGNode* Upstream = Edge->InputPin->Node;
					if (const int32* UpstreamCol = ColumnOf.Find(Upstream))
					{
						BestColumn = FMath::Max(BestColumn, *UpstreamCol + 1);
					}
				}
			}

			int32& Current = ColumnOf.FindChecked(Node);
			if (BestColumn > Current)
			{
				Current = BestColumn;
				bChanged = true;
			}
		}
		if (!bChanged)
		{
			break;
		}
	}

	// Stack nodes in each column in insertion order (matches the order AllNodes
	// was built in, which mirrors the underlying graph node order).
	TMap<int32, TArray<UPCGNode*>> NodesByColumn;
	for (UPCGNode* Node : AllNodes)
	{
		const int32 Column = ColumnOf.FindChecked(Node);
		NodesByColumn.FindOrAdd(Column).Add(Node);
	}

	constexpr int32 ColumnSpacing = 300;
	constexpr int32 RowSpacing = 180;

	int32 MaxColumn = 0;
	for (const TPair<int32, TArray<UPCGNode*>>& Pair : NodesByColumn)
	{
		MaxColumn = FMath::Max(MaxColumn, Pair.Key);
		int32 Row = 0;
		for (UPCGNode* Node : Pair.Value)
		{
			Node->SetNodePosition(Pair.Key * ColumnSpacing, Row * RowSpacing);
			++Row;
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetNumberField(TEXT("node_count"), AllNodes.Num());
	Data->SetNumberField(TEXT("column_count"), MaxColumn + 1);
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Introspection / Round-trip helpers
//=============================================================================

namespace
{
	/**
	 * Resolve the graph to operate on for a read-only introspection command.
	 * If an edit session is active and its graph path matches the requested
	 * path, return the session graph so the caller sees any uncommitted
	 * in-memory edits. Otherwise load the asset from disk (introspection must
	 * not require begin_pcg_edit — it's used by diagnostics and Phase 2
	 * spec round-tripping on untouched assets).
	 */
	UPCGGraph* ResolvePCGGraphForIntrospection(const FString& GraphPath)
	{
		FMCPPCGContext& Context = FMCPPCGContext::Get();
		if (Context.IsEditingGraph(GraphPath))
		{
			return Context.GetActiveGraph();
		}
		return LoadObject<UPCGGraph>(nullptr, *GraphPath);
	}

	/**
	 * Short-name id for a node in an introspection context:
	 *   - "$input"/"$output" for the auto-created IO nodes
	 *   - the session-registered id if a session is active and the node is
	 *     registered (stable across the edit flow)
	 *   - the node's internal FName as a last resort so offline introspection
	 *     still produces stable strings for round-tripping.
	 */
	FString ResolveIntrospectionNodeId(UPCGGraph* Graph, UPCGNode* Node)
	{
		if (!Node || !Graph)
		{
			return FString();
		}
		if (Node == Graph->GetInputNode())
		{
			return TEXT("$input");
		}
		if (Node == Graph->GetOutputNode())
		{
			return TEXT("$output");
		}

		FMCPPCGContext& Context = FMCPPCGContext::Get();
		if (Context.IsEditing() && Context.GetActiveGraph() == Graph)
		{
			const FString SessionId = Context.GetNodeId(Node);
			if (!SessionId.IsEmpty())
			{
				return SessionId;
			}
		}
		return Node->GetFName().ToString();
	}

	/**
	 * Serialize the non-default property set of a UPCGSettings object to a JSON
	 * object for snapshot emission. Matches the shape consumed by
	 * add_pcg_node's "properties" field so snapshots round-trip cleanly
	 * through Phase 2's apply_pcg_graph_spec.
	 *
	 * Filter rules:
	 *  - Skip transient, deprecated, private-script, and non-edit/non-BP-visible
	 *    fields (same as the schema filter in get_pcg_node_schema).
	 *  - Skip values that are byte-identical to the CDO default, UNLESS the
	 *    property is marked PCG_Overridable (those are the high-signal knobs a
	 *    reader cares about even when at default).
	 */
	TSharedPtr<FJsonObject> SerializeSettingsProperties(const UPCGSettings* Settings)
	{
		TSharedPtr<FJsonObject> Out = MakeShared<FJsonObject>();
		if (!Settings)
		{
			return Out;
		}

		UClass* ActualClass = Settings->GetClass();
		const UObject* CDO = ActualClass->GetDefaultObject();

		for (TFieldIterator<FProperty> It(ActualClass); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property)
			{
				continue;
			}

			// Same filter as get_pcg_node_schema, plus the edit-visible gate from
			// the task spec. Internal runtime state (CPF_Transient, deprecated
			// fields, etc.) should not appear in a round-trip snapshot.
			if (Property->HasAnyPropertyFlags(
					CPF_Transient | CPF_Deprecated | CPF_NativeAccessSpecifierPrivate))
			{
				continue;
			}
			if (!Property->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
			{
				continue;
			}

			const bool bIsOverridable = Property->HasMetaData(TEXT("PCG_Overridable"));
			if (!bIsOverridable && CDO && Property->Identical_InContainer(Settings, CDO))
			{
				// Byte-identical to default — omit to keep the snapshot compact.
				// Overridable properties are preserved because they're the "knobs"
				// humans and clients typically care about even at default values.
				continue;
			}

			const TSharedPtr<FJsonValue> PropValue =
				FUnrealMCPPCGPropertyMarshaler::SerializePropertyToJson(Settings, Property);
			if (PropValue.IsValid())
			{
				Out->SetField(Property->GetName(), PropValue);
			}
		}

		return Out;
	}

	/**
	 * Build a JSON object describing a single pin (label, type bitmask, allow
	 * multiple, current edges). Used by get_pcg_node_info and list_pcg_node_pins.
	 * Each edge lists the OTHER endpoint's node id + pin label so readers can
	 * walk connectivity without a second lookup.
	 */
	TSharedPtr<FJsonObject> SerializePin(
		UPCGGraph* Graph,
		UPCGPin* Pin,
		bool bIsOutputSide)
	{
		TSharedPtr<FJsonObject> PinJson = MakeShared<FJsonObject>();
		if (!Pin)
		{
			PinJson->SetStringField(TEXT("label"), TEXT(""));
			PinJson->SetBoolField(TEXT("is_connected"), false);
			PinJson->SetArrayField(TEXT("edges"), TArray<TSharedPtr<FJsonValue>>());
			return PinJson;
		}

		PinJson->SetStringField(TEXT("label"), Pin->Properties.Label.ToString());
#if ENGINE_MINOR_VERSION >= 5
		PinJson->SetStringField(TEXT("allowed_types"), Pin->Properties.AllowedTypes.ToString());
#else
		PinJson->SetNumberField(TEXT("allowed_types"), static_cast<int64>(Pin->Properties.AllowedTypes));
#endif
#if ENGINE_MINOR_VERSION >= 5
		PinJson->SetBoolField(TEXT("allow_multiple"), Pin->AllowsMultipleConnections());
#endif
		PinJson->SetBoolField(TEXT("is_connected"), Pin->IsConnected());

		TArray<TSharedPtr<FJsonValue>> EdgeArray;
		for (const TObjectPtr<UPCGEdge>& Edge : Pin->Edges)
		{
			if (!Edge)
			{
				continue;
			}

			// UPCGEdge semantics (verified via auto_layout_pcg_graph at line 1586):
			//   Edge->InputPin  -> upstream output pin (source side)
			//   Edge->OutputPin -> downstream input pin (sink side)
			// When serializing an output-side pin, the "other" end is Edge->OutputPin
			// (the downstream input). When serializing an input-side pin, the "other"
			// end is Edge->InputPin (the upstream output).
			const UPCGPin* OtherPin = bIsOutputSide ? Edge->OutputPin.Get() : Edge->InputPin.Get();
			if (!OtherPin || !OtherPin->Node)
			{
				continue;
			}

			TSharedPtr<FJsonObject> EdgeJson = MakeShared<FJsonObject>();
			EdgeJson->SetStringField(TEXT("other_node"),
				ResolveIntrospectionNodeId(Graph, OtherPin->Node));
			EdgeJson->SetStringField(TEXT("other_pin"), OtherPin->Properties.Label.ToString());
			EdgeArray.Add(MakeShared<FJsonValueObject>(EdgeJson));
		}
		PinJson->SetArrayField(TEXT("edges"), EdgeArray);

		return PinJson;
	}

	/** Collect all nodes that participate in snapshot/introspection, including IO nodes. */
	TArray<UPCGNode*> CollectAllGraphNodes(UPCGGraph* Graph)
	{
		TArray<UPCGNode*> AllNodes;
		if (!Graph)
		{
			return AllNodes;
		}
		if (UPCGNode* InputNode = Graph->GetInputNode())
		{
			AllNodes.AddUnique(InputNode);
		}
		for (UPCGNode* Node : Graph->GetNodes())
		{
			if (Node)
			{
				AllNodes.AddUnique(Node);
			}
		}
		if (UPCGNode* OutputNode = Graph->GetOutputNode())
		{
			AllNodes.AddUnique(OutputNode);
		}
		return AllNodes;
	}
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleGetPCGGraphSnapshot(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	UPCGGraph* Graph = ResolvePCGGraphForIntrospection(GraphPath);
	if (!Graph)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to resolve PCG graph: %s"), *GraphPath));
	}

	// Serialize every node in the graph (user nodes + $input/$output). Properties
	// are compact: CDO-equal non-overridable fields are elided in
	// SerializeSettingsProperties so the snapshot stays small enough to replay
	// through apply_pcg_graph_spec in Phase 2.
	TArray<UPCGNode*> AllNodes = CollectAllGraphNodes(Graph);

	TArray<TSharedPtr<FJsonValue>> NodeArray;
	for (UPCGNode* Node : AllNodes)
	{
		if (!Node)
		{
			continue;
		}

		TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();

		const FString NodeId = ResolveIntrospectionNodeId(Graph, Node);
		NodeJson->SetStringField(TEXT("id"), NodeId);

		UPCGSettings* Settings = Node->GetSettings();
		if (Settings)
		{
			NodeJson->SetStringField(TEXT("type"),
				StripPCGSettingsTypeName(Settings->GetClass()->GetName()));
		}
		else
		{
			NodeJson->SetStringField(TEXT("type"), FString());
		}

		TArray<TSharedPtr<FJsonValue>> PositionJson;
		PositionJson.Add(MakeShared<FJsonValueNumber>(Node->PositionX));
		PositionJson.Add(MakeShared<FJsonValueNumber>(Node->PositionY));
		NodeJson->SetArrayField(TEXT("position"), PositionJson);

		NodeJson->SetObjectField(TEXT("properties"), SerializeSettingsProperties(Settings));

		NodeArray.Add(MakeShared<FJsonValueObject>(NodeJson));
	}

	// Edge enumeration: each edge lives on BOTH sides (upstream output + downstream
	// input pin). To avoid duplication we walk output pins only and read their
	// Edges list — the edge is canonical there, and UPCGEdge::OutputPin gives us
	// the downstream sink node for the to_node field.
	TArray<TSharedPtr<FJsonValue>> EdgeArray;
	for (UPCGNode* Node : AllNodes)
	{
		if (!Node)
		{
			continue;
		}
		const FString FromNodeId = ResolveIntrospectionNodeId(Graph, Node);
		for (const TObjectPtr<UPCGPin>& OutputPin : Node->GetOutputPins())
		{
			if (!OutputPin)
			{
				continue;
			}
			const FString FromPinLabel = OutputPin->Properties.Label.ToString();
			for (const TObjectPtr<UPCGEdge>& Edge : OutputPin->Edges)
			{
				if (!Edge || !Edge->OutputPin || !Edge->OutputPin->Node)
				{
					continue;
				}
				TSharedPtr<FJsonObject> EdgeJson = MakeShared<FJsonObject>();
				EdgeJson->SetStringField(TEXT("from_node"), FromNodeId);
				EdgeJson->SetStringField(TEXT("from_pin"), FromPinLabel);
				EdgeJson->SetStringField(TEXT("to_node"),
					ResolveIntrospectionNodeId(Graph, Edge->OutputPin->Node));
				EdgeJson->SetStringField(TEXT("to_pin"), Edge->OutputPin->Properties.Label.ToString());
				EdgeArray.Add(MakeShared<FJsonValueObject>(EdgeJson));
			}
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("graph_path"), GraphPath);
	Data->SetArrayField(TEXT("nodes"), NodeArray);
	Data->SetArrayField(TEXT("edges"), EdgeArray);
	Data->SetNumberField(TEXT("node_count"), NodeArray.Num());
	Data->SetNumberField(TEXT("edge_count"), EdgeArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleGetPCGNodeInfo(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	UPCGGraph* Graph = ResolvePCGGraphForIntrospection(GraphPath);
	if (!Graph)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Failed to resolve PCG graph: %s"), *GraphPath));
	}

	// Reserved ids always resolve through the graph IO accessors regardless of
	// session state so offline callers still get a usable answer.
	UPCGNode* Node = nullptr;
	if (NodeId == TEXT("$input"))
	{
		Node = Graph->GetInputNode();
	}
	else if (NodeId == TEXT("$output"))
	{
		Node = Graph->GetOutputNode();
	}
	else
	{
		FMCPPCGContext& Context = FMCPPCGContext::Get();
		if (Context.IsEditing() && Context.GetActiveGraph() == Graph)
		{
			Node = Context.FindNode(NodeId);
		}
		// Fallback: scan by FName for offline introspection (no session).
		if (!Node)
		{
			const FName Target(*NodeId);
			for (UPCGNode* Candidate : Graph->GetNodes())
			{
				if (Candidate && Candidate->GetFName() == Target)
				{
					Node = Candidate;
					break;
				}
			}
		}
	}

	if (!Node)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node id not found: %s"), *NodeId));
	}

	UPCGSettings* Settings = Node->GetSettings();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("type"), Settings
		? StripPCGSettingsTypeName(Settings->GetClass()->GetName())
		: FString());

	TArray<TSharedPtr<FJsonValue>> PositionJson;
	PositionJson.Add(MakeShared<FJsonValueNumber>(Node->PositionX));
	PositionJson.Add(MakeShared<FJsonValueNumber>(Node->PositionY));
	Data->SetArrayField(TEXT("position"), PositionJson);

	Data->SetObjectField(TEXT("properties"), SerializeSettingsProperties(Settings));

	TArray<TSharedPtr<FJsonValue>> InputPinArray;
	for (const TObjectPtr<UPCGPin>& InputPin : Node->GetInputPins())
	{
		InputPinArray.Add(MakeShared<FJsonValueObject>(SerializePin(Graph, InputPin, /*bIsOutputSide=*/false)));
	}
	Data->SetArrayField(TEXT("input_pins"), InputPinArray);

	TArray<TSharedPtr<FJsonValue>> OutputPinArray;
	for (const TObjectPtr<UPCGPin>& OutputPin : Node->GetOutputPins())
	{
		OutputPinArray.Add(MakeShared<FJsonValueObject>(SerializePin(Graph, OutputPin, /*bIsOutputSide=*/true)));
	}
	Data->SetArrayField(TEXT("output_pins"), OutputPinArray);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleListPCGNodePins(const TSharedPtr<FJsonObject>& Params)
{
	// Pin-only subset of HandleGetPCGNodeInfo for debugging connection state
	// without pulling the full property payload.
	const TSharedPtr<FJsonObject> Full = HandleGetPCGNodeInfo(Params);
	if (!Full.IsValid())
	{
		return CreateErrorResponse(TEXT("Internal error: get_pcg_node_info returned null"));
	}

	bool bSuccess = false;
	Full->TryGetBoolField(TEXT("success"), bSuccess);
	if (!bSuccess)
	{
		return Full;
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();

	FString NodeId;
	Full->TryGetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("node_id"), NodeId);

	const TArray<TSharedPtr<FJsonValue>>* InputPinsPtr = nullptr;
	if (Full->TryGetArrayField(TEXT("input_pins"), InputPinsPtr) && InputPinsPtr)
	{
		Data->SetArrayField(TEXT("input_pins"), *InputPinsPtr);
	}
	else
	{
		Data->SetArrayField(TEXT("input_pins"), TArray<TSharedPtr<FJsonValue>>());
	}

	const TArray<TSharedPtr<FJsonValue>>* OutputPinsPtr = nullptr;
	if (Full->TryGetArrayField(TEXT("output_pins"), OutputPinsPtr) && OutputPinsPtr)
	{
		Data->SetArrayField(TEXT("output_pins"), *OutputPinsPtr);
	}
	else
	{
		Data->SetArrayField(TEXT("output_pins"), TArray<TSharedPtr<FJsonValue>>());
	}

	return CreateSuccessResponse(Data);
}

//=============================================================================
// Display / Annotation Handlers
//=============================================================================
//
// UPCGNode stores a display-title override (NodeTitle, FName) and a per-node
// sticky-note string (NodeComment, FString). Both live on the node wrapper
// rather than the settings object, so the generic set_pcg_node_property path
// can't reach them — these handlers expose them directly.
//
// Comment-box frames (the colored rectangles users wrap nodes in for grouping)
// are UEdGraphNode_Comment instances. PCG persists them via
// UPCGGraph::ExtraEditorNodes — a TArray<UObject*> that PCGEditorGraph
// duplicates into the editor graph on open. We append new comment nodes to
// that array; when the user reopens the PCG asset the frames appear.

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleSetPCGNodeTitle(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	// Empty title is allowed — resets the override so the node falls back to
	// the settings-class display name.
	FString Title;
	Params->TryGetStringField(TEXT("title"), Title);

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGNode* Node = Context.FindNode(NodeId);
	if (!Node)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node id not found in session: %s"), *NodeId));
	}

	Node->Modify();
	Node->NodeTitle = Title.IsEmpty() ? NAME_None : FName(*Title);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("title"), Title);
	Data->SetBoolField(TEXT("applied"), true);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleSetPCGNodeComment(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing 'node_id' parameter"));
	}

	// Empty comment clears the sticky note and hides the bubble.
	FString Comment;
	Params->TryGetStringField(TEXT("comment"), Comment);

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGNode* Node = Context.FindNode(NodeId);
	if (!Node)
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("Node id not found in session: %s"), *NodeId));
	}

#if WITH_EDITORONLY_DATA
	Node->Modify();
	Node->NodeComment = Comment;
	const bool bHasText = !Comment.IsEmpty();
	Node->bCommentBubbleVisible = bHasText ? 1 : 0;
	Node->bCommentBubblePinned = bHasText ? 1 : 0;
#endif

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("comment"), Comment);
	Data->SetBoolField(TEXT("applied"), true);
	return CreateSuccessResponse(Data);
}

namespace
{
	/**
	 * Append a UEdGraphNode_Comment to the PCG graph's ExtraEditorNodes array.
	 * GetExtraEditorNodes() exposes a const ref; the setter takes const-ptr
	 * elements, so we have to rebuild the array via copy + append.
	 */
	void AppendExtraEditorNode(UPCGGraph* Graph, UEdGraphNode* NewNode)
	{
		TArray<TObjectPtr<const UObject>> Updated;
		Updated.Reserve(Graph->GetExtraEditorNodes().Num() + 1);
		for (const TObjectPtr<UObject>& Existing : Graph->GetExtraEditorNodes())
		{
			Updated.Add(Existing.Get());
		}
		Updated.Add(NewNode);
		Graph->SetExtraEditorNodes(Updated);
	}

	/** Parse an optional 3- or 4-element color array. Defaults to a neutral grey-yellow. */
	FLinearColor ReadColor(const TSharedPtr<FJsonObject>& Params, const FLinearColor& Fallback)
	{
		const TArray<TSharedPtr<FJsonValue>>* ColorArray = nullptr;
		if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray && ColorArray->Num() >= 3)
		{
			const float R = static_cast<float>((*ColorArray)[0]->AsNumber());
			const float G = static_cast<float>((*ColorArray)[1]->AsNumber());
			const float B = static_cast<float>((*ColorArray)[2]->AsNumber());
			const float A = ColorArray->Num() >= 4
				? static_cast<float>((*ColorArray)[3]->AsNumber())
				: 1.0f;
			return FLinearColor(R, G, B, A);
		}
		return Fallback;
	}
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleAddPCGCommentBox(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	FString Text;
	Params->TryGetStringField(TEXT("text"), Text);

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGGraph* Graph = Context.GetActiveGraph();
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Active PCG graph pointer is null"));
	}

	// Position: required at minimum so the comment isn't dropped at the origin
	// on top of the input node. Size: optional, defaults match the engine's
	// default comment placement.
	int32 PosX = 0;
	int32 PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray = nullptr;
	if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray && PositionArray->Num() >= 2)
	{
		PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
	}

	int32 SizeX = 400;
	int32 SizeY = 200;
	const TArray<TSharedPtr<FJsonValue>>* SizeArray = nullptr;
	if (Params->TryGetArrayField(TEXT("size"), SizeArray) && SizeArray && SizeArray->Num() >= 2)
	{
		SizeX = static_cast<int32>((*SizeArray)[0]->AsNumber());
		SizeY = static_cast<int32>((*SizeArray)[1]->AsNumber());
	}

	const FLinearColor Color = ReadColor(Params, FLinearColor(1.0f, 0.85f, 0.3f, 0.4f));

	UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transactional);
	if (!CommentNode)
	{
		return CreateErrorResponse(TEXT("NewObject<UEdGraphNode_Comment> returned null"));
	}
	CommentNode->NodeComment = Text;
	CommentNode->NodePosX = PosX;
	CommentNode->NodePosY = PosY;
	CommentNode->NodeWidth = SizeX;
	CommentNode->NodeHeight = SizeY;
	CommentNode->CommentColor = Color;
	CommentNode->FontSize = 18;
	CommentNode->bColorCommentBubble = true;
	CommentNode->CreateNewGuid();

	Graph->Modify();
	AppendExtraEditorNode(Graph, CommentNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("comment_guid"), CommentNode->NodeGuid.ToString());
	Data->SetStringField(TEXT("text"), Text);
	TArray<TSharedPtr<FJsonValue>> PosJson;
	PosJson.Add(MakeShared<FJsonValueNumber>(PosX));
	PosJson.Add(MakeShared<FJsonValueNumber>(PosY));
	Data->SetArrayField(TEXT("position"), PosJson);
	TArray<TSharedPtr<FJsonValue>> SizeJson;
	SizeJson.Add(MakeShared<FJsonValueNumber>(SizeX));
	SizeJson.Add(MakeShared<FJsonValueNumber>(SizeY));
	Data->SetArrayField(TEXT("size"), SizeJson);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPPCGCommands::HandleFramePCGNodes(const TSharedPtr<FJsonObject>& Params)
{
	if (!Params.IsValid())
	{
		return CreateErrorResponse(TEXT("Missing parameters"));
	}

	FString GraphPath;
	if (!Params->TryGetStringField(TEXT("graph_path"), GraphPath))
	{
		return CreateErrorResponse(TEXT("Missing 'graph_path' parameter"));
	}

	const TArray<TSharedPtr<FJsonValue>>* NodeIdArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("node_ids"), NodeIdArray) || !NodeIdArray || NodeIdArray->Num() == 0)
	{
		return CreateErrorResponse(TEXT("Missing or empty 'node_ids' array"));
	}

	FString Text;
	Params->TryGetStringField(TEXT("text"), Text);

	// Padding sits between the bounding box of the framed nodes and the comment
	// frame itself; default leaves a comfortable margin without crowding labels.
	double Padding = 80.0;
	Params->TryGetNumberField(TEXT("padding"), Padding);

	FMCPPCGContext& Context = FMCPPCGContext::Get();
	if (!Context.IsEditingGraph(GraphPath))
	{
		return CreateErrorResponse(
			FString::Printf(TEXT("No active PCG edit session for graph '%s' (call begin_pcg_edit first)"),
							*GraphPath));
	}

	UPCGGraph* Graph = Context.GetActiveGraph();
	if (!Graph)
	{
		return CreateErrorResponse(TEXT("Active PCG graph pointer is null"));
	}

	// PCG nodes don't expose their rendered size; assume a nominal box so the
	// frame encloses the rightmost/bottommost extent of each node rather than
	// just its top-left anchor.
	constexpr int32 NodeNominalW = 280;
	constexpr int32 NodeNominalH = 120;

	int32 MinX = MAX_int32;
	int32 MinY = MAX_int32;
	int32 MaxRight = MIN_int32;
	int32 MaxBottom = MIN_int32;

	TArray<FString> ResolvedIds;
	TArray<FString> MissingIds;
	for (const TSharedPtr<FJsonValue>& Value : *NodeIdArray)
	{
		const FString Id = Value->AsString();
		UPCGNode* Node = Context.FindNode(Id);
		if (!Node)
		{
			MissingIds.Add(Id);
			continue;
		}
		int32 X = 0;
		int32 Y = 0;
		Node->GetNodePosition(X, Y);
		MinX = FMath::Min(MinX, X);
		MinY = FMath::Min(MinY, Y);
		MaxRight = FMath::Max(MaxRight, X + NodeNominalW);
		MaxBottom = FMath::Max(MaxBottom, Y + NodeNominalH);
		ResolvedIds.Add(Id);
	}

	if (ResolvedIds.Num() == 0)
	{
		return CreateErrorResponse(TEXT("None of the supplied node_ids resolved in this session"));
	}

	const int32 Pad = FMath::Max(0, FMath::RoundToInt(static_cast<float>(Padding)));
	// Extra vertical space at the top accommodates the comment title bar so it
	// doesn't visually overlap the first row of framed nodes.
	constexpr int32 TitleBarH = 40;
	const int32 BoxX = MinX - Pad;
	const int32 BoxY = MinY - Pad - TitleBarH;
	const int32 BoxW = (MaxRight - MinX) + 2 * Pad;
	const int32 BoxH = (MaxBottom - MinY) + 2 * Pad + TitleBarH;

	const FLinearColor Color = ReadColor(Params, FLinearColor(0.25f, 0.55f, 1.0f, 0.4f));

	UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(Graph, NAME_None, RF_Transactional);
	if (!CommentNode)
	{
		return CreateErrorResponse(TEXT("NewObject<UEdGraphNode_Comment> returned null"));
	}
	CommentNode->NodeComment = Text;
	CommentNode->NodePosX = BoxX;
	CommentNode->NodePosY = BoxY;
	CommentNode->NodeWidth = BoxW;
	CommentNode->NodeHeight = BoxH;
	CommentNode->CommentColor = Color;
	CommentNode->FontSize = 18;
	CommentNode->bColorCommentBubble = true;
	CommentNode->CreateNewGuid();

	Graph->Modify();
	AppendExtraEditorNode(Graph, CommentNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("comment_guid"), CommentNode->NodeGuid.ToString());
	Data->SetStringField(TEXT("text"), Text);
	TArray<TSharedPtr<FJsonValue>> PosJson;
	PosJson.Add(MakeShared<FJsonValueNumber>(BoxX));
	PosJson.Add(MakeShared<FJsonValueNumber>(BoxY));
	Data->SetArrayField(TEXT("position"), PosJson);
	TArray<TSharedPtr<FJsonValue>> SizeJson;
	SizeJson.Add(MakeShared<FJsonValueNumber>(BoxW));
	SizeJson.Add(MakeShared<FJsonValueNumber>(BoxH));
	Data->SetArrayField(TEXT("size"), SizeJson);
	TArray<TSharedPtr<FJsonValue>> ResolvedJson;
	for (const FString& Id : ResolvedIds)
	{
		ResolvedJson.Add(MakeShared<FJsonValueString>(Id));
	}
	Data->SetArrayField(TEXT("framed_node_ids"), ResolvedJson);
	if (MissingIds.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> MissingJson;
		for (const FString& Id : MissingIds)
		{
			MissingJson.Add(MakeShared<FJsonValueString>(Id));
		}
		Data->SetArrayField(TEXT("missing_node_ids"), MissingJson);
	}
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Command Registration
//=============================================================================

void FUnrealMCPPCGCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("pcg_ping"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("pcg_ping"), P); });

	// Graph Asset CRUD
	Registry.RegisterCommand(TEXT("create_pcg_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_pcg_graph"), P); });
	Registry.RegisterCommand(TEXT("delete_pcg_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_pcg_graph"), P); });
	Registry.RegisterCommand(TEXT("duplicate_pcg_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("duplicate_pcg_graph"), P); });
	Registry.RegisterCommand(TEXT("rename_pcg_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("rename_pcg_graph"), P); });
	Registry.RegisterCommand(TEXT("list_pcg_graphs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_pcg_graphs"), P); });
	Registry.RegisterCommand(TEXT("save_pcg_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("save_pcg_graph"), P); });

	// Node-type discovery
	Registry.RegisterCommand(TEXT("list_pcg_node_types"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_pcg_node_types"), P); });
	Registry.RegisterCommand(TEXT("get_pcg_node_schema"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_pcg_node_schema"), P); });

	// Batch session
	Registry.RegisterCommand(TEXT("begin_pcg_edit"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("begin_pcg_edit"), P); });
	Registry.RegisterCommand(TEXT("end_pcg_edit"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("end_pcg_edit"), P); });

	// Node construction
	Registry.RegisterCommand(TEXT("add_pcg_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_pcg_node"), P); });
	Registry.RegisterCommand(TEXT("delete_pcg_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_pcg_node"), P); });
	Registry.RegisterCommand(TEXT("move_pcg_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("move_pcg_node"), P); });

	// Property set/get
	Registry.RegisterCommand(TEXT("set_pcg_node_property"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_pcg_node_property"), P); });
	Registry.RegisterCommand(TEXT("get_pcg_node_property"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_pcg_node_property"), P); });

	// Array item manipulation
	Registry.RegisterCommand(TEXT("add_pcg_array_item"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_pcg_array_item"), P); });
	Registry.RegisterCommand(TEXT("remove_pcg_array_item"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_pcg_array_item"), P); });
	Registry.RegisterCommand(TEXT("clear_pcg_array"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("clear_pcg_array"), P); });

	// Edges
	Registry.RegisterCommand(TEXT("connect_pcg_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_pcg_nodes"), P); });
	Registry.RegisterCommand(TEXT("disconnect_pcg_pins"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("disconnect_pcg_pins"), P); });

	// Layout
	Registry.RegisterCommand(TEXT("auto_layout_pcg_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("auto_layout_pcg_graph"), P); });

	// Introspection
	Registry.RegisterCommand(TEXT("get_pcg_graph_snapshot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_pcg_graph_snapshot"), P); });
	Registry.RegisterCommand(TEXT("get_pcg_node_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_pcg_node_info"), P); });
	Registry.RegisterCommand(TEXT("list_pcg_node_pins"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_pcg_node_pins"), P); });

	// Display / annotation
	Registry.RegisterCommand(TEXT("set_pcg_node_title"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_pcg_node_title"), P); });
	Registry.RegisterCommand(TEXT("set_pcg_node_comment"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_pcg_node_comment"), P); });
	Registry.RegisterCommand(TEXT("add_pcg_comment_box"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_pcg_comment_box"), P); });
	Registry.RegisterCommand(TEXT("frame_pcg_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("frame_pcg_nodes"), P); });
}
