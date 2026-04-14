// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPBTRuntimeCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"

// AI includes
#include "AIController.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
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
#include "BehaviorTree/BTNode.h"

// Editor / asset includes
#include "Editor.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EngineUtils.h"

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// BT Runtime Control
	if (CommandType == TEXT("run_behavior_tree"))
	{
		return HandleRunBehaviorTree(Params);
	}
	else if (CommandType == TEXT("stop_behavior_tree"))
	{
		return HandleStopBehaviorTree(Params);
	}
	else if (CommandType == TEXT("pause_behavior_tree"))
	{
		return HandlePauseBehaviorTree(Params);
	}
	else if (CommandType == TEXT("resume_behavior_tree"))
	{
		return HandleResumeBehaviorTree(Params);
	}
	else if (CommandType == TEXT("restart_behavior_tree"))
	{
		return HandleRestartBehaviorTree(Params);
	}
	else if (CommandType == TEXT("get_bt_runtime_state"))
	{
		return HandleGetBTRuntimeState(Params);
	}
	// Blackboard Value Commands
	else if (CommandType == TEXT("set_blackboard_value"))
	{
		return HandleSetBlackboardValue(Params);
	}
	else if (CommandType == TEXT("get_blackboard_value"))
	{
		return HandleGetBlackboardValue(Params);
	}
	else if (CommandType == TEXT("get_blackboard_state"))
	{
		return HandleGetBlackboardState(Params);
	}
	else if (CommandType == TEXT("clear_blackboard_value"))
	{
		return HandleClearBlackboardValue(Params);
	}

	return CreateErrorResponse(FString::Printf(TEXT("Unknown BT runtime command: %s"), *CommandType));
}

//=============================================================================
// Helper Methods
//=============================================================================

UWorld* FUnrealMCPBTRuntimeCommands::GetPIEWorld()
{
	if (!GEditor)
	{
		return nullptr;
	}

	FWorldContext* PIEContext = GEditor->GetPIEWorldContext();
	if (!PIEContext)
	{
		return nullptr;
	}

	return PIEContext->World();
}

