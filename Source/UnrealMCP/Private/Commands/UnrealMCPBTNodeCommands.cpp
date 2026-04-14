// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPBTNodeCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Misc/PackageName.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphPin.h"

// Behavior Tree core includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"
#include "BehaviorTree/BehaviorTreeTypes.h"

// Composite node includes
#include "BehaviorTree/Composites/BTComposite_Selector.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "BehaviorTree/Composites/BTComposite_SimpleParallel.h"

// Task node includes
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_PlaySound.h"
#include "Sound/SoundCue.h"
#include "BehaviorTree/Tasks/BTTask_PlayAnimation.h"
#include "BehaviorTree/Tasks/BTTask_RunBehavior.h"
#include "BehaviorTree/Tasks/BTTask_RunEQSQuery.h"
#include "BehaviorTree/Tasks/BTTask_FinishWithResult.h"
#include "BehaviorTree/Tasks/BTTask_MakeNoise.h"
#include "BehaviorTree/Tasks/BTTask_RotateToFaceBBEntry.h"

// Decorator node includes
#include "BehaviorTree/Decorators/BTDecorator_Blackboard.h"
#include "BehaviorTree/Decorators/BTDecorator_Cooldown.h"
#include "BehaviorTree/Decorators/BTDecorator_Loop.h"
#include "BehaviorTree/Decorators/BTDecorator_TimeLimit.h"
#include "BehaviorTree/Decorators/BTDecorator_ForceSuccess.h"
#include "BehaviorTree/Decorators/BTDecorator_CompareBBEntries.h"
#include "BehaviorTree/Decorators/BTDecorator_ConeCheck.h"
#include "BehaviorTree/Decorators/BTDecorator_DoesPathExist.h"
#include "BehaviorTree/Decorators/BTDecorator_IsAtLocation.h"
#include "BehaviorTree/Decorators/BTDecorator_TagCooldown.h"

// Service node includes
#include "BehaviorTree/Services/BTService_DefaultFocus.h"
#include "BehaviorTree/Services/BTService_RunEQS.h"

// Editor graph node includes
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Service.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "AIGraphTypes.h"

// EQS includes for RunEQS task/service
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryTypes.h"

// Gameplay tags for TagCooldown
#include "GameplayTagContainer.h"

// Reflection for setting protected UPROPERTY members
#include "UObject/UnrealType.h"

