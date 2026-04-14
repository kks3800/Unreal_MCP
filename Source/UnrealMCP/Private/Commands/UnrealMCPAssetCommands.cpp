#include "Commands/UnrealMCPAssetCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "Subsystems/EditorAssetSubsystem.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/ARFilter.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "UObject/ObjectRedirector.h"
#include "Misc/PackageName.h"

FUnrealMCPAssetCommands::FUnrealMCPAssetCommands()
{
}

//=============================================================================
// Command Dispatcher
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// Group A: Discovery
	if (CommandType == TEXT("list_assets"))
	{
		return HandleListAssets(Params);
	}
	else if (CommandType == TEXT("get_asset_info"))
	{
		return HandleGetAssetInfo(Params);
	}
	else if (CommandType == TEXT("find_asset_references"))
	{
		return HandleFindAssetReferences(Params);
	}
	else if (CommandType == TEXT("get_folder_structure"))
	{
		return HandleGetFolderStructure(Params);
	}
	else if (CommandType == TEXT("get_asset_class"))
	{
		return HandleGetAssetClass(Params);
	}
	// Group B: Organization
	else if (CommandType == TEXT("rename_asset"))
	{
		return HandleRenameAsset(Params);
	}
	else if (CommandType == TEXT("move_asset"))
	{
		return HandleMoveAsset(Params);
	}
	else if (CommandType == TEXT("move_directory"))
	{
		return HandleMoveDirectory(Params);
	}
	else if (CommandType == TEXT("make_directory"))
	{
		return HandleMakeDirectory(Params);
	}
	else if (CommandType == TEXT("duplicate_asset"))
	{
		return HandleDuplicateAsset(Params);
	}
	else if (CommandType == TEXT("save_asset"))
	{
		return HandleSaveAsset(Params);
	}
	else if (CommandType == TEXT("save_directory"))
	{
		return HandleSaveDirectory(Params);
	}
	// Group C: Safety
	else if (CommandType == TEXT("validate_asset_move"))
	{
		return HandleValidateAssetMove(Params);
	}
	else if (CommandType == TEXT("fix_redirectors"))
	{
		return HandleFixRedirectors(Params);
	}
	else if (CommandType == TEXT("is_protected_path"))
	{
		return HandleIsProtectedPath(Params);
	}
	// Group D: Analysis / Batch
	else if (CommandType == TEXT("analyze_folder_organization"))
	{
		return HandleAnalyzeFolderOrganization(Params);
	}
	else if (CommandType == TEXT("batch_move_assets"))
	{
		return HandleBatchMoveAssets(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Unknown asset command: %s"), *CommandType));
}

//=============================================================================
// Helpers
//=============================================================================

bool FUnrealMCPAssetCommands::IsProtectedPath(
	const FString& Path,
	const TArray<FString>& ProtectedPaths) const
{
	for (const FString& Protected : ProtectedPaths)
	{
		if (Path.StartsWith(Protected))
		{
			return true;
		}
	}
	return false;
}

TArray<FString> FUnrealMCPAssetCommands::GetProtectedPaths(
	const TSharedPtr<FJsonObject>& Params) const
{
	TArray<FString> Result;
	if (Params->HasField(TEXT("protected_paths")))
	{
		const TArray<TSharedPtr<FJsonValue>>& Arr = Params->GetArrayField(TEXT("protected_paths"));
		for (const TSharedPtr<FJsonValue>& Val : Arr)
		{
			FString Path = Val->AsString();
			if (!Path.IsEmpty())
			{
				// Normalise: ensure no trailing slash for consistent prefix matching
				Path.RemoveFromEnd(TEXT("/"));
				Result.Add(Path);
			}
		}
	}
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::BuildFolderTree(
	const FString& DirectoryPath,
	int32 CurrentDepth,
	int32 MaxDepth,
	bool bIncludeAssetCounts) const
{
	TSharedPtr<FJsonObject> Node = MakeShareable(new FJsonObject);

	// Extract folder name from path
	FString FolderName = FPaths::GetCleanFilename(DirectoryPath);
	if (FolderName.IsEmpty())
	{
		FolderName = DirectoryPath;
	}

	Node->SetStringField(TEXT("name"), FolderName);
	Node->SetStringField(TEXT("path"), DirectoryPath);

	if (bIncludeAssetCounts)
	{
		// Count assets directly in this folder (non-recursive)
		TArray<FString> Assets = UEditorAssetLibrary::ListAssets(DirectoryPath, false, false);
		Node->SetNumberField(TEXT("asset_count"), Assets.Num());
	}

	// Build children if within depth limit
	TArray<TSharedPtr<FJsonValue>> Children;
	if (MaxDepth < 0 || CurrentDepth < MaxDepth)
	{
		// List assets including folders to discover subfolders
		TArray<FString> AllItems = UEditorAssetLibrary::ListAssets(DirectoryPath, false, true);
		TSet<FString> SubDirs;

		for (const FString& Item : AllItems)
		{
			if (UEditorAssetLibrary::DoesDirectoryExist(Item))
			{
				SubDirs.Add(Item);
			}
		}

		// Also use the asset registry to find sub-paths
		IAssetRegistry* Registry = IAssetRegistry::Get();
		if (Registry)
		{
			TArray<FString> SubPaths;
			Registry->GetSubPaths(DirectoryPath, SubPaths, false);
			for (const FString& SubPath : SubPaths)
			{
				SubDirs.Add(SubPath);
			}
		}

		for (const FString& SubDir : SubDirs)
		{
			TSharedPtr<FJsonObject> ChildNode = BuildFolderTree(
				SubDir, CurrentDepth + 1, MaxDepth, bIncludeAssetCounts);
			Children.Add(MakeShareable(new FJsonValueObject(ChildNode)));
		}
	}

	Node->SetArrayField(TEXT("children"), Children);
	return Node;
}

bool FUnrealMCPAssetCommands::CheckNamingConvention(
	const FString& AssetName,
	const FString& ClassName,
	FString& OutExpectedPrefix) const
{
	// Map class names to expected prefixes
	struct FPrefixRule
	{
		FString ClassName;
		TArray<FString> AcceptedPrefixes;
	};

	static const TArray<FPrefixRule> Rules = {
		{ TEXT("StaticMesh"),           { TEXT("SM_") } },
		{ TEXT("SkeletalMesh"),         { TEXT("SK_"), TEXT("SKM_") } },
		{ TEXT("Material"),             { TEXT("M_") } },
		{ TEXT("MaterialInstanceConstant"), { TEXT("MI_") } },
		{ TEXT("Texture2D"),            { TEXT("T_") } },
		{ TEXT("WidgetBlueprint"),      { TEXT("WBP_") } },
		{ TEXT("AnimBlueprint"),        { TEXT("ABP_") } },
		{ TEXT("NiagaraSystem"),        { TEXT("NS_") } },
		{ TEXT("SoundCue"),             { TEXT("SC_") } },
		{ TEXT("MetaSoundSource"),      { TEXT("MS_") } },
		{ TEXT("Blueprint"),            { TEXT("BP_") } },
		{ TEXT("CurveFloat"),           { TEXT("C_") } },
		{ TEXT("CurveLinearColor"),     { TEXT("C_") } },
	};

	for (const FPrefixRule& Rule : Rules)
	{
		if (ClassName.Contains(Rule.ClassName))
		{
			OutExpectedPrefix = Rule.AcceptedPrefixes[0];
			for (const FString& Prefix : Rule.AcceptedPrefixes)
			{
				if (AssetName.StartsWith(Prefix))
				{
					return true;
				}
			}
			return false;
		}
	}

	// No rule found for this class - pass
	OutExpectedPrefix.Empty();
	return true;
}

FString FUnrealMCPAssetCommands::GetSuggestedFolderForClass(const FString& ClassName) const
{
	if (ClassName.Contains(TEXT("StaticMesh")) || ClassName.Contains(TEXT("SkeletalMesh")))
	{
		return TEXT("Meshes");
	}
	if (ClassName.Contains(TEXT("Material")))
	{
		return TEXT("Materials");
	}
	if (ClassName.Contains(TEXT("Texture")))
	{
		return TEXT("Textures");
	}
	if (ClassName.Contains(TEXT("WidgetBlueprint")))
	{
		return TEXT("Widgets");
	}
	if (ClassName.Contains(TEXT("AnimBlueprint")) || ClassName.Contains(TEXT("AnimSequence")) || ClassName.Contains(TEXT("AnimMontage")))
	{
		return TEXT("Animations");
	}
	if (ClassName.Contains(TEXT("Niagara")))
	{
		return TEXT("FX");
	}
	if (ClassName.Contains(TEXT("SoundCue")) || ClassName.Contains(TEXT("SoundWave")))
	{
		return TEXT("Sounds");
	}
	if (ClassName.Contains(TEXT("MetaSound")))
	{
		return TEXT("Audio");
	}
	if (ClassName.Contains(TEXT("Blueprint")))
	{
		return TEXT("Blueprints");
	}
	if (ClassName.Contains(TEXT("Curve")))
	{
		return TEXT("Curves");
	}
	return FString();
}

//=============================================================================
// Group A: Discovery
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleListAssets(
	const TSharedPtr<FJsonObject>& Params)
{
	FString DirectoryPath = Params->GetStringField(TEXT("directory_path"));
	if (DirectoryPath.IsEmpty())
	{
		DirectoryPath = TEXT("/Game");
	}

	bool bRecursive = true;
	if (Params->HasField(TEXT("recursive")))
	{
		bRecursive = Params->GetBoolField(TEXT("recursive"));
	}

	FString AssetClassFilter;
	if (Params->HasField(TEXT("asset_class")))
	{
		AssetClassFilter = Params->GetStringField(TEXT("asset_class"));
	}

	FString NamePattern;
	if (Params->HasField(TEXT("name_pattern")))
	{
		NamePattern = Params->GetStringField(TEXT("name_pattern"));
	}

	// Use IAssetRegistry directly — orders of magnitude faster than
	// UEditorAssetLibrary::ListAssets which does a synchronous disk scan.
	IAssetRegistry* AssetRegistry = IAssetRegistry::Get();

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DirectoryPath));
	Filter.bRecursivePaths = bRecursive;

	TArray<FAssetData> AssetDataList;
	AssetRegistry->GetAssets(Filter, AssetDataList);

	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (!AssetData.IsValid())
		{
			continue;
		}

		FString ClassName = AssetData.AssetClassPath.GetAssetName().ToString();

		// Apply class filter (fuzzy Contains match, same as original behaviour)
		if (!AssetClassFilter.IsEmpty() && !ClassName.Contains(AssetClassFilter))
		{
			continue;
		}

		// Apply name pattern filter if specified (supports * and ? wildcards)
		if (!NamePattern.IsEmpty() && !AssetData.AssetName.ToString().MatchesWildcard(NamePattern))
		{
			continue;
		}

		TSharedPtr<FJsonObject> AssetObj = MakeShareable(new FJsonObject);
		AssetObj->SetStringField(TEXT("path"), AssetData.GetObjectPathString());
		AssetObj->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
		AssetObj->SetStringField(TEXT("class"), ClassName);
		AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetObj)));
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetArrayField(TEXT("assets"), AssetsArray);
	Result->SetNumberField(TEXT("count"), AssetsArray.Num());
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleGetAssetInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath = Params->GetStringField(TEXT("asset_path"));
	if (AssetPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("asset_path is required"));
	}

	FAssetData AssetData;
	IAssetRegistry* Registry = IAssetRegistry::Get();
	if (Registry)
	{
		AssetData = Registry->GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	}

	if (!AssetData.IsValid())
	{
		AssetData = UEditorAssetLibrary::FindAssetData(AssetPath);
	}

	if (!AssetData.IsValid())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
	Result->SetStringField(TEXT("class"), AssetData.AssetClassPath.GetAssetName().ToString());
	Result->SetStringField(TEXT("package_name"), AssetData.PackageName.ToString());
	Result->SetStringField(TEXT("package_path"), AssetData.PackagePath.ToString());
	Result->SetBoolField(TEXT("is_redirector"), AssetData.IsRedirector());

	// Disk size via the package file
	FString PackageFilename;
	if (FPackageName::DoesPackageExist(AssetData.PackageName.ToString(), &PackageFilename))
	{
		int64 FileSize = IFileManager::Get().FileSize(*PackageFilename);
		Result->SetNumberField(TEXT("disk_size_bytes"), static_cast<double>(FileSize));
	}

	// Tags
	TSharedPtr<FJsonObject> TagsObj = MakeShareable(new FJsonObject);
	AssetData.TagsAndValues.ForEach([&TagsObj](TPair<FName, FAssetTagValueRef> Pair)
	{
		TagsObj->SetStringField(Pair.Key.ToString(), Pair.Value.AsString());
	});
	Result->SetObjectField(TEXT("tags"), TagsObj);

	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleFindAssetReferences(
	const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath = Params->GetStringField(TEXT("asset_path"));
	if (AssetPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("asset_path is required"));
	}

	bool bLoadToConfirm = false;
	if (Params->HasField(TEXT("load_to_confirm")))
	{
		bLoadToConfirm = Params->GetBoolField(TEXT("load_to_confirm"));
	}

	TArray<FString> Referencers = UEditorAssetLibrary::FindPackageReferencersForAsset(
		AssetPath, bLoadToConfirm);

	TArray<TSharedPtr<FJsonValue>> RefsArray;
	for (const FString& Ref : Referencers)
	{
		RefsArray.Add(MakeShareable(new FJsonValueString(Ref)));
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetArrayField(TEXT("referencers"), RefsArray);
	Result->SetNumberField(TEXT("count"), RefsArray.Num());
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleGetFolderStructure(
	const TSharedPtr<FJsonObject>& Params)
{
	FString DirectoryPath = Params->GetStringField(TEXT("directory_path"));
	if (DirectoryPath.IsEmpty())
	{
		DirectoryPath = TEXT("/Game");
	}

	int32 MaxDepth = -1; // unlimited by default
	if (Params->HasField(TEXT("max_depth")))
	{
		MaxDepth = static_cast<int32>(Params->GetNumberField(TEXT("max_depth")));
	}

	bool bIncludeAssetCounts = false;
	if (Params->HasField(TEXT("include_asset_counts")))
	{
		bIncludeAssetCounts = Params->GetBoolField(TEXT("include_asset_counts"));
	}

	TSharedPtr<FJsonObject> Tree = BuildFolderTree(DirectoryPath, 0, MaxDepth, bIncludeAssetCounts);

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetObjectField(TEXT("tree"), Tree);
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleGetAssetClass(
	const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath = Params->GetStringField(TEXT("asset_path"));
	if (AssetPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("asset_path is required"));
	}

	FAssetData AssetData;
	IAssetRegistry* Registry = IAssetRegistry::Get();
	if (Registry)
	{
		AssetData = Registry->GetAssetByObjectPath(FSoftObjectPath(AssetPath));
	}

	if (!AssetData.IsValid())
	{
		AssetData = UEditorAssetLibrary::FindAssetData(AssetPath);
	}

	if (!AssetData.IsValid())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	FString ClassName = AssetData.AssetClassPath.GetAssetName().ToString();
	FString ParentClassName;

	// Try to resolve the parent class
	UClass* AssetClass = AssetData.GetClass();
	if (AssetClass && AssetClass->GetSuperClass())
	{
		ParentClassName = AssetClass->GetSuperClass()->GetName();
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	Result->SetStringField(TEXT("class_name"), ClassName);
	Result->SetStringField(TEXT("parent_class"), ParentClassName);
	return Result;
}

//=============================================================================
// Group B: Organization
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleRenameAsset(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SourcePath = Params->GetStringField(TEXT("source_path"));
	FString DestPath = Params->GetStringField(TEXT("destination_path"));

	if (SourcePath.IsEmpty() || DestPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("source_path and destination_path are required"));
	}

	TArray<FString> ProtectedPaths = GetProtectedPaths(Params);
	if (IsProtectedPath(SourcePath, ProtectedPaths))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Source path is protected: %s"), *SourcePath));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Source asset not found: %s"), *SourcePath));
	}

	if (UEditorAssetLibrary::DoesAssetExist(DestPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Destination already exists: %s"), *DestPath));
	}

	bool bSuccess = UEditorAssetLibrary::RenameAsset(SourcePath, DestPath);

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), bSuccess);
	Result->SetStringField(TEXT("old_path"), SourcePath);
	Result->SetStringField(TEXT("new_path"), DestPath);
	if (!bSuccess)
	{
		Result->SetStringField(TEXT("error"), TEXT("RenameAsset failed - check the log for details"));
	}
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleMoveAsset(
	const TSharedPtr<FJsonObject>& Params)
{
	// In UE, moving an asset is the same operation as renaming to a new path
	return HandleRenameAsset(Params);
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleMoveDirectory(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SourcePath = Params->GetStringField(TEXT("source_path"));
	FString DestPath = Params->GetStringField(TEXT("destination_path"));

	if (SourcePath.IsEmpty() || DestPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("source_path and destination_path are required"));
	}

	TArray<FString> ProtectedPaths = GetProtectedPaths(Params);
	if (IsProtectedPath(SourcePath, ProtectedPaths))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Source directory is protected: %s"), *SourcePath));
	}

	if (!UEditorAssetLibrary::DoesDirectoryExist(SourcePath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Source directory not found: %s"), *SourcePath));
	}

	// Count assets before move
	TArray<FString> AssetsBefore = UEditorAssetLibrary::ListAssets(SourcePath, true, false);
	int32 AssetCount = AssetsBefore.Num();

	// RenameDirectory is on UEditorAssetSubsystem (instance method)
	bool bSuccess = false;
	UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if (AssetSubsystem)
	{
		bSuccess = AssetSubsystem->RenameDirectory(SourcePath, DestPath);
	}
	else
	{
		// Fallback: move assets one by one
		bSuccess = true;
		if (!UEditorAssetLibrary::DoesDirectoryExist(DestPath))
		{
			UEditorAssetLibrary::MakeDirectory(DestPath);
		}
		for (const FString& AssetPath : AssetsBefore)
		{
			FString RelativePath = AssetPath;
			RelativePath.RemoveFromStart(SourcePath);
			FString NewPath = DestPath + RelativePath;
			if (!UEditorAssetLibrary::RenameAsset(AssetPath, NewPath))
			{
				bSuccess = false;
			}
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), bSuccess);
	Result->SetStringField(TEXT("old_path"), SourcePath);
	Result->SetStringField(TEXT("new_path"), DestPath);
	Result->SetNumberField(TEXT("assets_moved"), AssetCount);
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleMakeDirectory(
	const TSharedPtr<FJsonObject>& Params)
{
	FString DirectoryPath = Params->GetStringField(TEXT("directory_path"));
	if (DirectoryPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("directory_path is required"));
	}

	bool bAlreadyExisted = UEditorAssetLibrary::DoesDirectoryExist(DirectoryPath);
	bool bSuccess = true;

	if (!bAlreadyExisted)
	{
		bSuccess = UEditorAssetLibrary::MakeDirectory(DirectoryPath);
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), bSuccess);
	Result->SetStringField(TEXT("path"), DirectoryPath);
	Result->SetBoolField(TEXT("already_existed"), bAlreadyExisted);
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleDuplicateAsset(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SourcePath = Params->GetStringField(TEXT("source_path"));
	FString DestPath = Params->GetStringField(TEXT("destination_path"));

	if (SourcePath.IsEmpty() || DestPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("source_path and destination_path are required"));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(SourcePath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Source asset not found: %s"), *SourcePath));
	}

	if (UEditorAssetLibrary::DoesAssetExist(DestPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Destination already exists: %s"), *DestPath));
	}

	UObject* DuplicatedAsset = UEditorAssetLibrary::DuplicateAsset(SourcePath, DestPath);
	bool bSuccess = (DuplicatedAsset != nullptr);

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), bSuccess);
	Result->SetStringField(TEXT("source_path"), SourcePath);
	Result->SetStringField(TEXT("destination_path"), DestPath);
	if (!bSuccess)
	{
		Result->SetStringField(TEXT("error"), TEXT("DuplicateAsset failed"));
	}
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleSaveAsset(
	const TSharedPtr<FJsonObject>& Params)
{
	FString AssetPath = Params->GetStringField(TEXT("asset_path"));
	if (AssetPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("asset_path is required"));
	}

	bool bOnlyIfDirty = true;
	if (Params->HasField(TEXT("only_if_dirty")))
	{
		bOnlyIfDirty = Params->GetBoolField(TEXT("only_if_dirty"));
	}

	if (!UEditorAssetLibrary::DoesAssetExist(AssetPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Asset not found: %s"), *AssetPath));
	}

	bool bSuccess = UEditorAssetLibrary::SaveAsset(AssetPath, bOnlyIfDirty);

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), bSuccess);
	Result->SetStringField(TEXT("asset_path"), AssetPath);
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleSaveDirectory(
	const TSharedPtr<FJsonObject>& Params)
{
	FString DirectoryPath = Params->GetStringField(TEXT("directory_path"));
	if (DirectoryPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("directory_path is required"));
	}

	bool bRecursive = true;
	if (Params->HasField(TEXT("recursive")))
	{
		bRecursive = Params->GetBoolField(TEXT("recursive"));
	}

	bool bOnlyIfDirty = true;
	if (Params->HasField(TEXT("only_if_dirty")))
	{
		bOnlyIfDirty = Params->GetBoolField(TEXT("only_if_dirty"));
	}

	if (!UEditorAssetLibrary::DoesDirectoryExist(DirectoryPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Directory not found: %s"), *DirectoryPath));
	}

	// Use subsystem for SaveDirectory
	bool bSuccess = false;
	UEditorAssetSubsystem* AssetSubsystem = GEditor->GetEditorSubsystem<UEditorAssetSubsystem>();
	if (AssetSubsystem)
	{
		bSuccess = AssetSubsystem->SaveDirectory(DirectoryPath, bOnlyIfDirty, bRecursive);
	}
	else
	{
		// Fallback: save assets individually
		TArray<FString> Assets = UEditorAssetLibrary::ListAssets(DirectoryPath, bRecursive, false);
		bSuccess = true;
		for (const FString& AssetPath : Assets)
		{
			if (!UEditorAssetLibrary::SaveAsset(AssetPath, bOnlyIfDirty))
			{
				bSuccess = false;
			}
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), bSuccess);
	Result->SetStringField(TEXT("directory_path"), DirectoryPath);
	return Result;
}

