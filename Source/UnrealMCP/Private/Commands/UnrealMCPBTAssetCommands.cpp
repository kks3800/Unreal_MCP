// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPBTAssetCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "Subsystems/AssetEditorSubsystem.h"

// Behavior Tree / Blackboard includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BlackboardData.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Bool.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Float.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Int.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_String.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Class.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Enum.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Name.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Rotator.h"
#include "BehaviorTree/BTCompositeNode.h"

// Editor factories
#include "BehaviorTreeFactory.h"
#include "BlackboardDataFactory.h"

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// BT Asset Commands
	if (CommandType == TEXT("create_behavior_tree"))
	{
		return HandleCreateBehaviorTree(Params);
	}
	else if (CommandType == TEXT("delete_behavior_tree"))
	{
		return HandleDeleteBehaviorTree(Params);
	}
	else if (CommandType == TEXT("list_behavior_trees"))
	{
		return HandleListBehaviorTrees(Params);
	}
	else if (CommandType == TEXT("get_behavior_tree_info"))
	{
		return HandleGetBehaviorTreeInfo(Params);
	}
	else if (CommandType == TEXT("save_behavior_tree"))
	{
		return HandleSaveBehaviorTree(Params);
	}
	else if (CommandType == TEXT("open_behavior_tree"))
	{
		return HandleOpenBehaviorTree(Params);
	}
	else if (CommandType == TEXT("set_behavior_tree_blackboard"))
	{
		return HandleSetBehaviorTreeBlackboard(Params);
	}
	// Blackboard Commands
	else if (CommandType == TEXT("create_blackboard"))
	{
		return HandleCreateBlackboard(Params);
	}
	else if (CommandType == TEXT("delete_blackboard"))
	{
		return HandleDeleteBlackboard(Params);
	}
	else if (CommandType == TEXT("list_blackboards"))
	{
		return HandleListBlackboards(Params);
	}
	else if (CommandType == TEXT("get_blackboard_info"))
	{
		return HandleGetBlackboardInfo(Params);
	}
	else if (CommandType == TEXT("save_blackboard"))
	{
		return HandleSaveBlackboard(Params);
	}
	else if (CommandType == TEXT("add_blackboard_key"))
	{
		return HandleAddBlackboardKey(Params);
	}
	else if (CommandType == TEXT("remove_blackboard_key"))
	{
		return HandleRemoveBlackboardKey(Params);
	}
	else if (CommandType == TEXT("modify_blackboard_key"))
	{
		return HandleModifyBlackboardKey(Params);
	}

	return CreateErrorResponse(FString::Printf(TEXT("Unknown BT/BB asset command: %s"), *CommandType));
}

//=============================================================================
// Helper Methods
//=============================================================================

UBehaviorTree* FUnrealMCPBTAssetCommands::LoadBehaviorTree(const FString& TreeName, FString& OutPath)
{
	// Check cache first
	if (TWeakObjectPtr<UBehaviorTree>* Cached = ActiveTrees.Find(TreeName))
	{
		if (Cached->IsValid())
		{
			UBehaviorTree* BT = Cached->Get();
			OutPath = BT->GetPathName();
			return BT;
		}
	}

	// Try direct path
	FString AssetPath = TreeName;
	if (!AssetPath.StartsWith(TEXT("/")))
	{
		AssetPath = FString::Printf(TEXT("/Game/AI/%s.%s"), *TreeName, *TreeName);
	}

	UBehaviorTree* BT = Cast<UBehaviorTree>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!BT)
	{
		// Search asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssetsByClass(UBehaviorTree::StaticClass()->GetClassPathName(), AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			if (Asset.AssetName.ToString() == TreeName)
			{
				BT = Cast<UBehaviorTree>(Asset.GetAsset());
				if (BT)
				{
					OutPath = Asset.GetObjectPathString();
					break;
				}
			}
		}
	}

	if (BT)
	{
		OutPath = BT->GetPathName();
		ActiveTrees.Add(TreeName, BT);
	}

	return BT;
}