AAIController* FUnrealMCPBTRuntimeCommands::FindAIControllerForActor(UWorld* World, const FString& ActorName)
{
	if (!World)
	{
		return nullptr;
	}

	// Search pawns first - most common case
	for (TActorIterator<APawn> It(World); It; ++It)
	{
		APawn* Pawn = *It;
		if (Pawn->GetName() == ActorName || Pawn->GetActorLabel() == ActorName)
		{
			AAIController* AIC = Cast<AAIController>(Pawn->GetController());
			if (AIC)
			{
				return AIC;
			}
		}
	}

	// Also try finding by AAIController name directly
	for (TActorIterator<AAIController> It(World); It; ++It)
	{
		AAIController* AIC = *It;
		if (AIC->GetName() == ActorName || AIC->GetActorLabel() == ActorName)
		{
			return AIC;
		}
	}

	return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
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

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

//=============================================================================
// BT Runtime Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleRunBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	// Load the BT asset - try direct path first, then search asset registry
	FString AssetPath = TreeName;
	if (!AssetPath.StartsWith(TEXT("/")))
	{
		AssetPath = FString::Printf(TEXT("/Game/AI/%s.%s"), *TreeName, *TreeName);
	}

	UBehaviorTree* BT = Cast<UBehaviorTree>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!BT)
	{
		// Search asset registry by name
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
					break;
				}
			}
		}
	}

	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	bool bSuccess = AIC->RunBehaviorTree(BT);
	if (!bSuccess)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to run Behavior Tree '%s' on '%s'"), *TreeName, *ActorName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("tree"), BT->GetName());
	Data->SetStringField(TEXT("tree_path"), BT->GetPathName());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleStopBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
	if (!BTComp)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BehaviorTreeComponent found on AI Controller for: %s"), *ActorName));
	}

	FString StopModeStr = TEXT("Safe");
	Params->TryGetStringField(TEXT("stop_mode"), StopModeStr);

	EBTStopMode::Type StopMode = (StopModeStr == TEXT("Forced"))
		? EBTStopMode::Forced
		: EBTStopMode::Safe;

	BTComp->StopTree(StopMode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("stop_mode"), StopModeStr);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandlePauseBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
	if (!BTComp)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BehaviorTreeComponent found on AI Controller for: %s"), *ActorName));
	}

	FString Reason = TEXT("MCP");
	Params->TryGetStringField(TEXT("reason"), Reason);

	BTComp->PauseLogic(Reason);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("reason"), Reason);
	Data->SetBoolField(TEXT("paused"), true);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleResumeBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
	if (!BTComp)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BehaviorTreeComponent found on AI Controller for: %s"), *ActorName));
	}

	FString Reason = TEXT("MCP");
	Params->TryGetStringField(TEXT("reason"), Reason);

	BTComp->ResumeLogic(Reason);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("reason"), Reason);
	Data->SetBoolField(TEXT("paused"), false);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleRestartBehaviorTree(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
	if (!BTComp)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BehaviorTreeComponent found on AI Controller for: %s"), *ActorName));
	}

	BTComp->RestartTree();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetBoolField(TEXT("restarted"), true);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleGetBTRuntimeState(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBehaviorTreeComponent* BTComp = Cast<UBehaviorTreeComponent>(AIC->GetBrainComponent());
	if (!BTComp)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BehaviorTreeComponent found on AI Controller for: %s"), *ActorName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);

	// Current tree info
	UBehaviorTree* CurrentTree = BTComp->GetCurrentTree();
	if (CurrentTree)
	{
		Data->SetStringField(TEXT("current_tree"), CurrentTree->GetName());
		Data->SetStringField(TEXT("current_tree_path"), CurrentTree->GetPathName());
	}
	else
	{
		Data->SetStringField(TEXT("current_tree"), TEXT("None"));
	}

	// Running state
	bool bIsRunning = BTComp->IsRunning();
	bool bIsPaused = BTComp->IsPaused();
	Data->SetBoolField(TEXT("is_running"), bIsRunning);
	Data->SetBoolField(TEXT("is_paused"), bIsPaused);

	// Active node info
	if (bIsRunning)
	{
		const UBTNode* ActiveNode = BTComp->GetActiveNode();
		if (ActiveNode)
		{
			Data->SetStringField(TEXT("active_node"), ActiveNode->GetNodeName());
			Data->SetStringField(TEXT("active_node_class"), ActiveNode->GetClass()->GetName());
			Data->SetNumberField(TEXT("active_node_index"), ActiveNode->GetExecutionIndex());
		}
	}

	// Blackboard info
	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	Data->SetBoolField(TEXT("has_blackboard"), BB != nullptr);
	if (BB && BB->GetBlackboardAsset())
	{
		Data->SetStringField(TEXT("blackboard_asset"), BB->GetBlackboardAsset()->GetName());
	}

	return CreateSuccessResponse(Data);
}