namespace BTNodeHelpers
{
	/** Set a float UPROPERTY by name via reflection. Works for protected members. */
	static void SetFloatProperty(UObject* Obj, const FString& PropName, float Value)
	{
		FProperty* Prop = Obj->GetClass()->FindPropertyByName(FName(*PropName));
		if (Prop)
		{
			FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop);
			if (FloatProp)
			{
				FloatProp->SetPropertyValue_InContainer(Obj, Value);
			}
		}
	}

	/** Set a uint8/enum UPROPERTY by name via reflection. */
	static void SetByteProperty(UObject* Obj, const FString& PropName, uint8 Value)
	{
		FProperty* Prop = Obj->GetClass()->FindPropertyByName(FName(*PropName));
		if (Prop)
		{
			FByteProperty* ByteProp = CastField<FByteProperty>(Prop);
			if (ByteProp)
			{
				ByteProp->SetPropertyValue_InContainer(Obj, Value);
				return;
			}
			// Try enum property
			FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop);
			if (EnumProp)
			{
				void* PropAddr = EnumProp->ContainerPtrToValuePtr<void>(Obj);
				FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
				if (UnderlyingProp)
				{
					UnderlyingProp->SetIntPropertyValue(PropAddr, static_cast<int64>(Value));
				}
			}
		}
	}

	/**
	 * Set the protected DefaultValue inside a FValueOrBBKey_Float struct UPROPERTY.
	 * Uses nested reflection: finds the struct property, then the float DefaultValue within it.
	 */
	static void SetValueOrBBKeyFloat(UObject* Obj, const FString& PropName, float Value)
	{
		FStructProperty* StructProp = CastField<FStructProperty>(Obj->GetClass()->FindPropertyByName(FName(*PropName)));
		if (!StructProp) { return; }
		void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Obj);
		FFloatProperty* InnerProp = CastField<FFloatProperty>(StructProp->Struct->FindPropertyByName(TEXT("DefaultValue")));
		if (InnerProp)
		{
			InnerProp->SetPropertyValue(InnerProp->ContainerPtrToValuePtr<void>(StructAddr), Value);
		}
	}

	/**
	 * Set the protected DefaultValue inside a FValueOrBBKey_Bool struct UPROPERTY.
	 */
	static void SetValueOrBBKeyBool(UObject* Obj, const FString& PropName, bool Value)
	{
		FStructProperty* StructProp = CastField<FStructProperty>(Obj->GetClass()->FindPropertyByName(FName(*PropName)));
		if (!StructProp) { return; }
		void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Obj);
		FBoolProperty* InnerProp = CastField<FBoolProperty>(StructProp->Struct->FindPropertyByName(TEXT("DefaultValue")));
		if (InnerProp)
		{
			InnerProp->SetPropertyValue(InnerProp->ContainerPtrToValuePtr<void>(StructAddr), Value);
		}
	}

	/**
	 * Set the protected DefaultValue inside a FValueOrBBKey_Object struct UPROPERTY.
	 */
	static void SetValueOrBBKeyObject(UObject* Obj, const FString& PropName, UObject* Value)
	{
		FStructProperty* StructProp = CastField<FStructProperty>(Obj->GetClass()->FindPropertyByName(FName(*PropName)));
		if (!StructProp) { return; }
		void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Obj);
		FObjectProperty* InnerProp = CastField<FObjectProperty>(StructProp->Struct->FindPropertyByName(TEXT("DefaultValue")));
		if (InnerProp)
		{
			InnerProp->SetObjectPropertyValue(InnerProp->ContainerPtrToValuePtr<void>(StructAddr), Value);
		}
	}

	/** Set a FBlackboardKeySelector UPROPERTY SelectedKeyName by property name via reflection. */
	static void SetBlackboardKeySelectorName(UObject* Obj, const FString& PropName, const FName& KeyName)
	{
		FProperty* Prop = Obj->GetClass()->FindPropertyByName(FName(*PropName));
		if (Prop)
		{
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (StructProp)
			{
				FBlackboardKeySelector* Selector = StructProp->ContainerPtrToValuePtr<FBlackboardKeySelector>(Obj);
				if (Selector)
				{
					Selector->SelectedKeyName = KeyName;
				}
			}
		}
	}

	/**
	 * Generic property setter from JSON value. Auto-detects the UPROPERTY type
	 * and applies the correct conversion.
	 *
	 * Supported types:
	 *   - float, double              (from JSON number)
	 *   - int32, int64, uint8        (from JSON number)
	 *   - bool                       (from JSON bool)
	 *   - FString                    (from JSON string)
	 *   - FName                      (from JSON string)
	 *   - FBlackboardKeySelector     (from JSON string → sets SelectedKeyName)
	 *   - FValueOrBBKey_Float        (from JSON number → sets DefaultValue)
	 *   - Enum / uint8 enum          (from JSON number or string → enum by name)
	 *
	 * @return true if the property was found and set successfully
	 */
	static bool SetPropertyFromJsonValue(UObject* Obj, const FString& PropName, const TSharedPtr<FJsonValue>& JsonValue)
	{
		if (!Obj || !JsonValue.IsValid())
		{
			return false;
		}

		FProperty* Prop = Obj->GetClass()->FindPropertyByName(FName(*PropName));
		if (!Prop)
		{
			return false;
		}

		void* PropAddr = Prop->ContainerPtrToValuePtr<void>(Obj);

		// Float
		if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
		{
			FloatProp->SetPropertyValue(PropAddr, static_cast<float>(JsonValue->AsNumber()));
			return true;
		}

		// Double
		if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
		{
			DoubleProp->SetPropertyValue(PropAddr, JsonValue->AsNumber());
			return true;
		}

		// Int32
		if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			IntProp->SetPropertyValue(PropAddr, static_cast<int32>(JsonValue->AsNumber()));
			return true;
		}

		// Int64
		if (FInt64Property* Int64Prop = CastField<FInt64Property>(Prop))
		{
			Int64Prop->SetPropertyValue(PropAddr, static_cast<int64>(JsonValue->AsNumber()));
			return true;
		}

		// Bool
		if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
		{
			BoolProp->SetPropertyValue(PropAddr, JsonValue->AsBool());
			return true;
		}

		// uint8 (plain byte or enum backing)
		if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
		{
			ByteProp->SetPropertyValue(PropAddr, static_cast<uint8>(JsonValue->AsNumber()));
			return true;
		}

		// Enum property (strongly typed)
		if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
		{
			FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
			if (!UnderlyingProp)
			{
				return false;
			}

			// Try as number first, then as enum name string
			if (JsonValue->Type == EJson::Number)
			{
				UnderlyingProp->SetIntPropertyValue(PropAddr, static_cast<int64>(JsonValue->AsNumber()));
				return true;
			}

			FString EnumStr = JsonValue->AsString();
			UEnum* EnumDef = EnumProp->GetEnum();
			if (EnumDef)
			{
				int64 EnumVal = EnumDef->GetValueByNameString(EnumStr);
				if (EnumVal != INDEX_NONE)
				{
					UnderlyingProp->SetIntPropertyValue(PropAddr, EnumVal);
					return true;
				}
			}
			return false;
		}

		// FString
		if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
		{
			StrProp->SetPropertyValue(PropAddr, JsonValue->AsString());
			return true;
		}

		// FName
		if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
		{
			NameProp->SetPropertyValue(PropAddr, FName(*JsonValue->AsString()));
			return true;
		}

		// Struct properties (FBlackboardKeySelector, FValueOrBBKey_Float, etc.)
		if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			// FBlackboardKeySelector: set SelectedKeyName from string
			if (StructProp->Struct == FBlackboardKeySelector::StaticStruct())
			{
				FBlackboardKeySelector* Selector = reinterpret_cast<FBlackboardKeySelector*>(PropAddr);
				Selector->SelectedKeyName = FName(*JsonValue->AsString());
				return true;
			}

			// FValueOrBBKey_Float: set DefaultValue from number
			FString StructName = StructProp->Struct->GetName();
			if (StructName.Contains(TEXT("ValueOrBBKey")))
			{
				FFloatProperty* InnerFloat = CastField<FFloatProperty>(
					StructProp->Struct->FindPropertyByName(TEXT("DefaultValue")));
				if (InnerFloat)
				{
					InnerFloat->SetPropertyValue(
						InnerFloat->ContainerPtrToValuePtr<void>(PropAddr),
						static_cast<float>(JsonValue->AsNumber()));
					return true;
				}
			}

			// Fallback: try ImportText for other struct types
			FString TextValue = JsonValue->AsString();
			if (!TextValue.IsEmpty())
			{
				const TCHAR* Result = Prop->ImportText_Direct(*TextValue, PropAddr, Obj, PPF_None);
				return Result != nullptr;
			}
		}

		return false;
	}
}

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// Generic node creation (any BTNode subclass)
	if (CommandType == TEXT("add_bt_node_by_class"))
	{
		return HandleAddBTNodeByClass(Params);
	}
	// Composite commands
	else if (CommandType == TEXT("add_bt_selector"))
	{
		return HandleAddBTSelector(Params);
	}
	else if (CommandType == TEXT("add_bt_sequence"))
	{
		return HandleAddBTSequence(Params);
	}
	else if (CommandType == TEXT("add_bt_simple_parallel"))
	{
		return HandleAddBTSimpleParallel(Params);
	}
	// Task commands
	else if (CommandType == TEXT("add_bt_task_wait"))
	{
		return HandleAddBTTaskWait(Params);
	}
	else if (CommandType == TEXT("add_bt_task_move_to"))
	{
		return HandleAddBTTaskMoveTo(Params);
	}
	else if (CommandType == TEXT("add_bt_task_play_sound"))
	{
		return HandleAddBTTaskPlaySound(Params);
	}
	else if (CommandType == TEXT("add_bt_task_play_animation"))
	{
		return HandleAddBTTaskPlayAnimation(Params);
	}
	else if (CommandType == TEXT("add_bt_task_run_behavior"))
	{
		return HandleAddBTTaskRunBehavior(Params);
	}
	else if (CommandType == TEXT("add_bt_task_run_eqs"))
	{
		return HandleAddBTTaskRunEQS(Params);
	}
	else if (CommandType == TEXT("add_bt_task_finish_with_result"))
	{
		return HandleAddBTTaskFinishWithResult(Params);
	}
	else if (CommandType == TEXT("add_bt_task_make_noise"))
	{
		return HandleAddBTTaskMakeNoise(Params);
	}
	else if (CommandType == TEXT("add_bt_task_rotate_to_face"))
	{
		return HandleAddBTTaskRotateToFace(Params);
	}
	else if (CommandType == TEXT("add_bt_task_set_key_value"))
	{
		return HandleAddBTTaskSetKeyValue(Params);
	}
	// Decorator commands
	else if (CommandType == TEXT("add_bt_decorator_blackboard"))
	{
		return HandleAddBTDecoratorBlackboard(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_cooldown"))
	{
		return HandleAddBTDecoratorCooldown(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_loop"))
	{
		return HandleAddBTDecoratorLoop(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_time_limit"))
	{
		return HandleAddBTDecoratorTimeLimit(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_force_success"))
	{
		return HandleAddBTDecoratorForceSuccess(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_compare_bb"))
	{
		return HandleAddBTDecoratorCompareBB(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_cone_check"))
	{
		return HandleAddBTDecoratorConeCheck(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_does_path_exist"))
	{
		return HandleAddBTDecoratorDoesPathExist(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_is_at_location"))
	{
		return HandleAddBTDecoratorIsAtLocation(Params);
	}
	else if (CommandType == TEXT("add_bt_decorator_tag_cooldown"))
	{
		return HandleAddBTDecoratorTagCooldown(Params);
	}
	// Service commands
	else if (CommandType == TEXT("add_bt_service_default_focus"))
	{
		return HandleAddBTServiceDefaultFocus(Params);
	}
	else if (CommandType == TEXT("add_bt_service_run_eqs"))
	{
		return HandleAddBTServiceRunEQS(Params);
	}

	return CreateErrorResponse(FString::Printf(TEXT("Unknown BT node command: %s"), *CommandType));
}

//=============================================================================
// Helper Methods
//=============================================================================

UBehaviorTree* FUnrealMCPBTNodeCommands::LoadBehaviorTree(const FString& TreeName, FString& OutPath)
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

UEdGraphNode* FUnrealMCPBTNodeCommands::FindGraphNodeByIndex(UBehaviorTreeGraph* BTGraph, int32 NodeIndex)
{
	if (!BTGraph || NodeIndex < 0 || NodeIndex >= BTGraph->Nodes.Num())
	{
		return nullptr;
	}
	return BTGraph->Nodes[NodeIndex];
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
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

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

//=============================================================================
// Generic Node Creation
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTNodeByClass(
	const TSharedPtr<FJsonObject>& Params)
{
	// Required: tree_name and class_name
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString ClassName;
	if (!Params->TryGetStringField(TEXT("class_name"), ClassName))
	{
		return CreateErrorResponse(TEXT("Missing 'class_name' parameter"));
	}

	// Optional: parent (tasks/composites) or target (decorators/services)
	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	int32 TargetNodeIndex = -1;
	Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex);

	// Load Behavior Tree
	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	// Resolve UClass dynamically via reflection.
	// Try: exact name, with U prefix, without U prefix
	UClass* NodeClass = nullptr;
	{
		TArray<FString> Candidates;
		Candidates.Add(ClassName);
		if (!ClassName.StartsWith(TEXT("U")))
		{
			Candidates.Add(FString::Printf(TEXT("U%s"), *ClassName));
		}
		else
		{
			Candidates.Add(ClassName.Mid(1));
		}

		for (const FString& Candidate : Candidates)
		{
			NodeClass = FindFirstObject<UClass>(*Candidate, EFindFirstObjectOptions::NativeFirst);
			if (NodeClass)
			{
				break;
			}
		}
	}

	if (!NodeClass)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Class not found: %s. Ensure the module containing this class is loaded."), *ClassName));
	}

	// Classify the node type based on class hierarchy
	bool bIsTask = NodeClass->IsChildOf(UBTTaskNode::StaticClass());
	bool bIsDecorator = NodeClass->IsChildOf(UBTDecorator::StaticClass());
	bool bIsService = NodeClass->IsChildOf(UBTService::StaticClass());
	bool bIsComposite = NodeClass->IsChildOf(UBTCompositeNode::StaticClass());

	if (!bIsTask && !bIsDecorator && !bIsService && !bIsComposite)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Class '%s' does not derive from UBTTaskNode, UBTDecorator, UBTService, or UBTCompositeNode"),
			*ClassName));
	}

	FString NodeCategory;
	UBehaviorTreeGraphNode* GraphNode = nullptr;

	if (bIsTask)
	{
		NodeCategory = TEXT("Task");
		UBehaviorTreeGraphNode_Task* TaskGraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
		UBTTaskNode* NodeInstance = NewObject<UBTTaskNode>(TaskGraphNode, NodeClass);

		TaskGraphNode->NodeInstance = NodeInstance;
		TaskGraphNode->ClassData = FGraphNodeClassData(NodeClass, TEXT(""));
		GraphNode = TaskGraphNode;
	}
	else if (bIsDecorator)
	{
		NodeCategory = TEXT("Decorator");
		if (TargetNodeIndex < 0)
		{
			return CreateErrorResponse(TEXT("Decorators require 'target_node_index' to attach to"));
		}

		UBehaviorTreeGraphNode_Decorator* DecGraphNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
		UBTDecorator* NodeInstance = NewObject<UBTDecorator>(DecGraphNode, NodeClass);

		DecGraphNode->NodeInstance = NodeInstance;
		DecGraphNode->ClassData = FGraphNodeClassData(NodeClass, TEXT(""));
		GraphNode = DecGraphNode;
	}
	else if (bIsService)
	{
		NodeCategory = TEXT("Service");
		if (TargetNodeIndex < 0)
		{
			return CreateErrorResponse(TEXT("Services require 'target_node_index' to attach to"));
		}

		UBehaviorTreeGraphNode_Service* SvcGraphNode = NewObject<UBehaviorTreeGraphNode_Service>(BTGraph);
		UBTService* NodeInstance = NewObject<UBTService>(SvcGraphNode, NodeClass);

		SvcGraphNode->NodeInstance = NodeInstance;
		SvcGraphNode->ClassData = FGraphNodeClassData(NodeClass, TEXT(""));
		GraphNode = SvcGraphNode;
	}
	else if (bIsComposite)
	{
		NodeCategory = TEXT("Composite");
		UBehaviorTreeGraphNode_Composite* CompGraphNode = NewObject<UBehaviorTreeGraphNode_Composite>(BTGraph);
		UBTCompositeNode* NodeInstance = NewObject<UBTCompositeNode>(CompGraphNode, NodeClass);

		CompGraphNode->NodeInstance = NodeInstance;
		CompGraphNode->ClassData = FGraphNodeClassData(NodeClass, TEXT(""));
		GraphNode = CompGraphNode;
	}

	if (!GraphNode)
	{
		return CreateErrorResponse(TEXT("Failed to create graph node"));
	}

	// Set properties from the optional "properties" JSON object
	const TSharedPtr<FJsonObject>* PropertiesObj = nullptr;
	TArray<FString> PropsSet;
	TArray<FString> PropsFailed;

	if (Params->TryGetObjectField(TEXT("properties"), PropertiesObj) && PropertiesObj->IsValid())
	{
		UObject* NodeInstance = GraphNode->NodeInstance;
		for (const auto& Pair : (*PropertiesObj)->Values)
		{
			if (BTNodeHelpers::SetPropertyFromJsonValue(NodeInstance, Pair.Key, Pair.Value))
			{
				PropsSet.Add(Pair.Key);
			}
			else
			{
				PropsFailed.Add(Pair.Key);
			}
		}
	}

	// For decorators/services: attach as sub-node to the target
	if (bIsDecorator || bIsService)
	{
		UEdGraphNode* TargetEdNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
		UBehaviorTreeGraphNode* TargetBTNode = Cast<UBehaviorTreeGraphNode>(TargetEdNode);
		if (!TargetBTNode)
		{
			return CreateErrorResponse(FString::Printf(
				TEXT("Target node at index %d is not a valid BT graph node"), TargetNodeIndex));
		}

		TargetBTNode->AddSubNode(GraphNode, BTGraph);
	}
	else
	{
		// For tasks/composites: add to graph and connect to parent via pins
		BTGraph->AddNode(GraphNode, true, false);
		GraphNode->CreateNewGuid();
		GraphNode->PostPlacedNewNode();
		GraphNode->AllocateDefaultPins();

		if (ParentNodeIndex >= 0)
		{
			UEdGraphNode* ParentEdNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
			if (ParentEdNode)
			{
				UEdGraphPin* ParentOut = nullptr;
				UEdGraphPin* ChildIn = nullptr;

				for (UEdGraphPin* Pin : ParentEdNode->Pins)
				{
					if (Pin->Direction == EGPD_Output)
					{
						ParentOut = Pin;
						break;
					}
				}
				for (UEdGraphPin* Pin : GraphNode->Pins)
				{
					if (Pin->Direction == EGPD_Input)
					{
						ChildIn = Pin;
						break;
					}
				}

				if (ParentOut && ChildIn)
				{
					ParentOut->MakeLinkTo(ChildIn);
				}
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	// Build response
	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_class"), NodeClass->GetName());
	Data->SetStringField(TEXT("node_category"), NodeCategory);
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);

	if (PropsSet.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> SetArr;
		for (const FString& S : PropsSet)
		{
			SetArr.Add(MakeShared<FJsonValueString>(S));
		}
		Data->SetArrayField(TEXT("properties_set"), SetArr);
	}
	if (PropsFailed.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> FailArr;
		for (const FString& S : PropsFailed)
		{
			FailArr.Add(MakeShared<FJsonValueString>(S));
		}
		Data->SetArrayField(TEXT("properties_failed"), FailArr);
	}

	return CreateSuccessResponse(Data);
}

//=============================================================================
// Composite Node Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTSelector(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	// Create graph node wrapper
	UBehaviorTreeGraphNode_Composite* GraphNode = NewObject<UBehaviorTreeGraphNode_Composite>(BTGraph);

	// Create runtime node instance
	UBTComposite_Selector* NodeInstance = NewObject<UBTComposite_Selector>(GraphNode);
	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTComposite_Selector::StaticClass(), TEXT(""));

	// Place in graph
	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	// Connect to parent if specified
	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	// Find the index of the newly added node
	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Selector"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTComposite_Selector"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTSequence(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Composite* GraphNode = NewObject<UBehaviorTreeGraphNode_Composite>(BTGraph);
	UBTComposite_Sequence* NodeInstance = NewObject<UBTComposite_Sequence>(GraphNode);
	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTComposite_Sequence::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Sequence"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTComposite_Sequence"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTSimpleParallel(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString FinishModeStr = TEXT("Immediate");
	Params->TryGetStringField(TEXT("finish_mode"), FinishModeStr);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Composite* GraphNode = NewObject<UBehaviorTreeGraphNode_Composite>(BTGraph);
	UBTComposite_SimpleParallel* NodeInstance = NewObject<UBTComposite_SimpleParallel>(GraphNode);

	// Set finish mode
	if (FinishModeStr == TEXT("Delayed") || FinishModeStr == TEXT("WaitForBackground"))
	{
		NodeInstance->FinishMode = EBTParallelMode::WaitForBackground;
	}
	else
	{
		NodeInstance->FinishMode = EBTParallelMode::AbortBackground;
	}

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTComposite_SimpleParallel::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("SimpleParallel"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetStringField(TEXT("finish_mode"), FinishModeStr);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTComposite_SimpleParallel"));
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Task Node Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskWait(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	double WaitTimeValue = 5.0;
	Params->TryGetNumberField(TEXT("wait_time"), WaitTimeValue);

	double RandomDeviationValue = 0.0;
	Params->TryGetNumberField(TEXT("random_deviation"), RandomDeviationValue);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_Wait* NodeInstance = NewObject<UBTTask_Wait>(GraphNode);

	// WaitTime and RandomDeviation are FValueOrBBKey_Float in UE 5.7
	BTNodeHelpers::SetValueOrBBKeyFloat(NodeInstance, TEXT("WaitTime"), static_cast<float>(WaitTimeValue));
	BTNodeHelpers::SetValueOrBBKeyFloat(NodeInstance, TEXT("RandomDeviation"), static_cast<float>(RandomDeviationValue));

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_Wait::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Wait"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetNumberField(TEXT("wait_time"), WaitTimeValue);
	Data->SetNumberField(TEXT("random_deviation"), RandomDeviationValue);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_Wait"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskMoveTo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	double AcceptableRadiusValue = 5.0;
	Params->TryGetNumberField(TEXT("acceptable_radius"), AcceptableRadiusValue);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_MoveTo* NodeInstance = NewObject<UBTTask_MoveTo>(GraphNode);

	// AcceptableRadius is FValueOrBBKey_Float in UE 5.7
	BTNodeHelpers::SetValueOrBBKeyFloat(NodeInstance, TEXT("AcceptableRadius"), static_cast<float>(AcceptableRadiusValue));

	// BlackboardKey is protected on UBTTask_BlackboardBase - use reflection
	if (!BBKey.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(NodeInstance, TEXT("BlackboardKey"), FName(*BBKey));
	}

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_MoveTo::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("MoveTo"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetNumberField(TEXT("acceptable_radius"), AcceptableRadiusValue);
	if (!BBKey.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key"), BBKey);
	}
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_MoveTo"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskPlaySound(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString SoundPath;
	Params->TryGetStringField(TEXT("sound_path"), SoundPath);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_PlaySound* NodeInstance = NewObject<UBTTask_PlaySound>(GraphNode);

	// Load sound asset if path provided
	if (!SoundPath.IsEmpty())
	{
		USoundCue* Sound = Cast<USoundCue>(UEditorAssetLibrary::LoadAsset(SoundPath));
		if (Sound)
		{
#if ENGINE_MINOR_VERSION >= 5
			// SoundToPlay is FValueOrBBKey_Object in UE 5.5+ - set via reflection
			BTNodeHelpers::SetValueOrBBKeyObject(NodeInstance, TEXT("SoundToPlay"), Sound);
#else
			// SoundToPlay is USoundCue* directly in older UE versions
			if (FObjectProperty* SoundProp = CastField<FObjectProperty>(NodeInstance->GetClass()->FindPropertyByName(TEXT("SoundToPlay"))))
			{
				SoundProp->SetObjectPropertyValue(SoundProp->ContainerPtrToValuePtr<void>(NodeInstance), Sound);
			}
#endif
		}
	}

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_PlaySound::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("PlaySound"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	if (!SoundPath.IsEmpty())
	{
		Data->SetStringField(TEXT("sound_path"), SoundPath);
	}
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_PlaySound"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskPlayAnimation(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString AnimPath;
	Params->TryGetStringField(TEXT("anim_path"), AnimPath);

	bool bLooping = false;
	Params->TryGetBoolField(TEXT("looping"), bLooping);

	bool bNonBlocking = false;
	Params->TryGetBoolField(TEXT("non_blocking"), bNonBlocking);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_PlayAnimation* NodeInstance = NewObject<UBTTask_PlayAnimation>(GraphNode);

	// Load animation asset if path provided
	if (!AnimPath.IsEmpty())
	{
		UAnimationAsset* Anim = Cast<UAnimationAsset>(UEditorAssetLibrary::LoadAsset(AnimPath));
		if (Anim)
		{
			// AnimationToPlay is FValueOrBBKey_Object in UE 5.7 - set via reflection
			BTNodeHelpers::SetValueOrBBKeyObject(NodeInstance, TEXT("AnimationToPlay"), Anim);
		}
	}

	// bLooping and bNonBlocking are FValueOrBBKey_Bool in UE 5.7 - set via reflection
	BTNodeHelpers::SetValueOrBBKeyBool(NodeInstance, TEXT("bLooping"), bLooping);
	BTNodeHelpers::SetValueOrBBKeyBool(NodeInstance, TEXT("bNonBlocking"), bNonBlocking);

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_PlayAnimation::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("PlayAnimation"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetBoolField(TEXT("looping"), bLooping);
	Data->SetBoolField(TEXT("non_blocking"), bNonBlocking);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_PlayAnimation"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskRunBehavior(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	FString SubTreeName;
	if (!Params->TryGetStringField(TEXT("sub_tree_name"), SubTreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'sub_tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	// Load the sub-tree
	FString SubTreePath;
	UBehaviorTree* SubTree = LoadBehaviorTree(SubTreeName, SubTreePath);
	if (!SubTree)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Sub-tree not found: %s"), *SubTreeName));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_RunBehavior* NodeInstance = NewObject<UBTTask_RunBehavior>(GraphNode);

	// BehaviorAsset is a TObjectPtr<UBehaviorTree> - set via reflection since it's protected
	FProperty* BehaviorAssetProp = UBTTask_RunBehavior::StaticClass()->FindPropertyByName(TEXT("BehaviorAsset"));
	if (BehaviorAssetProp)
	{
		FObjectProperty* ObjProp = CastField<FObjectProperty>(BehaviorAssetProp);
		if (ObjProp)
		{
			ObjProp->SetObjectPropertyValue_InContainer(NodeInstance, SubTree);
		}
	}

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_RunBehavior::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("RunBehavior"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetStringField(TEXT("sub_tree"), SubTreeName);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_RunBehavior"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskRunEQS(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString QueryTemplate;
	Params->TryGetStringField(TEXT("query_template"), QueryTemplate);

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_RunEQSQuery* NodeInstance = NewObject<UBTTask_RunEQSQuery>(GraphNode);

	// BlackboardKey is protected on UBTTask_BlackboardBase - use reflection
	if (!BBKey.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(NodeInstance, TEXT("BlackboardKey"), FName(*BBKey));
	}

	// EQSRequest is protected - set QueryTemplate via reflection
	if (!QueryTemplate.IsEmpty())
	{
		UEnvQuery* Query = Cast<UEnvQuery>(UEditorAssetLibrary::LoadAsset(QueryTemplate));
		if (Query)
		{
			FProperty* EQSProp = NodeInstance->GetClass()->FindPropertyByName(TEXT("EQSRequest"));
			if (EQSProp)
			{
				FStructProperty* StructProp = CastField<FStructProperty>(EQSProp);
				if (StructProp)
				{
					FEQSParametrizedQueryExecutionRequest* Request = StructProp->ContainerPtrToValuePtr<FEQSParametrizedQueryExecutionRequest>(NodeInstance);
					if (Request)
					{
						Request->QueryTemplate = Query;
					}
				}
			}
		}
	}

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_RunEQSQuery::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("RunEQSQuery"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	if (!BBKey.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key"), BBKey);
	}
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_RunEQSQuery"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskFinishWithResult(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString ResultStr = TEXT("Succeeded");
	Params->TryGetStringField(TEXT("result"), ResultStr);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_FinishWithResult* NodeInstance = NewObject<UBTTask_FinishWithResult>(GraphNode);

	// Result is a protected FValueOrBBKey_Enum in UE 5.7 - use reflection
	// EBTNodeResult::Type values: Succeeded=0, Failed=1, Aborted=2, InProgress=3
	{
		uint8 ResultValue = static_cast<uint8>(EBTNodeResult::Succeeded);
		if (ResultStr == TEXT("Failed"))
		{
			ResultValue = static_cast<uint8>(EBTNodeResult::Failed);
		}
		else if (ResultStr == TEXT("Aborted"))
		{
			ResultValue = static_cast<uint8>(EBTNodeResult::Aborted);
		}
		BTNodeHelpers::SetByteProperty(NodeInstance, TEXT("Result"), ResultValue);
	}

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_FinishWithResult::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("FinishWithResult"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetStringField(TEXT("result"), ResultStr);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_FinishWithResult"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskMakeNoise(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	double LoudnessValue = 0.5;
	Params->TryGetNumberField(TEXT("loudness"), LoudnessValue);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_MakeNoise* NodeInstance = NewObject<UBTTask_MakeNoise>(GraphNode);

	// Note: Property is named "Loudnes" (typo in UE source, missing 's')
	// Loudnes is FValueOrBBKey_Float in UE 5.7
	BTNodeHelpers::SetValueOrBBKeyFloat(NodeInstance, TEXT("Loudnes"), static_cast<float>(LoudnessValue));

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_MakeNoise::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("MakeNoise"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetNumberField(TEXT("loudness"), LoudnessValue);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_MakeNoise"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskRotateToFace(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_RotateToFaceBBEntry* NodeInstance = NewObject<UBTTask_RotateToFaceBBEntry>(GraphNode);

	// BlackboardKey is protected on UBTTask_BlackboardBase - use reflection
	if (!BBKey.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(NodeInstance, TEXT("BlackboardKey"), FName(*BBKey));
	}

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_RotateToFaceBBEntry::StaticClass(), TEXT(""));

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("RotateToFaceBBEntry"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	if (!BBKey.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key"), BBKey);
	}
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_RotateToFaceBBEntry"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTTaskSetKeyValue(
	const TSharedPtr<FJsonObject>& Params)
{
	// NOTE: UBTTask_SetKeyValue does not exist as a single class in UE 5.7.
	// There is a family of specialized set-value tasks. As a practical placeholder,
	// we create a UBTTask_Wait node with a descriptive comment indicating it should
	// be replaced with the appropriate set-key-value logic in Blueprint or C++.

	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentNodeIndex = -1;
	Params->TryGetNumberField(TEXT("parent_node_index"), ParentNodeIndex);

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	FString Value;
	Params->TryGetStringField(TEXT("value"), Value);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	// Create a Wait task as a placeholder since there is no single UBTTask_SetKeyValue class
	UBehaviorTreeGraphNode_Task* GraphNode = NewObject<UBehaviorTreeGraphNode_Task>(BTGraph);
	UBTTask_Wait* NodeInstance = NewObject<UBTTask_Wait>(GraphNode);

	// Set a minimal wait time to indicate this is a placeholder
	BTNodeHelpers::SetValueOrBBKeyFloat(NodeInstance, TEXT("WaitTime"), 0.0f);

	GraphNode->NodeInstance = NodeInstance;
	GraphNode->ClassData = FGraphNodeClassData(UBTTask_Wait::StaticClass(), TEXT(""));

	// Add a comment to the node indicating its intended purpose
	GraphNode->NodeComment = FString::Printf(TEXT("PLACEHOLDER: Set BB Key '%s' = '%s'. Replace with BTTask_BlueprintBase implementation."), *BBKey, *Value);

	BTGraph->AddNode(GraphNode, true, false);
	GraphNode->CreateNewGuid();
	GraphNode->PostPlacedNewNode();
	GraphNode->AllocateDefaultPins();

	if (ParentNodeIndex >= 0)
	{
		UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BTGraph, ParentNodeIndex);
		if (ParentGraphNode)
		{
			UEdGraphPin* ParentOut = nullptr;
			UEdGraphPin* ChildIn = nullptr;
			for (UEdGraphPin* Pin : ParentGraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Output)
				{
					ParentOut = Pin;
					break;
				}
			}
			for (UEdGraphPin* Pin : GraphNode->Pins)
			{
				if (Pin->Direction == EGPD_Input)
				{
					ChildIn = Pin;
					break;
				}
			}
			if (ParentOut && ChildIn)
			{
				ParentOut->MakeLinkTo(ChildIn);
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	int32 NewNodeIndex = BTGraph->Nodes.IndexOfByKey(GraphNode);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("SetKeyValue_Placeholder"));
	Data->SetNumberField(TEXT("node_index"), NewNodeIndex);
	Data->SetStringField(TEXT("bb_key"), BBKey);
	Data->SetStringField(TEXT("value"), Value);
	Data->SetStringField(TEXT("note"), TEXT("UBTTask_SetKeyValue does not exist in UE 5.7. Created a Wait(0) placeholder with comment. Replace with Blueprint-based task."));
	Data->SetStringField(TEXT("node_class"), TEXT("UBTTask_Wait"));
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Decorator Node Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorBlackboard(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	FString NotifyObserverStr = TEXT("ResultChange");
	Params->TryGetStringField(TEXT("notify_observer"), NotifyObserverStr);

	FString FlowAbortModeStr = TEXT("None");
	Params->TryGetStringField(TEXT("flow_abort_mode"), FlowAbortModeStr);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	// Create decorator graph node
	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_Blackboard* DecInstance = NewObject<UBTDecorator_Blackboard>(DecNode);

	// BlackboardKey is protected on UBTDecorator_BlackboardBase - use reflection
	if (!BBKey.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(DecInstance, TEXT("BlackboardKey"), FName(*BBKey));
	}

	// NotifyObserver is a protected UPROPERTY - use reflection
	{
		uint8 NotifyValue = static_cast<uint8>(EBTBlackboardRestart::ResultChange);
		if (NotifyObserverStr == TEXT("ValueChange"))
		{
			NotifyValue = static_cast<uint8>(EBTBlackboardRestart::ValueChange);
		}
		BTNodeHelpers::SetByteProperty(DecInstance, TEXT("NotifyObserver"), NotifyValue);
	}

	// FlowAbortMode is a protected UPROPERTY on the base UBTDecorator - use reflection
	{
		uint8 AbortMode = static_cast<uint8>(EBTFlowAbortMode::None);
		if (FlowAbortModeStr == TEXT("Self"))
		{
			AbortMode = static_cast<uint8>(EBTFlowAbortMode::Self);
		}
		else if (FlowAbortModeStr == TEXT("LowerPriority"))
		{
			AbortMode = static_cast<uint8>(EBTFlowAbortMode::LowerPriority);
		}
		else if (FlowAbortModeStr == TEXT("Both"))
		{
			AbortMode = static_cast<uint8>(EBTFlowAbortMode::Both);
		}
		BTNodeHelpers::SetByteProperty(DecInstance, TEXT("FlowAbortMode"), AbortMode);
	}

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_Blackboard::StaticClass(), TEXT(""));

	// Attach as sub-node to the target
	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_Blackboard"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	if (!BBKey.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key"), BBKey);
	}
	Data->SetStringField(TEXT("notify_observer"), NotifyObserverStr);
	Data->SetStringField(TEXT("flow_abort_mode"), FlowAbortModeStr);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_Blackboard"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorCooldown(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	double CooldownTime = 5.0;
	Params->TryGetNumberField(TEXT("cooldown_time"), CooldownTime);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_Cooldown* DecInstance = NewObject<UBTDecorator_Cooldown>(DecNode);

	// CoolDownTime is FValueOrBBKey_Float in UE 5.7
	DecInstance->CoolDownTime = static_cast<float>(CooldownTime);

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_Cooldown::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_Cooldown"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	Data->SetNumberField(TEXT("cooldown_time"), CooldownTime);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_Cooldown"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorLoop(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	int32 NumLoops = 3;
	Params->TryGetNumberField(TEXT("num_loops"), NumLoops);

	bool bInfiniteLoop = false;
	Params->TryGetBoolField(TEXT("infinite_loop"), bInfiniteLoop);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_Loop* DecInstance = NewObject<UBTDecorator_Loop>(DecNode);

	// NumLoops is FValueOrBBKey_Int32 in UE 5.7
	DecInstance->NumLoops = NumLoops;
	DecInstance->bInfiniteLoop = bInfiniteLoop;

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_Loop::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_Loop"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	Data->SetNumberField(TEXT("num_loops"), NumLoops);
	Data->SetBoolField(TEXT("infinite_loop"), bInfiniteLoop);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_Loop"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorTimeLimit(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	double TimeLimitValue = 5.0;
	Params->TryGetNumberField(TEXT("time_limit"), TimeLimitValue);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_TimeLimit* DecInstance = NewObject<UBTDecorator_TimeLimit>(DecNode);

	// TimeLimit is FValueOrBBKey_Float in UE 5.7
	DecInstance->TimeLimit = static_cast<float>(TimeLimitValue);

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_TimeLimit::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_TimeLimit"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	Data->SetNumberField(TEXT("time_limit"), TimeLimitValue);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_TimeLimit"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorForceSuccess(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_ForceSuccess* DecInstance = NewObject<UBTDecorator_ForceSuccess>(DecNode);

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_ForceSuccess::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_ForceSuccess"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_ForceSuccess"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorCompareBB(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString KeyA;
	Params->TryGetStringField(TEXT("key_a"), KeyA);

	FString KeyB;
	Params->TryGetStringField(TEXT("key_b"), KeyB);

	FString OperatorStr = TEXT("Equal");
	Params->TryGetStringField(TEXT("operator"), OperatorStr);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_CompareBBEntries* DecInstance = NewObject<UBTDecorator_CompareBBEntries>(DecNode);

	// BlackboardKeyA, BlackboardKeyB, and Operator are protected - use reflection
	if (!KeyA.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(DecInstance, TEXT("BlackboardKeyA"), FName(*KeyA));
	}
	if (!KeyB.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(DecInstance, TEXT("BlackboardKeyB"), FName(*KeyB));
	}

	// Set operator via reflection
	{
		uint8 OpValue = static_cast<uint8>(EBlackBoardEntryComparison::Equal);
		if (OperatorStr == TEXT("NotEqual"))
		{
			OpValue = static_cast<uint8>(EBlackBoardEntryComparison::NotEqual);
		}
		BTNodeHelpers::SetByteProperty(DecInstance, TEXT("Operator"), OpValue);
	}

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_CompareBBEntries::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_CompareBBEntries"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	if (!KeyA.IsEmpty())
	{
		Data->SetStringField(TEXT("key_a"), KeyA);
	}
	if (!KeyB.IsEmpty())
	{
		Data->SetStringField(TEXT("key_b"), KeyB);
	}
	Data->SetStringField(TEXT("operator"), OperatorStr);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_CompareBBEntries"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorConeCheck(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	double ConeHalfAngle = 45.0;
	Params->TryGetNumberField(TEXT("cone_half_angle"), ConeHalfAngle);

	FString ConeOriginKey;
	Params->TryGetStringField(TEXT("cone_origin_key"), ConeOriginKey);

	FString ObservedKey;
	Params->TryGetStringField(TEXT("observed_key"), ObservedKey);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_ConeCheck* DecInstance = NewObject<UBTDecorator_ConeCheck>(DecNode);

	// ConeHalfAngle is FValueOrBBKey_Float in UE 5.7
	DecInstance->ConeHalfAngle = static_cast<float>(ConeHalfAngle);

	// Set blackboard key selectors
	if (!ConeOriginKey.IsEmpty())
	{
		DecInstance->ConeOrigin.SelectedKeyName = FName(*ConeOriginKey);
	}
	if (!ObservedKey.IsEmpty())
	{
		DecInstance->Observed.SelectedKeyName = FName(*ObservedKey);
	}

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_ConeCheck::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_ConeCheck"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	Data->SetNumberField(TEXT("cone_half_angle"), ConeHalfAngle);
	if (!ConeOriginKey.IsEmpty())
	{
		Data->SetStringField(TEXT("cone_origin_key"), ConeOriginKey);
	}
	if (!ObservedKey.IsEmpty())
	{
		Data->SetStringField(TEXT("observed_key"), ObservedKey);
	}
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_ConeCheck"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorDoesPathExist(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString BBKeyA;
	Params->TryGetStringField(TEXT("bb_key_a"), BBKeyA);

	FString BBKeyB;
	Params->TryGetStringField(TEXT("bb_key_b"), BBKeyB);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_DoesPathExist* DecInstance = NewObject<UBTDecorator_DoesPathExist>(DecNode);

	// BlackboardKeyA and BlackboardKeyB are protected - use reflection
	if (!BBKeyA.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(DecInstance, TEXT("BlackboardKeyA"), FName(*BBKeyA));
	}
	if (!BBKeyB.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(DecInstance, TEXT("BlackboardKeyB"), FName(*BBKeyB));
	}

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_DoesPathExist::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_DoesPathExist"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	if (!BBKeyA.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key_a"), BBKeyA);
	}
	if (!BBKeyB.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key_b"), BBKeyB);
	}
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_DoesPathExist"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorIsAtLocation(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	double AcceptableRadius = 50.0;
	Params->TryGetNumberField(TEXT("acceptable_radius"), AcceptableRadius);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_IsAtLocation* DecInstance = NewObject<UBTDecorator_IsAtLocation>(DecNode);

	// BlackboardKey is protected on UBTDecorator_BlackboardBase - use reflection
	if (!BBKey.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(DecInstance, TEXT("BlackboardKey"), FName(*BBKey));
	}

	// AcceptableRadius is a public float on UBTDecorator_IsAtLocation
	DecInstance->AcceptableRadius = static_cast<float>(AcceptableRadius);

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_IsAtLocation::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_IsAtLocation"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	if (!BBKey.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key"), BBKey);
	}
	Data->SetNumberField(TEXT("acceptable_radius"), AcceptableRadius);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_IsAtLocation"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTDecoratorTagCooldown(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString CooldownTagStr;
	if (!Params->TryGetStringField(TEXT("cooldown_tag"), CooldownTagStr))
	{
		return CreateErrorResponse(TEXT("Missing 'cooldown_tag' parameter"));
	}

	double CooldownDuration = 5.0;
	Params->TryGetNumberField(TEXT("cooldown_duration"), CooldownDuration);

	bool bAddToExisting = true;
	Params->TryGetBoolField(TEXT("add_to_existing"), bAddToExisting);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Decorator* DecNode = NewObject<UBehaviorTreeGraphNode_Decorator>(BTGraph);
	UBTDecorator_TagCooldown* DecInstance = NewObject<UBTDecorator_TagCooldown>(DecNode);

	// Set the gameplay tag
	DecInstance->CooldownTag = FGameplayTag::RequestGameplayTag(FName(*CooldownTagStr), false);

	// CooldownDuration is FValueOrBBKey_Float in UE 5.7
	DecInstance->CooldownDuration = static_cast<float>(CooldownDuration);

	// bAddToExistingDuration is FValueOrBBKey_Bool in UE 5.7
	DecInstance->bAddToExistingDuration = bAddToExisting;

	DecNode->NodeInstance = DecInstance;
	DecNode->ClassData = FGraphNodeClassData(UBTDecorator_TagCooldown::StaticClass(), TEXT(""));

	BTGraphNode->AddSubNode(DecNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Decorator_TagCooldown"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	Data->SetStringField(TEXT("cooldown_tag"), CooldownTagStr);
	Data->SetNumberField(TEXT("cooldown_duration"), CooldownDuration);
	Data->SetBoolField(TEXT("add_to_existing"), bAddToExisting);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTDecorator_TagCooldown"));
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Service Node Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTServiceDefaultFocus(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	double IntervalValue = 0.5;
	Params->TryGetNumberField(TEXT("interval"), IntervalValue);

	double RandomDeviationValue = 0.1;
	Params->TryGetNumberField(TEXT("random_deviation"), RandomDeviationValue);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Service* SvcNode = NewObject<UBehaviorTreeGraphNode_Service>(BTGraph);
	UBTService_DefaultFocus* SvcInstance = NewObject<UBTService_DefaultFocus>(SvcNode);

	// BlackboardKey is protected on UBTService_BlackboardBase - use reflection
	if (!BBKey.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(SvcInstance, TEXT("BlackboardKey"), FName(*BBKey));
	}

	// Interval and RandomDeviation are protected on UBTService base class - use reflection
	BTNodeHelpers::SetFloatProperty(SvcInstance, TEXT("Interval"), static_cast<float>(IntervalValue));
	BTNodeHelpers::SetFloatProperty(SvcInstance, TEXT("RandomDeviation"), static_cast<float>(RandomDeviationValue));

	SvcNode->NodeInstance = SvcInstance;
	SvcNode->ClassData = FGraphNodeClassData(UBTService_DefaultFocus::StaticClass(), TEXT(""));

	// Attach as sub-node to the target
	BTGraphNode->AddSubNode(SvcNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Service_DefaultFocus"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	if (!BBKey.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key"), BBKey);
	}
	Data->SetNumberField(TEXT("interval"), IntervalValue);
	Data->SetNumberField(TEXT("random_deviation"), RandomDeviationValue);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTService_DefaultFocus"));
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPBTNodeCommands::HandleAddBTServiceRunEQS(
	const TSharedPtr<FJsonObject>& Params)
{
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 TargetNodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("target_node_index"), TargetNodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'target_node_index' parameter"));
	}

	FString QueryTemplate;
	Params->TryGetStringField(TEXT("query_template"), QueryTemplate);

	FString BBKey;
	Params->TryGetStringField(TEXT("bb_key"), BBKey);

	double IntervalValue = 0.5;
	Params->TryGetNumberField(TEXT("interval"), IntervalValue);

	double RandomDeviationValue = 0.1;
	Params->TryGetNumberField(TEXT("random_deviation"), RandomDeviationValue);

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no valid graph"));
	}

	UEdGraphNode* TargetGraphNode = FindGraphNodeByIndex(BTGraph, TargetNodeIndex);
	if (!TargetGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target node not found at index: %d"), TargetNodeIndex));
	}

	UBehaviorTreeGraphNode* BTGraphNode = Cast<UBehaviorTreeGraphNode>(TargetGraphNode);
	if (!BTGraphNode)
	{
		return CreateErrorResponse(TEXT("Target node is not a valid BT graph node"));
	}

	UBehaviorTreeGraphNode_Service* SvcNode = NewObject<UBehaviorTreeGraphNode_Service>(BTGraph);
	UBTService_RunEQS* SvcInstance = NewObject<UBTService_RunEQS>(SvcNode);

	// BlackboardKey is protected on UBTService_BlackboardBase - use reflection
	if (!BBKey.IsEmpty())
	{
		BTNodeHelpers::SetBlackboardKeySelectorName(SvcInstance, TEXT("BlackboardKey"), FName(*BBKey));
	}

	// EQSRequest is protected - set QueryTemplate via reflection
	if (!QueryTemplate.IsEmpty())
	{
		UEnvQuery* Query = Cast<UEnvQuery>(UEditorAssetLibrary::LoadAsset(QueryTemplate));
		if (Query)
		{
			FProperty* EQSProp = SvcInstance->GetClass()->FindPropertyByName(TEXT("EQSRequest"));
			if (EQSProp)
			{
				FStructProperty* StructProp = CastField<FStructProperty>(EQSProp);
				if (StructProp)
				{
					FEQSParametrizedQueryExecutionRequest* Request = StructProp->ContainerPtrToValuePtr<FEQSParametrizedQueryExecutionRequest>(SvcInstance);
					if (Request)
					{
						Request->QueryTemplate = Query;
					}
				}
			}
		}
	}

	// Interval and RandomDeviation are protected on UBTService base class - use reflection
	BTNodeHelpers::SetFloatProperty(SvcInstance, TEXT("Interval"), static_cast<float>(IntervalValue));
	BTNodeHelpers::SetFloatProperty(SvcInstance, TEXT("RandomDeviation"), static_cast<float>(RandomDeviationValue));

	SvcNode->NodeInstance = SvcInstance;
	SvcNode->ClassData = FGraphNodeClassData(UBTService_RunEQS::StaticClass(), TEXT(""));

	// Attach as sub-node to the target
	BTGraphNode->AddSubNode(SvcNode, BTGraph);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("node_type"), TEXT("Service_RunEQS"));
	Data->SetNumberField(TEXT("target_node_index"), TargetNodeIndex);
	if (!BBKey.IsEmpty())
	{
		Data->SetStringField(TEXT("bb_key"), BBKey);
	}
	Data->SetNumberField(TEXT("interval"), IntervalValue);
	Data->SetNumberField(TEXT("random_deviation"), RandomDeviationValue);
	Data->SetStringField(TEXT("node_class"), TEXT("UBTService_RunEQS"));
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Command Registration
//=============================================================================

void FUnrealMCPBTNodeCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// Generic node creation (any BTNode subclass via reflection)
	Registry.RegisterCommand(TEXT("add_bt_node_by_class"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_node_by_class"), P); });

	// Composite commands
	Registry.RegisterCommand(TEXT("add_bt_selector"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_selector"), P); });
	Registry.RegisterCommand(TEXT("add_bt_sequence"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_sequence"), P); });
	Registry.RegisterCommand(TEXT("add_bt_simple_parallel"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_simple_parallel"), P); });

	// Task commands
	Registry.RegisterCommand(TEXT("add_bt_task_wait"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_wait"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_move_to"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_move_to"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_play_sound"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_play_sound"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_play_animation"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_play_animation"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_run_behavior"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_run_behavior"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_run_eqs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_run_eqs"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_finish_with_result"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_finish_with_result"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_make_noise"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_make_noise"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_rotate_to_face"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_rotate_to_face"), P); });
	Registry.RegisterCommand(TEXT("add_bt_task_set_key_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_task_set_key_value"), P); });

	// Decorator commands
	Registry.RegisterCommand(TEXT("add_bt_decorator_blackboard"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_blackboard"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_cooldown"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_cooldown"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_loop"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_loop"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_time_limit"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_time_limit"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_force_success"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_force_success"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_compare_bb"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_compare_bb"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_cone_check"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_cone_check"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_does_path_exist"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_does_path_exist"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_is_at_location"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_is_at_location"), P); });
	Registry.RegisterCommand(TEXT("add_bt_decorator_tag_cooldown"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_decorator_tag_cooldown"), P); });

	// Service commands
	Registry.RegisterCommand(TEXT("add_bt_service_default_focus"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_service_default_focus"), P); });
	Registry.RegisterCommand(TEXT("add_bt_service_run_eqs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bt_service_run_eqs"), P); });
}