UBlackboardData* FUnrealMCPBTAssetCommands::LoadBlackboardData(const FString& BBName, FString& OutPath)
{
	// Check cache first
	if (TWeakObjectPtr<UBlackboardData>* Cached = ActiveBlackboards.Find(BBName))
	{
		if (Cached->IsValid())
		{
			UBlackboardData* BB = Cached->Get();
			OutPath = BB->GetPathName();
			return BB;
		}
	}

	// Try direct path
	FString AssetPath = BBName;
	if (!AssetPath.StartsWith(TEXT("/")))
	{
		AssetPath = FString::Printf(TEXT("/Game/AI/%s.%s"), *BBName, *BBName);
	}

	UBlackboardData* BB = Cast<UBlackboardData>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!BB)
	{
		// Search asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssetsByClass(UBlackboardData::StaticClass()->GetClassPathName(), AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			if (Asset.AssetName.ToString() == BBName)
			{
				BB = Cast<UBlackboardData>(Asset.GetAsset());
				if (BB)
				{
					OutPath = Asset.GetObjectPathString();
					break;
				}
			}
		}
	}

	if (BB)
	{
		OutPath = BB->GetPathName();
		ActiveBlackboards.Add(BBName, BB);
	}

	return BB;
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
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

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

//=============================================================================
// BT Asset Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleCreateBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString Path = TEXT("/Game/AI");
	Params->TryGetStringField(TEXT("path"), Path);

	// Create the package
	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *TreeName);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}

	// Use the factory to create the BT asset
	UBehaviorTreeFactory* Factory = NewObject<UBehaviorTreeFactory>();
	UBehaviorTree* NewBT = Cast<UBehaviorTree>(Factory->FactoryCreateNew(
		UBehaviorTree::StaticClass(), Package, FName(*TreeName),
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!NewBT)
	{
		return CreateErrorResponse(TEXT("Failed to create Behavior Tree asset"));
	}

	// Optionally associate a blackboard
	FString BlackboardAssetName;
	if (Params->TryGetStringField(TEXT("blackboard_asset"), BlackboardAssetName) && !BlackboardAssetName.IsEmpty())
	{
		FString BBPath;
		UBlackboardData* BBData = LoadBlackboardData(BlackboardAssetName, BBPath);
		if (BBData)
		{
			NewBT->BlackboardAsset = BBData;
		}
	}

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(NewBT);
	NewBT->MarkPackageDirty();

	// Cache it
	ActiveTrees.Add(TreeName, NewBT);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("name"), TreeName);
	Data->SetStringField(TEXT("path"), PackagePath);
	Data->SetBoolField(TEXT("has_blackboard"), NewBT->BlackboardAsset != nullptr);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleDeleteBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	// Get the package path for deletion
	FString PackagePath = BT->GetOutermost()->GetName();
	FString ObjectPath = PackagePath + TEXT(".") + BT->GetName();

	// Remove from cache
	ActiveTrees.Remove(TreeName);

	if (!UEditorAssetLibrary::DeleteAsset(ObjectPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to delete: %s"), *ObjectPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("deleted"), TreeName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleListBehaviorTrees(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Path = TEXT("/Game");
	Params->TryGetStringField(TEXT("path"), Path);
	bool bRecursive = true;
	Params->TryGetBoolField(TEXT("recursive"), bRecursive);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UBehaviorTree::StaticClass()->GetClassPathName(), AssetList);

	TArray<TSharedPtr<FJsonValue>> TreeArray;
	for (const FAssetData& Asset : AssetList)
	{
		FString AssetPathStr = Asset.GetObjectPathString();
		if (!bRecursive)
		{
			FString AssetDir = FPackageName::GetLongPackagePath(Asset.PackageName.ToString());
			if (AssetDir != Path)
			{
				continue;
			}
		}
		else if (!AssetPathStr.StartsWith(Path))
		{
			continue;
		}

		TSharedPtr<FJsonObject> TreeInfo = MakeShared<FJsonObject>();
		TreeInfo->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		TreeInfo->SetStringField(TEXT("path"), AssetPathStr);
		TreeArray.Add(MakeShared<FJsonValueObject>(TreeInfo));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetArrayField(TEXT("behavior_trees"), TreeArray);
	Data->SetNumberField(TEXT("count"), TreeArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleGetBehaviorTreeInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("name"), BT->GetName());
	Data->SetStringField(TEXT("path"), AssetPath);
	Data->SetBoolField(TEXT("has_root_node"), BT->RootNode != nullptr);
	Data->SetBoolField(TEXT("has_blackboard"), BT->BlackboardAsset != nullptr);

	if (BT->BlackboardAsset)
	{
		Data->SetStringField(TEXT("blackboard_name"), BT->BlackboardAsset->GetName());
		Data->SetStringField(TEXT("blackboard_path"), BT->BlackboardAsset->GetPathName());
	}

	// Count nodes in the graph
#if WITH_EDITORONLY_DATA
	if (BT->BTGraph)
	{
		Data->SetNumberField(TEXT("graph_node_count"), BT->BTGraph->Nodes.Num());
	}
#endif

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleSaveBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UPackage* Package = BT->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, BT, *PackageFileName, SaveArgs);

	if (!bSaved)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to save: %s"), *TreeName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("saved"), TreeName);
	Data->SetStringField(TEXT("file"), PackageFileName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleOpenBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (AssetEditorSubsystem)
	{
		AssetEditorSubsystem->OpenEditorForAsset(BT);
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("opened"), TreeName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleSetBehaviorTreeBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString BlackboardName;
	if (!Params->TryGetStringField(TEXT("blackboard_name"), BlackboardName))
	{
		return CreateErrorResponse(TEXT("Missing 'blackboard_name' parameter"));
	}

	FString TreePath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, TreePath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	FString BBPath;
	UBlackboardData* BBData = LoadBlackboardData(BlackboardName, BBPath);
	if (!BBData)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard not found: %s"), *BlackboardName));
	}

	BT->BlackboardAsset = BBData;
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("blackboard"), BlackboardName);
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Blackboard Asset Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleCreateBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return CreateErrorResponse(TEXT("Missing 'bb_name' parameter"));
	}

	FString Path = TEXT("/Game/AI");
	Params->TryGetStringField(TEXT("path"), Path);

	// Create the package
	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *BBName);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}

	// Use the factory to create the BB asset
	UBlackboardDataFactory* Factory = NewObject<UBlackboardDataFactory>();
	UBlackboardData* NewBB = Cast<UBlackboardData>(Factory->FactoryCreateNew(
		UBlackboardData::StaticClass(), Package, FName(*BBName),
		RF_Public | RF_Standalone, nullptr, GWarn));

	if (!NewBB)
	{
		return CreateErrorResponse(TEXT("Failed to create Blackboard Data asset"));
	}

	// Optionally set parent blackboard
	FString ParentBBName;
	if (Params->TryGetStringField(TEXT("parent_bb"), ParentBBName) && !ParentBBName.IsEmpty())
	{
		FString ParentPath;
		UBlackboardData* ParentBB = LoadBlackboardData(ParentBBName, ParentPath);
		if (ParentBB)
		{
			NewBB->Parent = ParentBB;
		}
	}

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(NewBB);
	NewBB->MarkPackageDirty();

	// Cache it
	ActiveBlackboards.Add(BBName, NewBB);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("name"), BBName);
	Data->SetStringField(TEXT("path"), PackagePath);
	Data->SetBoolField(TEXT("has_parent"), NewBB->Parent != nullptr);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleDeleteBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return CreateErrorResponse(TEXT("Missing 'bb_name' parameter"));
	}

	FString AssetPath;
	UBlackboardData* BB = LoadBlackboardData(BBName, AssetPath);
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard not found: %s"), *BBName));
	}

	FString PackagePath = BB->GetOutermost()->GetName();
	FString ObjectPath = PackagePath + TEXT(".") + BB->GetName();

	ActiveBlackboards.Remove(BBName);

	if (!UEditorAssetLibrary::DeleteAsset(ObjectPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to delete: %s"), *ObjectPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("deleted"), BBName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleListBlackboards(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Path = TEXT("/Game");
	Params->TryGetStringField(TEXT("path"), Path);
	bool bRecursive = true;
	Params->TryGetBoolField(TEXT("recursive"), bRecursive);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UBlackboardData::StaticClass()->GetClassPathName(), AssetList);

	TArray<TSharedPtr<FJsonValue>> BBArray;
	for (const FAssetData& Asset : AssetList)
	{
		FString AssetPathStr = Asset.GetObjectPathString();
		if (!bRecursive)
		{
			FString AssetDir = FPackageName::GetLongPackagePath(Asset.PackageName.ToString());
			if (AssetDir != Path)
			{
				continue;
			}
		}
		else if (!AssetPathStr.StartsWith(Path))
		{
			continue;
		}

		TSharedPtr<FJsonObject> BBInfo = MakeShared<FJsonObject>();
		BBInfo->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		BBInfo->SetStringField(TEXT("path"), AssetPathStr);
		BBArray.Add(MakeShared<FJsonValueObject>(BBInfo));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetArrayField(TEXT("blackboards"), BBArray);
	Data->SetNumberField(TEXT("count"), BBArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleGetBlackboardInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return CreateErrorResponse(TEXT("Missing 'bb_name' parameter"));
	}

	FString AssetPath;
	UBlackboardData* BB = LoadBlackboardData(BBName, AssetPath);
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard not found: %s"), *BBName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("name"), BB->GetName());
	Data->SetStringField(TEXT("path"), AssetPath);
	Data->SetBoolField(TEXT("has_parent"), BB->Parent != nullptr);
	if (BB->Parent)
	{
		Data->SetStringField(TEXT("parent_name"), BB->Parent->GetName());
	}

	// List all keys
	TArray<TSharedPtr<FJsonValue>> KeyArray;
	const TArray<FBlackboardEntry>& Keys = BB->Keys;
	for (int32 i = 0; i < Keys.Num(); ++i)
	{
		const FBlackboardEntry& Entry = Keys[i];
		TSharedPtr<FJsonObject> KeyInfo = MakeShared<FJsonObject>();
		KeyInfo->SetStringField(TEXT("name"), Entry.EntryName.ToString());
		KeyInfo->SetNumberField(TEXT("index"), i);
		KeyInfo->SetBoolField(TEXT("instance_synced"), Entry.bInstanceSynced != 0);

		if (Entry.KeyType)
		{
			KeyInfo->SetStringField(TEXT("type"), Entry.KeyType->GetClass()->GetName());
		}

#if WITH_EDITORONLY_DATA
		if (!Entry.EntryDescription.IsEmpty())
		{
			KeyInfo->SetStringField(TEXT("description"), Entry.EntryDescription);
		}
#endif

		KeyArray.Add(MakeShared<FJsonValueObject>(KeyInfo));
	}

	Data->SetArrayField(TEXT("keys"), KeyArray);
	Data->SetNumberField(TEXT("key_count"), Keys.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleSaveBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return CreateErrorResponse(TEXT("Missing 'bb_name' parameter"));
	}

	FString AssetPath;
	UBlackboardData* BB = LoadBlackboardData(BBName, AssetPath);
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard not found: %s"), *BBName));
	}

	UPackage* Package = BB->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, BB, *PackageFileName, SaveArgs);

	if (!bSaved)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to save: %s"), *BBName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("saved"), BBName);
	Data->SetStringField(TEXT("file"), PackageFileName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleAddBlackboardKey(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return CreateErrorResponse(TEXT("Missing 'bb_name' parameter"));
	}

	FString KeyName;
	if (!Params->TryGetStringField(TEXT("key_name"), KeyName))
	{
		return CreateErrorResponse(TEXT("Missing 'key_name' parameter"));
	}

	FString KeyType = TEXT("Bool");
	Params->TryGetStringField(TEXT("key_type"), KeyType);

	FString AssetPath;
	UBlackboardData* BB = LoadBlackboardData(BBName, AssetPath);
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard not found: %s"), *BBName));
	}

	// Check if key already exists
	for (const FBlackboardEntry& Existing : BB->Keys)
	{
		if (Existing.EntryName == FName(*KeyName))
		{
			return CreateErrorResponse(FString::Printf(TEXT("Key '%s' already exists"), *KeyName));
		}
	}

	// Create the new entry
	FBlackboardEntry NewEntry;
	NewEntry.EntryName = FName(*KeyName);

	// Create the appropriate key type
	if (KeyType == TEXT("Bool"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_Bool>(BB);
	}
	else if (KeyType == TEXT("Float"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_Float>(BB);
	}
	else if (KeyType == TEXT("Int"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_Int>(BB);
	}
	else if (KeyType == TEXT("Vector"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_Vector>(BB);
	}
	else if (KeyType == TEXT("Object"))
	{
		UBlackboardKeyType_Object* ObjKey = NewObject<UBlackboardKeyType_Object>(BB);
		ObjKey->BaseClass = AActor::StaticClass();
		NewEntry.KeyType = ObjKey;
	}
	else if (KeyType == TEXT("String"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_String>(BB);
	}
	else if (KeyType == TEXT("Class"))
	{
		UBlackboardKeyType_Class* ClassKey = NewObject<UBlackboardKeyType_Class>(BB);
		ClassKey->BaseClass = UObject::StaticClass();
		NewEntry.KeyType = ClassKey;
	}
	else if (KeyType == TEXT("Enum"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_Enum>(BB);
	}
	else if (KeyType == TEXT("Name"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_Name>(BB);
	}
	else if (KeyType == TEXT("Rotator"))
	{
		NewEntry.KeyType = NewObject<UBlackboardKeyType_Rotator>(BB);
	}
	else
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Unknown key type: %s. Supported: Bool, Float, Int, Vector, Object, String, Class, Enum, Name, Rotator"),
			*KeyType));
	}

	// Set optional properties
	bool bInstanceSynced = false;
	if (Params->TryGetBoolField(TEXT("instance_synced"), bInstanceSynced))
	{
		NewEntry.bInstanceSynced = bInstanceSynced;
	}

#if WITH_EDITORONLY_DATA
	FString Description;
	if (Params->TryGetStringField(TEXT("description"), Description))
	{
		NewEntry.EntryDescription = Description;
	}
#endif

	BB->Keys.Add(NewEntry);
	BB->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("blackboard"), BBName);
	Data->SetStringField(TEXT("key_name"), KeyName);
	Data->SetStringField(TEXT("key_type"), KeyType);
	Data->SetNumberField(TEXT("key_index"), BB->Keys.Num() - 1);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleRemoveBlackboardKey(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return CreateErrorResponse(TEXT("Missing 'bb_name' parameter"));
	}

	FString KeyName;
	if (!Params->TryGetStringField(TEXT("key_name"), KeyName))
	{
		return CreateErrorResponse(TEXT("Missing 'key_name' parameter"));
	}

	FString AssetPath;
	UBlackboardData* BB = LoadBlackboardData(BBName, AssetPath);
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard not found: %s"), *BBName));
	}

	// Find and remove the key
	int32 RemovedIndex = INDEX_NONE;
	for (int32 i = 0; i < BB->Keys.Num(); ++i)
	{
		if (BB->Keys[i].EntryName == FName(*KeyName))
		{
			RemovedIndex = i;
			BB->Keys.RemoveAt(i);
			break;
		}
	}

	if (RemovedIndex == INDEX_NONE)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Key '%s' not found in blackboard '%s'"), *KeyName, *BBName));
	}

	BB->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("blackboard"), BBName);
	Data->SetStringField(TEXT("removed_key"), KeyName);
	Data->SetNumberField(TEXT("remaining_keys"), BB->Keys.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTAssetCommands::HandleModifyBlackboardKey(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BBName;
	if (!Params->TryGetStringField(TEXT("bb_name"), BBName))
	{
		return CreateErrorResponse(TEXT("Missing 'bb_name' parameter"));
	}

	FString KeyName;
	if (!Params->TryGetStringField(TEXT("key_name"), KeyName))
	{
		return CreateErrorResponse(TEXT("Missing 'key_name' parameter"));
	}

	FString AssetPath;
	UBlackboardData* BB = LoadBlackboardData(BBName, AssetPath);
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard not found: %s"), *BBName));
	}

	// Find the key
	FBlackboardEntry* FoundEntry = nullptr;
	for (FBlackboardEntry& Entry : BB->Keys)
	{
		if (Entry.EntryName == FName(*KeyName))
		{
			FoundEntry = &Entry;
			break;
		}
	}

	if (!FoundEntry)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Key '%s' not found in blackboard '%s'"), *KeyName, *BBName));
	}

	// Apply modifications
	FString NewName;
	if (Params->TryGetStringField(TEXT("new_name"), NewName) && !NewName.IsEmpty())
	{
		FoundEntry->EntryName = FName(*NewName);
	}