//=============================================================================
// Group C: Safety
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleValidateAssetMove(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SourcePath = Params->GetStringField(TEXT("source_path"));
	FString DestPath = Params->GetStringField(TEXT("destination_path"));

	if (SourcePath.IsEmpty() || DestPath.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("source_path and destination_path are required"));
	}

	TArray<FString> ProtectedPaths = GetProtectedPaths(Params);

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("source_path"), SourcePath);
	Result->SetStringField(TEXT("destination_path"), DestPath);

	// Check: source exists
	bool bSourceExists = UEditorAssetLibrary::DoesAssetExist(SourcePath);
	Result->SetBoolField(TEXT("source_exists"), bSourceExists);

	// Check: destination already exists
	bool bDestExists = UEditorAssetLibrary::DoesAssetExist(DestPath);
	Result->SetBoolField(TEXT("destination_exists"), bDestExists);

	// Check: is protected
	bool bIsProtected = IsProtectedPath(SourcePath, ProtectedPaths);
	Result->SetBoolField(TEXT("is_protected"), bIsProtected);

	// Find referencers
	TArray<FString> Referencers = UEditorAssetLibrary::FindPackageReferencersForAsset(SourcePath, false);
	TArray<TSharedPtr<FJsonValue>> RefsArray;
	for (const FString& Ref : Referencers)
	{
		RefsArray.Add(MakeShareable(new FJsonValueString(Ref)));
	}
	Result->SetArrayField(TEXT("referencers"), RefsArray);
	Result->SetNumberField(TEXT("referencer_count"), RefsArray.Num());

	// Overall verdict
	bool bCanMove = bSourceExists && !bDestExists && !bIsProtected;
	Result->SetBoolField(TEXT("can_move"), bCanMove);

	// Warnings
	TArray<TSharedPtr<FJsonValue>> Warnings;
	if (!bSourceExists)
	{
		Warnings.Add(MakeShareable(new FJsonValueString(TEXT("Source asset does not exist"))));
	}
	if (bDestExists)
	{
		Warnings.Add(MakeShareable(new FJsonValueString(TEXT("Destination already exists - would overwrite"))));
	}
	if (bIsProtected)
	{
		Warnings.Add(MakeShareable(new FJsonValueString(TEXT("Source is in a protected directory"))));
	}
	if (RefsArray.Num() > 0)
	{
		Warnings.Add(MakeShareable(new FJsonValueString(
			FString::Printf(TEXT("Asset has %d referencers that will need redirectors"), RefsArray.Num()))));
	}
	Result->SetArrayField(TEXT("warnings"), Warnings);

	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleFixRedirectors(
	const TSharedPtr<FJsonObject>& Params)
{
	FString DirectoryPath = TEXT("/Game");
	if (Params->HasField(TEXT("directory_path")))
	{
		DirectoryPath = Params->GetStringField(TEXT("directory_path"));
	}

	bool bRecursive = true;
	if (Params->HasField(TEXT("recursive")))
	{
		bRecursive = Params->GetBoolField(TEXT("recursive"));
	}

	// Find all redirectors in the directory
	IAssetRegistry* Registry = IAssetRegistry::Get();
	if (!Registry)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Asset Registry not available"));
	}

	FARFilter Filter;
	Filter.PackagePaths.Add(FName(*DirectoryPath));
	Filter.bRecursivePaths = bRecursive;
	Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/CoreUObject"), TEXT("ObjectRedirector")));

	TArray<FAssetData> RedirectorAssets;
	Registry->GetAssets(Filter, RedirectorAssets);

	if (RedirectorAssets.Num() == 0)
	{
		TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
		Result->SetBoolField(TEXT("success"), true);
		Result->SetNumberField(TEXT("redirectors_fixed"), 0);
		Result->SetStringField(TEXT("message"), TEXT("No redirectors found"));
		return Result;
	}

	// Load the redirector objects
	TArray<UObjectRedirector*> Redirectors;
	for (const FAssetData& AssetData : RedirectorAssets)
	{
		UObject* Obj = AssetData.GetAsset();
		UObjectRedirector* Redirector = Cast<UObjectRedirector>(Obj);
		if (Redirector)
		{
			Redirectors.Add(Redirector);
		}
	}

	// Fix them up via IAssetTools
	if (Redirectors.Num() > 0)
	{
		FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools"));
		AssetToolsModule.Get().FixupReferencers(Redirectors, false, ERedirectFixupMode::DeleteFixedUpRedirectors);
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetNumberField(TEXT("redirectors_fixed"), Redirectors.Num());
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleIsProtectedPath(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Path = Params->GetStringField(TEXT("path"));
	if (Path.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("path is required"));
	}

	TArray<FString> ProtectedPaths = GetProtectedPaths(Params);

	bool bIsProtected = false;
	FString MatchedPath;
	for (const FString& Protected : ProtectedPaths)
	{
		if (Path.StartsWith(Protected))
		{
			bIsProtected = true;
			MatchedPath = Protected;
			break;
		}
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("path"), Path);
	Result->SetBoolField(TEXT("is_protected"), bIsProtected);
	if (bIsProtected)
	{
		Result->SetStringField(TEXT("reason"),
			FString::Printf(TEXT("Path starts with protected directory: %s"), *MatchedPath));
	}
	return Result;
}