//=============================================================================
// Blackboard Value Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleSetBlackboardValue(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString KeyName;
	if (!Params->TryGetStringField(TEXT("key_name"), KeyName))
	{
		return CreateErrorResponse(TEXT("Missing 'key_name' parameter"));
	}

	FString ValueType;
	if (!Params->TryGetStringField(TEXT("value_type"), ValueType))
	{
		return CreateErrorResponse(TEXT("Missing 'value_type' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BlackboardComponent found for actor: %s"), *ActorName));
	}

	FName BBKeyName(*KeyName);

	if (ValueType == TEXT("Bool"))
	{
		bool bValue = false;
		if (!Params->TryGetBoolField(TEXT("value"), bValue))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for Bool type"));
		}
		BB->SetValueAsBool(BBKeyName, bValue);
	}
	else if (ValueType == TEXT("Float"))
	{
		double Value = 0.0;
		if (!Params->TryGetNumberField(TEXT("value"), Value))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for Float type"));
		}
		BB->SetValueAsFloat(BBKeyName, static_cast<float>(Value));
	}
	else if (ValueType == TEXT("Int"))
	{
		double Value = 0.0;
		if (!Params->TryGetNumberField(TEXT("value"), Value))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for Int type"));
		}
		BB->SetValueAsInt(BBKeyName, static_cast<int32>(Value));
	}
	else if (ValueType == TEXT("Vector"))
	{
		const TArray<TSharedPtr<FJsonValue>>* VecArray = nullptr;
		if (Params->TryGetArrayField(TEXT("value"), VecArray) && VecArray->Num() == 3)
		{
			FVector Vec(
				(*VecArray)[0]->AsNumber(),
				(*VecArray)[1]->AsNumber(),
				(*VecArray)[2]->AsNumber()
			);
			BB->SetValueAsVector(BBKeyName, Vec);
		}
		else
		{
			return CreateErrorResponse(TEXT("'value' for Vector type must be a JSON array of 3 numbers [x, y, z]"));
		}
	}
	else if (ValueType == TEXT("String"))
	{
		FString Value;
		if (!Params->TryGetStringField(TEXT("value"), Value))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for String type"));
		}
		BB->SetValueAsString(BBKeyName, Value);
	}
	else if (ValueType == TEXT("Object"))
	{
		FString ObjName;
		if (!Params->TryGetStringField(TEXT("value"), ObjName))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for Object type (expected actor name string)"));
		}

		bool bFound = false;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->GetName() == ObjName || Actor->GetActorLabel() == ObjName)
			{
				BB->SetValueAsObject(BBKeyName, Actor);
				bFound = true;
				break;
			}
		}

		if (!bFound)
		{
			return CreateErrorResponse(FString::Printf(TEXT("Actor not found in PIE world: %s"), *ObjName));
		}
	}
	else if (ValueType == TEXT("Name"))
	{
		FString Value;
		if (!Params->TryGetStringField(TEXT("value"), Value))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for Name type"));
		}
		BB->SetValueAsName(BBKeyName, FName(*Value));
	}
	else if (ValueType == TEXT("Rotator"))
	{
		const TArray<TSharedPtr<FJsonValue>>* RotArray = nullptr;
		if (Params->TryGetArrayField(TEXT("value"), RotArray) && RotArray->Num() == 3)
		{
			FRotator Rot(
				(*RotArray)[0]->AsNumber(),
				(*RotArray)[1]->AsNumber(),
				(*RotArray)[2]->AsNumber()
			);
			BB->SetValueAsRotator(BBKeyName, Rot);
		}
		else
		{
			return CreateErrorResponse(TEXT("'value' for Rotator type must be a JSON array of 3 numbers [pitch, yaw, roll]"));
		}
	}
	else if (ValueType == TEXT("Enum"))
	{
		double Value = 0.0;
		if (!Params->TryGetNumberField(TEXT("value"), Value))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for Enum type (expected number)"));
		}
		BB->SetValueAsEnum(BBKeyName, static_cast<uint8>(Value));
	}
	else if (ValueType == TEXT("Class"))
	{
		FString ClassName;
		if (!Params->TryGetStringField(TEXT("value"), ClassName))
		{
			return CreateErrorResponse(TEXT("Missing or invalid 'value' for Class type (expected class path string)"));
		}
		UClass* FoundClass = FindObject<UClass>(nullptr, *ClassName);
		if (!FoundClass)
		{
			FoundClass = LoadObject<UClass>(nullptr, *ClassName);
		}
		if (!FoundClass)
		{
			return CreateErrorResponse(FString::Printf(TEXT("Class not found: %s"), *ClassName));
		}
		BB->SetValueAsClass(BBKeyName, FoundClass);
	}
	else
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Unknown value_type: %s. Supported: Bool, Float, Int, Vector, String, Object, Name, Rotator, Enum, Class"),
			*ValueType));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("key_name"), KeyName);
	Data->SetStringField(TEXT("value_type"), ValueType);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleGetBlackboardValue(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString KeyName;
	if (!Params->TryGetStringField(TEXT("key_name"), KeyName))
	{
		return CreateErrorResponse(TEXT("Missing 'key_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BlackboardComponent found for actor: %s"), *ActorName));
	}

	UBlackboardData* BBData = BB->GetBlackboardAsset();
	if (!BBData)
	{
		return CreateErrorResponse(TEXT("No BlackboardData asset associated with BlackboardComponent"));
	}

	// Find the key entry to determine its type
	FName BBKeyName(*KeyName);
	FBlackboard::FKey KeyID = BB->GetKeyID(BBKeyName);
	if (KeyID == FBlackboard::InvalidKey)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard key not found: %s"), *KeyName));
	}

	const FBlackboardEntry* FoundEntry = nullptr;
	for (const FBlackboardEntry& Entry : BBData->Keys)
	{
		if (Entry.EntryName == BBKeyName)
		{
			FoundEntry = &Entry;
			break;
		}
	}

	// Also check parent blackboard keys
	if (!FoundEntry && BBData->Parent)
	{
		for (const FBlackboardEntry& Entry : BBData->Parent->Keys)
		{
			if (Entry.EntryName == BBKeyName)
			{
				FoundEntry = &Entry;
				break;
			}
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("key_name"), KeyName);

	if (!FoundEntry || !FoundEntry->KeyType)
	{
		// Key exists but we cannot determine type - return raw string representation
		Data->SetStringField(TEXT("value_type"), TEXT("Unknown"));
		Data->SetStringField(TEXT("value"), BB->DescribeKeyValue(KeyID, EBlackboardDescription::Full));
		return CreateSuccessResponse(Data);
	}

	// Read value based on key type
	if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Bool>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Bool"));
		Data->SetBoolField(TEXT("value"), BB->GetValueAsBool(BBKeyName));
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Float>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Float"));
		Data->SetNumberField(TEXT("value"), BB->GetValueAsFloat(BBKeyName));
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Int>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Int"));
		Data->SetNumberField(TEXT("value"), BB->GetValueAsInt(BBKeyName));
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Vector>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Vector"));
		FVector Vec = BB->GetValueAsVector(BBKeyName);
		TArray<TSharedPtr<FJsonValue>> VecArray;
		VecArray.Add(MakeShared<FJsonValueNumber>(Vec.X));
		VecArray.Add(MakeShared<FJsonValueNumber>(Vec.Y));
		VecArray.Add(MakeShared<FJsonValueNumber>(Vec.Z));
		Data->SetArrayField(TEXT("value"), VecArray);
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_String>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("String"));
		Data->SetStringField(TEXT("value"), BB->GetValueAsString(BBKeyName));
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Object>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Object"));
		UObject* Obj = BB->GetValueAsObject(BBKeyName);
		Data->SetStringField(TEXT("value"), Obj ? Obj->GetName() : TEXT("None"));
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Name>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Name"));
		Data->SetStringField(TEXT("value"), BB->GetValueAsName(BBKeyName).ToString());
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Rotator>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Rotator"));
		FRotator Rot = BB->GetValueAsRotator(BBKeyName);
		TArray<TSharedPtr<FJsonValue>> RotArray;
		RotArray.Add(MakeShared<FJsonValueNumber>(Rot.Pitch));
		RotArray.Add(MakeShared<FJsonValueNumber>(Rot.Yaw));
		RotArray.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
		Data->SetArrayField(TEXT("value"), RotArray);
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Enum>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Enum"));
		Data->SetNumberField(TEXT("value"), BB->GetValueAsEnum(BBKeyName));
	}
	else if (FoundEntry->KeyType->IsA<UBlackboardKeyType_Class>())
	{
		Data->SetStringField(TEXT("value_type"), TEXT("Class"));
		UClass* ClassValue = BB->GetValueAsClass(BBKeyName);
		Data->SetStringField(TEXT("value"), ClassValue ? ClassValue->GetPathName() : TEXT("None"));
	}
	else
	{
		Data->SetStringField(TEXT("value_type"), FoundEntry->KeyType->GetClass()->GetName());
		Data->SetStringField(TEXT("value"), BB->DescribeKeyValue(KeyID, EBlackboardDescription::Full));
	}

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleGetBlackboardState(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BlackboardComponent found for actor: %s"), *ActorName));
	}

	UBlackboardData* BBData = BB->GetBlackboardAsset();
	if (!BBData)
	{
		return CreateErrorResponse(TEXT("No BlackboardData asset associated with BlackboardComponent"));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("blackboard_asset"), BBData->GetName());

	// Collect all keys from this blackboard and its parent chain
	TArray<const FBlackboardEntry*> AllKeys;
	UBlackboardData* CurrentBB = BBData;
	while (CurrentBB)
	{
		for (const FBlackboardEntry& Entry : CurrentBB->Keys)
		{
			AllKeys.Add(&Entry);
		}
		CurrentBB = CurrentBB->Parent;
	}

	TArray<TSharedPtr<FJsonValue>> KeysArray;
	for (const FBlackboardEntry* Entry : AllKeys)
	{
		if (!Entry || !Entry->KeyType)
		{
			continue;
		}

		FName BBKeyName = Entry->EntryName;
		TSharedPtr<FJsonObject> KeyObj = MakeShared<FJsonObject>();
		KeyObj->SetStringField(TEXT("key_name"), BBKeyName.ToString());

		if (Entry->KeyType->IsA<UBlackboardKeyType_Bool>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Bool"));
			KeyObj->SetBoolField(TEXT("value"), BB->GetValueAsBool(BBKeyName));
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Float>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Float"));
			KeyObj->SetNumberField(TEXT("value"), BB->GetValueAsFloat(BBKeyName));
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Int>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Int"));
			KeyObj->SetNumberField(TEXT("value"), BB->GetValueAsInt(BBKeyName));
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Vector>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Vector"));
			FVector Vec = BB->GetValueAsVector(BBKeyName);
			TArray<TSharedPtr<FJsonValue>> VecArray;
			VecArray.Add(MakeShared<FJsonValueNumber>(Vec.X));
			VecArray.Add(MakeShared<FJsonValueNumber>(Vec.Y));
			VecArray.Add(MakeShared<FJsonValueNumber>(Vec.Z));
			KeyObj->SetArrayField(TEXT("value"), VecArray);
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_String>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("String"));
			KeyObj->SetStringField(TEXT("value"), BB->GetValueAsString(BBKeyName));
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Object>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Object"));
			UObject* Obj = BB->GetValueAsObject(BBKeyName);
			KeyObj->SetStringField(TEXT("value"), Obj ? Obj->GetName() : TEXT("None"));
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Name>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Name"));
			KeyObj->SetStringField(TEXT("value"), BB->GetValueAsName(BBKeyName).ToString());
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Rotator>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Rotator"));
			FRotator Rot = BB->GetValueAsRotator(BBKeyName);
			TArray<TSharedPtr<FJsonValue>> RotArray;
			RotArray.Add(MakeShared<FJsonValueNumber>(Rot.Pitch));
			RotArray.Add(MakeShared<FJsonValueNumber>(Rot.Yaw));
			RotArray.Add(MakeShared<FJsonValueNumber>(Rot.Roll));
			KeyObj->SetArrayField(TEXT("value"), RotArray);
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Enum>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Enum"));
			KeyObj->SetNumberField(TEXT("value"), BB->GetValueAsEnum(BBKeyName));
		}
		else if (Entry->KeyType->IsA<UBlackboardKeyType_Class>())
		{
			KeyObj->SetStringField(TEXT("type"), TEXT("Class"));
			UClass* ClassValue = BB->GetValueAsClass(BBKeyName);
			KeyObj->SetStringField(TEXT("value"), ClassValue ? ClassValue->GetPathName() : TEXT("None"));
		}
		else
		{
			KeyObj->SetStringField(TEXT("type"), Entry->KeyType->GetClass()->GetName());
			FBlackboard::FKey KeyID = BB->GetKeyID(BBKeyName);
			KeyObj->SetStringField(TEXT("value"), BB->DescribeKeyValue(KeyID, EBlackboardDescription::Full));
		}

		KeysArray.Add(MakeShared<FJsonValueObject>(KeyObj));
	}

	Data->SetArrayField(TEXT("keys"), KeysArray);
	Data->SetNumberField(TEXT("key_count"), KeysArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTRuntimeCommands::HandleClearBlackboardValue(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ActorName;
	if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
	{
		return CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
	}

	FString KeyName;
	if (!Params->TryGetStringField(TEXT("key_name"), KeyName))
	{
		return CreateErrorResponse(TEXT("Missing 'key_name' parameter"));
	}

	UWorld* World = GetPIEWorld();
	if (!World)
	{
		return CreateErrorResponse(TEXT("PIE is not active. Start Play-In-Editor first."));
	}

	AAIController* AIC = FindAIControllerForActor(World, ActorName);
	if (!AIC)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No AI Controller found for actor: %s"), *ActorName));
	}

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB)
	{
		return CreateErrorResponse(FString::Printf(TEXT("No BlackboardComponent found for actor: %s"), *ActorName));
	}

	FName BBKeyName(*KeyName);
	FBlackboard::FKey KeyID = BB->GetKeyID(BBKeyName);
	if (KeyID == FBlackboard::InvalidKey)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Blackboard key not found: %s"), *KeyName));
	}

	BB->ClearValue(BBKeyName);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("actor"), ActorName);
	Data->SetStringField(TEXT("cleared_key"), KeyName);
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Command Registration
//=============================================================================

void FUnrealMCPBTRuntimeCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// BT Runtime Control
	Registry.RegisterCommand(TEXT("run_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("run_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("stop_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("stop_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("pause_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("pause_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("resume_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("resume_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("restart_behavior_tree"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("restart_behavior_tree"), P); });
	Registry.RegisterCommand(TEXT("get_bt_runtime_state"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_bt_runtime_state"), P); });

	// Blackboard Value Commands
	Registry.RegisterCommand(TEXT("set_blackboard_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_blackboard_value"), P); });
	Registry.RegisterCommand(TEXT("get_blackboard_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blackboard_value"), P); });
	Registry.RegisterCommand(TEXT("get_blackboard_state"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blackboard_state"), P); });
	Registry.RegisterCommand(TEXT("clear_blackboard_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("clear_blackboard_value"), P); });
}