#if WITH_EDITORONLY_DATA
	FString Description;
	if (Params->TryGetStringField(TEXT("description"), Description))
	{
		FoundEntry->EntryDescription = Description;
	}
#endif

	bool bInstanceSynced;
	if (Params->TryGetBoolField(TEXT("instance_synced"), bInstanceSynced))
	{
		FoundEntry->bInstanceSynced = bInstanceSynced;
	}

	BB->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("blackboard"), BBName);
	Data->SetStringField(TEXT("key_name"), FoundEntry->EntryName.ToString());
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Command Registration
//=============================================================================

void FUnrealMCPBTAssetCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// BT Asset Commands
	Registry.RegisterCommand(TEXT("create_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("delete_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("list_behavior_trees"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_behavior_trees"), P); });
	Registry.RegisterCommand(TEXT("get_behavior_tree_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_behavior_tree_info"), P); });
	Registry.RegisterCommand(TEXT("save_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("save_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("open_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("open_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("set_behavior_tree_blackboard"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_behavior_tree_blackboard"), P); });

	// Blackboard Commands
	Registry.RegisterCommand(TEXT("create_blackboard"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_blackboard"), P); });
	Registry.RegisterCommand(TEXT("delete_blackboard"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_blackboard"), P); });
	Registry.RegisterCommand(TEXT("list_blackboards"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_blackboards"), P); });
	Registry.RegisterCommand(TEXT("get_blackboard_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blackboard_info"), P); });
	Registry.RegisterCommand(TEXT("save_blackboard"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("save_blackboard"), P); });
	Registry.RegisterCommand(TEXT("add_blackboard_key"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blackboard_key"), P); });
	Registry.RegisterCommand(TEXT("remove_blackboard_key"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_blackboard_key"), P); });
	Registry.RegisterCommand(TEXT("modify_blackboard_key"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("modify_blackboard_key"), P); });
}