//=============================================================================
// Group D: Analysis / Batch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleAnalyzeFolderOrganization(
	const TSharedPtr<FJsonObject>& Params)
{
	FString DirectoryPath = Params->GetStringField(TEXT("directory_path"));
	if (DirectoryPath.IsEmpty())
	{
		DirectoryPath = TEXT("/Game");
	}

	TArray<FString> ProtectedPaths = GetProtectedPaths(Params);

	TArray<FString> AssetPaths = UEditorAssetLibrary::ListAssets(DirectoryPath, true, false);
	IAssetRegistry* Registry = IAssetRegistry::Get();

	// Counters
	TMap<FString, int32> AssetsByClass;
	TArray<TSharedPtr<FJsonValue>> MisplacedAssets;
	TArray<TSharedPtr<FJsonValue>> NamingIssues;
	int32 TotalAssets = 0;

	for (const FString& AssetPath : AssetPaths)
	{
		FAssetData AssetData;
		if (Registry)
		{
			AssetData = Registry->GetAssetByObjectPath(FSoftObjectPath(AssetPath));
		}
		else
		{
			AssetData = UEditorAssetLibrary::FindAssetData(AssetPath);
		}

		if (!AssetData.IsValid())
		{
			continue;
		}

		TotalAssets++;
		FString ClassName = AssetData.AssetClassPath.GetAssetName().ToString();
		FString AssetName = AssetData.AssetName.ToString();

		// Count by class
		int32& Count = AssetsByClass.FindOrAdd(ClassName);
		Count++;

		// Skip protected paths
		if (IsProtectedPath(AssetPath, ProtectedPaths))
		{
			continue;
		}

		// Check naming convention
		FString ExpectedPrefix;
		if (!CheckNamingConvention(AssetName, ClassName, ExpectedPrefix) && !ExpectedPrefix.IsEmpty())
		{
			TSharedPtr<FJsonObject> Issue = MakeShareable(new FJsonObject);
			Issue->SetStringField(TEXT("asset_path"), AssetPath);
			Issue->SetStringField(TEXT("asset_name"), AssetName);
			Issue->SetStringField(TEXT("class"), ClassName);
			Issue->SetStringField(TEXT("expected_prefix"), ExpectedPrefix);
			NamingIssues.Add(MakeShareable(new FJsonValueObject(Issue)));
		}

		// Check folder placement
		FString SuggestedFolder = GetSuggestedFolderForClass(ClassName);
		if (!SuggestedFolder.IsEmpty())
		{
			FString CurrentFolder = AssetData.PackagePath.ToString();
			if (!CurrentFolder.Contains(SuggestedFolder))
			{
				TSharedPtr<FJsonObject> Misplaced = MakeShareable(new FJsonObject);
				Misplaced->SetStringField(TEXT("asset_path"), AssetPath);
				Misplaced->SetStringField(TEXT("asset_name"), AssetName);
				Misplaced->SetStringField(TEXT("class"), ClassName);
				Misplaced->SetStringField(TEXT("current_folder"), CurrentFolder);
				Misplaced->SetStringField(TEXT("suggested_folder"), SuggestedFolder);
				MisplacedAssets.Add(MakeShareable(new FJsonValueObject(Misplaced)));
			}
		}
	}

	// Find empty folders
	TArray<TSharedPtr<FJsonValue>> EmptyFolders;
	if (Registry)
	{
		TArray<FString> SubPaths;
		Registry->GetSubPaths(DirectoryPath, SubPaths, true);
		for (const FString& SubPath : SubPaths)
		{
			if (IsProtectedPath(SubPath, ProtectedPaths))
			{
				continue;
			}
			TArray<FString> FolderAssets = UEditorAssetLibrary::ListAssets(SubPath, false, false);
			if (FolderAssets.Num() == 0)
			{
				// Also check if it has subfolders with assets
				TArray<FString> ChildPaths;
				Registry->GetSubPaths(SubPath, ChildPaths, false);
				if (ChildPaths.Num() == 0)
				{
					EmptyFolders.Add(MakeShareable(new FJsonValueString(SubPath)));
				}
			}
		}
	}

	// Build assets_by_class object
	TSharedPtr<FJsonObject> ClassCountsObj = MakeShareable(new FJsonObject);
	for (const auto& Pair : AssetsByClass)
	{
		ClassCountsObj->SetNumberField(Pair.Key, Pair.Value);
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetStringField(TEXT("directory_path"), DirectoryPath);
	Result->SetNumberField(TEXT("total_assets"), TotalAssets);
	Result->SetObjectField(TEXT("assets_by_class"), ClassCountsObj);
	Result->SetArrayField(TEXT("misplaced_assets"), MisplacedAssets);
	Result->SetNumberField(TEXT("misplaced_count"), MisplacedAssets.Num());
	Result->SetArrayField(TEXT("naming_issues"), NamingIssues);
	Result->SetNumberField(TEXT("naming_issue_count"), NamingIssues.Num());
	Result->SetArrayField(TEXT("empty_folders"), EmptyFolders);
	Result->SetNumberField(TEXT("empty_folder_count"), EmptyFolders.Num());
	return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPAssetCommands::HandleBatchMoveAssets(
	const TSharedPtr<FJsonObject>& Params)
{
	if (!Params->HasField(TEXT("moves")))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("moves array is required"));
	}

	bool bDryRun = false;
	if (Params->HasField(TEXT("dry_run")))
	{
		bDryRun = Params->GetBoolField(TEXT("dry_run"));
	}

	TArray<FString> ProtectedPaths = GetProtectedPaths(Params);
	const TArray<TSharedPtr<FJsonValue>>& MovesArray = Params->GetArrayField(TEXT("moves"));

	// Phase 1: Validate all moves
	TArray<TSharedPtr<FJsonValue>> Results;
	int32 TotalMoved = 0;
	int32 TotalFailed = 0;
	bool bAllValid = true;

	struct FMoveEntry
	{
		FString Source;
		FString Destination;
	};
	TArray<FMoveEntry> ValidatedMoves;

	for (const TSharedPtr<FJsonValue>& MoveVal : MovesArray)
	{
		TSharedPtr<FJsonObject> MoveObj = MoveVal->AsObject();
		if (!MoveObj.IsValid())
		{
			continue;
		}

		FString Source = MoveObj->GetStringField(TEXT("source_path"));
		FString Destination = MoveObj->GetStringField(TEXT("destination_path"));

		TSharedPtr<FJsonObject> MoveResult = MakeShareable(new FJsonObject);
		MoveResult->SetStringField(TEXT("source_path"), Source);
		MoveResult->SetStringField(TEXT("destination_path"), Destination);

		// Validate
		TArray<TSharedPtr<FJsonValue>> MoveWarnings;
		bool bValid = true;

		if (Source.IsEmpty() || Destination.IsEmpty())
		{
			MoveWarnings.Add(MakeShareable(new FJsonValueString(TEXT("Missing source or destination path"))));
			bValid = false;
		}

		if (bValid && IsProtectedPath(Source, ProtectedPaths))
		{
			MoveWarnings.Add(MakeShareable(new FJsonValueString(TEXT("Source is in a protected directory"))));
			bValid = false;
		}

		if (bValid && !UEditorAssetLibrary::DoesAssetExist(Source))
		{
			MoveWarnings.Add(MakeShareable(new FJsonValueString(TEXT("Source asset does not exist"))));
			bValid = false;
		}

		if (bValid && UEditorAssetLibrary::DoesAssetExist(Destination))
		{
			MoveWarnings.Add(MakeShareable(new FJsonValueString(TEXT("Destination already exists"))));
			bValid = false;
		}

		MoveResult->SetBoolField(TEXT("valid"), bValid);
		MoveResult->SetArrayField(TEXT("warnings"), MoveWarnings);

		if (bValid)
		{
			ValidatedMoves.Add({ Source, Destination });
		}
		else
		{
			bAllValid = false;
			TotalFailed++;
			MoveResult->SetBoolField(TEXT("moved"), false);
		}

		Results.Add(MakeShareable(new FJsonValueObject(MoveResult)));
	}

	// Phase 2: Execute moves (only if not dry run)
	if (!bDryRun)
	{
		int32 ResultIdx = 0;
		int32 ValidIdx = 0;
		for (int32 i = 0; i < Results.Num(); i++)
		{
			TSharedPtr<FJsonObject> MoveResult = Results[i]->AsObject();
			if (!MoveResult->GetBoolField(TEXT("valid")))
			{
				continue;
			}

			const FMoveEntry& Move = ValidatedMoves[ValidIdx++];
			bool bMoved = UEditorAssetLibrary::RenameAsset(Move.Source, Move.Destination);
			MoveResult->SetBoolField(TEXT("moved"), bMoved);

			if (bMoved)
			{
				TotalMoved++;
			}
			else
			{
				TotalFailed++;
				TArray<TSharedPtr<FJsonValue>> Warnings = MoveResult->GetArrayField(TEXT("warnings"));
				Warnings.Add(MakeShareable(new FJsonValueString(TEXT("Move operation failed"))));
				MoveResult->SetArrayField(TEXT("warnings"), Warnings);
			}
		}
	}
	else
	{
		TotalMoved = ValidatedMoves.Num();
	}

	TSharedPtr<FJsonObject> Result = MakeShareable(new FJsonObject);
	Result->SetBoolField(TEXT("success"), true);
	Result->SetBoolField(TEXT("dry_run"), bDryRun);
	Result->SetArrayField(TEXT("results"), Results);
	Result->SetNumberField(TEXT("total_moved"), TotalMoved);
	Result->SetNumberField(TEXT("total_failed"), TotalFailed);
	Result->SetNumberField(TEXT("total_requested"), MovesArray.Num());
	return Result;
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPAssetCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// Discovery
	Registry.RegisterCommand(TEXT("list_assets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_assets"), P); });
	Registry.RegisterCommand(TEXT("get_asset_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_asset_info"), P); });
	Registry.RegisterCommand(TEXT("find_asset_references"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("find_asset_references"), P); });
	Registry.RegisterCommand(TEXT("get_folder_structure"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_folder_structure"), P); });
	Registry.RegisterCommand(TEXT("get_asset_class"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_asset_class"), P); });
	// Organization
	Registry.RegisterCommand(TEXT("rename_asset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("rename_asset"), P); });
	Registry.RegisterCommand(TEXT("move_asset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("move_asset"), P); });
	Registry.RegisterCommand(TEXT("move_directory"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("move_directory"), P); });
	Registry.RegisterCommand(TEXT("make_directory"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("make_directory"), P); });
	Registry.RegisterCommand(TEXT("duplicate_asset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("duplicate_asset"), P); });
	Registry.RegisterCommand(TEXT("save_asset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("save_asset"), P); });
	Registry.RegisterCommand(TEXT("save_directory"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("save_directory"), P); });
	// Safety
	Registry.RegisterCommand(TEXT("validate_asset_move"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("validate_asset_move"), P); });
	Registry.RegisterCommand(TEXT("fix_redirectors"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("fix_redirectors"), P); });
	Registry.RegisterCommand(TEXT("is_protected_path"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("is_protected_path"), P); });
	// Analysis / Batch
	Registry.RegisterCommand(TEXT("analyze_folder_organization"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("analyze_folder_organization"), P); });
	Registry.RegisterCommand(TEXT("batch_move_assets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("batch_move_assets"), P); });
}
