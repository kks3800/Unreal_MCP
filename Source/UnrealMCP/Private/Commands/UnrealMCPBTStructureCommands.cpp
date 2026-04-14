// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPBTStructureCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "Misc/PackageName.h"

// Behavior Tree includes
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BTNode.h"
#include "BehaviorTree/BTCompositeNode.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTService.h"

// BT Editor graph includes
#include "BehaviorTreeGraph.h"
#include "BehaviorTreeGraphNode.h"
#include "BehaviorTreeGraphNode_Root.h"
#include "BehaviorTreeGraphNode_Composite.h"
#include "BehaviorTreeGraphNode_Task.h"
#include "BehaviorTreeGraphNode_Decorator.h"
#include "BehaviorTreeGraphNode_Service.h"

// Graph includes
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"

// Reflection
#include "UObject/UnrealType.h"

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// Connection Commands
	if (CommandType == TEXT("connect_bt_nodes"))
	{
		return HandleConnectBTNodes(Params);
	}
	else if (CommandType == TEXT("disconnect_bt_nodes"))
	{
		return HandleDisconnectBTNodes(Params);
	}
	else if (CommandType == TEXT("reorder_bt_children"))
	{
		return HandleReorderBTChildren(Params);
	}
	// Layout Commands
	else if (CommandType == TEXT("delete_bt_node"))
	{
		return HandleDeleteBTNode(Params);
	}
	else if (CommandType == TEXT("move_bt_node"))
	{
		return HandleMoveBTNode(Params);
	}
	else if (CommandType == TEXT("auto_arrange_bt"))
	{
		return HandleAutoArrangeBT(Params);
	}
	// Inspection Commands
	else if (CommandType == TEXT("get_bt_tree_structure"))
	{
		return HandleGetBTTreeStructure(Params);
	}
	else if (CommandType == TEXT("get_bt_node_info"))
	{
		return HandleGetBTNodeInfo(Params);
	}
	else if (CommandType == TEXT("set_bt_node_property"))
	{
		return HandleSetBTNodeProperty(Params);
	}

	return CreateErrorResponse(FString::Printf(TEXT("Unknown BT structure command: %s"), *CommandType));
}

//=============================================================================
// Helper Methods
//=============================================================================

UBehaviorTree* FUnrealMCPBTStructureCommands::LoadBehaviorTree(const FString& TreeName, FString& OutPath)
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

UEdGraphNode* FUnrealMCPBTStructureCommands::FindGraphNodeByIndex(UBehaviorTree* BT, int32 NodeIndex)
{
	if (!BT)
	{
		return nullptr;
	}

#if WITH_EDITORONLY_DATA
	UEdGraph* BTGraph = BT->BTGraph;
	if (!BTGraph)
	{
		return nullptr;
	}

	if (NodeIndex >= 0 && NodeIndex < BTGraph->Nodes.Num())
	{
		return BTGraph->Nodes[NodeIndex];
	}
#endif

	return nullptr;
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::BuildNodeJson(
	UBehaviorTreeGraphNode* BTNode,
	UBehaviorTree* BT)
{
	if (!BTNode)
	{
		return nullptr;
	}

	TSharedPtr<FJsonObject> NodeJson = MakeShared<FJsonObject>();

#if WITH_EDITORONLY_DATA
	UEdGraph* BTGraph = BT->BTGraph;
	if (!BTGraph)
	{
		return nullptr;
	}

	// Find index in graph nodes array
	int32 NodeIndex = BTGraph->Nodes.IndexOfByKey(BTNode);
	NodeJson->SetNumberField(TEXT("index"), NodeIndex);

	// Determine node type string
	FString NodeType;
	if (BTNode->IsA<UBehaviorTreeGraphNode_Root>())
	{
		NodeType = TEXT("Root");
	}
	else if (BTNode->IsA<UBehaviorTreeGraphNode_Composite>())
	{
		NodeType = TEXT("Composite");
		if (BTNode->NodeInstance)
		{
			NodeType = FString::Printf(TEXT("Composite_%s"), *BTNode->NodeInstance->GetClass()->GetName());
		}
	}
	else if (BTNode->IsA<UBehaviorTreeGraphNode_Task>())
	{
		NodeType = TEXT("Task");
		if (BTNode->NodeInstance)
		{
			NodeType = FString::Printf(TEXT("Task_%s"), *BTNode->NodeInstance->GetClass()->GetName());
		}
	}
	else if (BTNode->IsA<UBehaviorTreeGraphNode_Decorator>())
	{
		NodeType = TEXT("Decorator");
		if (BTNode->NodeInstance)
		{
			NodeType = FString::Printf(TEXT("Decorator_%s"), *BTNode->NodeInstance->GetClass()->GetName());
		}
	}
	else if (BTNode->IsA<UBehaviorTreeGraphNode_Service>())
	{
		NodeType = TEXT("Service");
		if (BTNode->NodeInstance)
		{
			NodeType = FString::Printf(TEXT("Service_%s"), *BTNode->NodeInstance->GetClass()->GetName());
		}
	}
	else
	{
		NodeType = BTNode->GetClass()->GetName();
	}
	NodeJson->SetStringField(TEXT("type"), NodeType);

	// Node name / description
	FString NodeName = BTNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString();
	NodeJson->SetStringField(TEXT("name"), NodeName);

	// Position
	TSharedPtr<FJsonObject> PosJson = MakeShared<FJsonObject>();
	PosJson->SetNumberField(TEXT("x"), BTNode->NodePosX);
	PosJson->SetNumberField(TEXT("y"), BTNode->NodePosY);
	NodeJson->SetObjectField(TEXT("position"), PosJson);

	// Decorators
	TArray<TSharedPtr<FJsonValue>> DecoratorArray;
	for (UBehaviorTreeGraphNode* DecoratorNode : BTNode->Decorators)
	{
		if (DecoratorNode)
		{
			int32 DecIndex = BTGraph->Nodes.IndexOfByKey(DecoratorNode);
			TSharedPtr<FJsonObject> DecJson = MakeShared<FJsonObject>();
			DecJson->SetNumberField(TEXT("index"), DecIndex);

			FString DecType = TEXT("Decorator");
			if (DecoratorNode->NodeInstance)
			{
				DecType = FString::Printf(TEXT("Decorator_%s"), *DecoratorNode->NodeInstance->GetClass()->GetName());
			}
			DecJson->SetStringField(TEXT("type"), DecType);
			DecJson->SetStringField(TEXT("name"), DecoratorNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

			DecoratorArray.Add(MakeShared<FJsonValueObject>(DecJson));
		}
	}
	NodeJson->SetArrayField(TEXT("decorators"), DecoratorArray);

	// Services
	TArray<TSharedPtr<FJsonValue>> ServiceArray;
	for (UBehaviorTreeGraphNode* ServiceNode : BTNode->Services)
	{
		if (ServiceNode)
		{
			int32 SvcIndex = BTGraph->Nodes.IndexOfByKey(ServiceNode);
			TSharedPtr<FJsonObject> SvcJson = MakeShared<FJsonObject>();
			SvcJson->SetNumberField(TEXT("index"), SvcIndex);

			FString SvcType = TEXT("Service");
			if (ServiceNode->NodeInstance)
			{
				SvcType = FString::Printf(TEXT("Service_%s"), *ServiceNode->NodeInstance->GetClass()->GetName());
			}
			SvcJson->SetStringField(TEXT("type"), SvcType);
			SvcJson->SetStringField(TEXT("name"), ServiceNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

			ServiceArray.Add(MakeShared<FJsonValueObject>(SvcJson));
		}
	}
	NodeJson->SetArrayField(TEXT("services"), ServiceArray);

	// Children - follow output pin links
	TArray<TSharedPtr<FJsonValue>> ChildrenArray;
	UEdGraphPin* OutputPin = BTNode->GetOutputPin(0);
	if (OutputPin)
	{
		for (UEdGraphPin* LinkedPin : OutputPin->LinkedTo)
		{
			if (LinkedPin && LinkedPin->GetOwningNode())
			{
				UBehaviorTreeGraphNode* ChildBTNode = Cast<UBehaviorTreeGraphNode>(LinkedPin->GetOwningNode());
				if (ChildBTNode)
				{
					TSharedPtr<FJsonObject> ChildJson = BuildNodeJson(ChildBTNode, BT);
					if (ChildJson.IsValid())
					{
						ChildrenArray.Add(MakeShared<FJsonValueObject>(ChildJson));
					}
				}
			}
		}
	}
	NodeJson->SetArrayField(TEXT("children"), ChildrenArray);
#endif

	return NodeJson;
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
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

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

//=============================================================================
// Connection Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleConnectBTNodes(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentIndex = -1;
	if (!Params->TryGetNumberField(TEXT("parent_index"), ParentIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'parent_index' parameter"));
	}

	int32 ChildIndex = -1;
	if (!Params->TryGetNumberField(TEXT("child_index"), ChildIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'child_index' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UEdGraph* BTGraph = BT->BTGraph;
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no graph"));
	}

	UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BT, ParentIndex);
	UEdGraphNode* ChildGraphNode = FindGraphNodeByIndex(BT, ChildIndex);

	if (!ParentGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Parent node not found at index %d"), ParentIndex));
	}
	if (!ChildGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Child node not found at index %d"), ChildIndex));
	}

	// Get output pin from parent and input pin from child
	UAIGraphNode* ParentAINode = Cast<UAIGraphNode>(ParentGraphNode);
	UAIGraphNode* ChildAINode = Cast<UAIGraphNode>(ChildGraphNode);

	if (!ParentAINode || !ChildAINode)
	{
		return CreateErrorResponse(TEXT("Nodes are not valid BT graph nodes"));
	}

	UEdGraphPin* OutputPin = ParentAINode->GetOutputPin(0);
	UEdGraphPin* InputPin = ChildAINode->GetInputPin(0);

	if (!OutputPin)
	{
		return CreateErrorResponse(TEXT("Parent node has no output pin"));
	}
	if (!InputPin)
	{
		return CreateErrorResponse(TEXT("Child node has no input pin"));
	}

	// Make the connection
	OutputPin->MakeLinkTo(InputPin);

	// Notify the graph of the change
	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetNumberField(TEXT("parent_index"), ParentIndex);
	Data->SetNumberField(TEXT("child_index"), ChildIndex);
	Data->SetBoolField(TEXT("connected"), true);
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleDisconnectBTNodes(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentIndex = -1;
	if (!Params->TryGetNumberField(TEXT("parent_index"), ParentIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'parent_index' parameter"));
	}

	int32 ChildIndex = -1;
	if (!Params->TryGetNumberField(TEXT("child_index"), ChildIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'child_index' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UEdGraph* BTGraph = BT->BTGraph;
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no graph"));
	}

	UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BT, ParentIndex);
	UEdGraphNode* ChildGraphNode = FindGraphNodeByIndex(BT, ChildIndex);

	if (!ParentGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Parent node not found at index %d"), ParentIndex));
	}
	if (!ChildGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Child node not found at index %d"), ChildIndex));
	}

	UAIGraphNode* ParentAINode = Cast<UAIGraphNode>(ParentGraphNode);
	UAIGraphNode* ChildAINode = Cast<UAIGraphNode>(ChildGraphNode);

	if (!ParentAINode || !ChildAINode)
	{
		return CreateErrorResponse(TEXT("Nodes are not valid BT graph nodes"));
	}

	UEdGraphPin* OutputPin = ParentAINode->GetOutputPin(0);
	UEdGraphPin* InputPin = ChildAINode->GetInputPin(0);

	if (!OutputPin)
	{
		return CreateErrorResponse(TEXT("Parent node has no output pin"));
	}
	if (!InputPin)
	{
		return CreateErrorResponse(TEXT("Child node has no input pin"));
	}

	// Break the specific link
	OutputPin->BreakLinkTo(InputPin);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetNumberField(TEXT("parent_index"), ParentIndex);
	Data->SetNumberField(TEXT("child_index"), ChildIndex);
	Data->SetBoolField(TEXT("disconnected"), true);
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleReorderBTChildren(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 ParentIndex = -1;
	if (!Params->TryGetNumberField(TEXT("parent_index"), ParentIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'parent_index' parameter"));
	}

	// Get child_order array
	TArray<int32> ChildOrder;
	const TArray<TSharedPtr<FJsonValue>>* ChildOrderJson = nullptr;
	if (!Params->TryGetArrayField(TEXT("child_order"), ChildOrderJson) || !ChildOrderJson)
	{
		return CreateErrorResponse(TEXT("Missing 'child_order' parameter (array of int)"));
	}
	for (const TSharedPtr<FJsonValue>& Val : *ChildOrderJson)
	{
		ChildOrder.Add(static_cast<int32>(Val->AsNumber()));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UEdGraph* BTGraph = BT->BTGraph;
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no graph"));
	}

	UEdGraphNode* ParentGraphNode = FindGraphNodeByIndex(BT, ParentIndex);
	if (!ParentGraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Parent node not found at index %d"), ParentIndex));
	}

	UAIGraphNode* ParentAINode = Cast<UAIGraphNode>(ParentGraphNode);
	if (!ParentAINode)
	{
		return CreateErrorResponse(TEXT("Parent is not a valid BT graph node"));
	}

	UEdGraphPin* OutputPin = ParentAINode->GetOutputPin(0);
	if (!OutputPin)
	{
		return CreateErrorResponse(TEXT("Parent node has no output pin"));
	}

	// Collect currently connected children in order
	TArray<UEdGraphPin*> CurrentChildPins = OutputPin->LinkedTo;
	if (ChildOrder.Num() != CurrentChildPins.Num())
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("child_order length (%d) does not match current child count (%d)"),
			ChildOrder.Num(), CurrentChildPins.Num()));
	}

	// Validate all indices are within range and unique
	TSet<int32> Seen;
	for (int32 Idx : ChildOrder)
	{
		if (Idx < 0 || Idx >= CurrentChildPins.Num())
		{
			return CreateErrorResponse(FString::Printf(
				TEXT("Invalid child order index: %d (valid range: 0-%d)"),
				Idx, CurrentChildPins.Num() - 1));
		}
		if (Seen.Contains(Idx))
		{
			return CreateErrorResponse(FString::Printf(TEXT("Duplicate index in child_order: %d"), Idx));
		}
		Seen.Add(Idx);
	}

	// Snapshot the current child pins in original order
	TArray<UEdGraphPin*> OriginalChildPins = CurrentChildPins;

	// Break all existing child connections
	TArray<UEdGraphPin*> PinsToBreak = OutputPin->LinkedTo;
	for (UEdGraphPin* LinkedPin : PinsToBreak)
	{
		OutputPin->BreakLinkTo(LinkedPin);
	}

	// Reconnect in the new order
	for (int32 NewPos = 0; NewPos < ChildOrder.Num(); ++NewPos)
	{
		int32 OriginalIdx = ChildOrder[NewPos];
		UEdGraphPin* ChildPin = OriginalChildPins[OriginalIdx];
		OutputPin->MakeLinkTo(ChildPin);
	}

	// Rebuild child order on the graph
	UBehaviorTreeGraph* BTGraphTyped = Cast<UBehaviorTreeGraph>(BTGraph);
	if (BTGraphTyped)
	{
		BTGraphTyped->RebuildChildOrder(ParentGraphNode);
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetNumberField(TEXT("parent_index"), ParentIndex);

	TArray<TSharedPtr<FJsonValue>> OrderArray;
	for (int32 Idx : ChildOrder)
	{
		OrderArray.Add(MakeShared<FJsonValueNumber>(Idx));
	}
	Data->SetArrayField(TEXT("new_order"), OrderArray);
	Data->SetNumberField(TEXT("child_count"), ChildOrder.Num());
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

//=============================================================================
// Layout Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleDeleteBTNode(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 NodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("node_index"), NodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'node_index' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UEdGraph* BTGraph = BT->BTGraph;
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no graph"));
	}

	UEdGraphNode* NodeToDelete = FindGraphNodeByIndex(BT, NodeIndex);
	if (!NodeToDelete)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found at index %d"), NodeIndex));
	}

	// Prevent deletion of root node
	if (NodeToDelete->IsA<UBehaviorTreeGraphNode_Root>())
	{
		return CreateErrorResponse(TEXT("Cannot delete the root node"));
	}

	// Check if node allows deletion
	if (!NodeToDelete->CanUserDeleteNode())
	{
		return CreateErrorResponse(TEXT("This node cannot be deleted"));
	}

	// Capture info before deletion
	FString DeletedNodeName = NodeToDelete->GetNodeTitle(ENodeTitleType::FullTitle).ToString();

	// Break all pin connections before removal
	NodeToDelete->BreakAllNodeLinks();

	// Remove the node from the graph
	BTGraph->RemoveNode(NodeToDelete);

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetNumberField(TEXT("deleted_index"), NodeIndex);
	Data->SetStringField(TEXT("deleted_node"), DeletedNodeName);
	Data->SetNumberField(TEXT("remaining_nodes"), BTGraph->Nodes.Num());
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleMoveBTNode(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 NodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("node_index"), NodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'node_index' parameter"));
	}

	double X = 0.0;
	if (!Params->TryGetNumberField(TEXT("x"), X))
	{
		return CreateErrorResponse(TEXT("Missing 'x' parameter"));
	}

	double Y = 0.0;
	if (!Params->TryGetNumberField(TEXT("y"), Y))
	{
		return CreateErrorResponse(TEXT("Missing 'y' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UEdGraphNode* GraphNode = FindGraphNodeByIndex(BT, NodeIndex);
	if (!GraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found at index %d"), NodeIndex));
	}

	GraphNode->NodePosX = static_cast<int32>(X);
	GraphNode->NodePosY = static_cast<int32>(Y);

	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetNumberField(TEXT("node_index"), NodeIndex);
	Data->SetNumberField(TEXT("x"), GraphNode->NodePosX);
	Data->SetNumberField(TEXT("y"), GraphNode->NodePosY);
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleAutoArrangeBT(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
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

	UBehaviorTreeGraph* BTGraph = Cast<UBehaviorTreeGraph>(BT->BTGraph);
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no graph"));
	}

	// The built-in AutoArrange() relies on DEPRECATED_NodeWidget which requires
	// an active SGraphPanel with live widgets. Programmatically-created nodes
	// don't have widgets, so we use a manual tree layout instead.

	// Find root node
	UBehaviorTreeGraphNode_Root* RootNode = nullptr;
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		UBehaviorTreeGraphNode_Root* AsRoot = Cast<UBehaviorTreeGraphNode_Root>(Node);
		if (AsRoot)
		{
			RootNode = AsRoot;
			break;
		}
	}

	if (!RootNode)
	{
		return CreateErrorResponse(TEXT("No root node found in BT graph"));
	}

	// BFS to compute tree layout: each node gets a depth and horizontal slot
	constexpr int32 NodeWidth = 280;
	constexpr int32 NodeSpacingX = 40;
	constexpr int32 LevelSpacingY = 200;
	constexpr int32 DecoratorOffsetY = -60;

	struct FLayoutEntry
	{
		UEdGraphNode* Node;
		int32 Depth;
	};

	// BFS traversal collecting nodes by depth
	TArray<FLayoutEntry> LayoutQueue;
	TMap<UEdGraphNode*, int32> DepthMap;
	TMap<int32, TArray<UEdGraphNode*>> NodesByDepth;

	LayoutQueue.Add({RootNode, 0});
	DepthMap.Add(RootNode, 0);

	int32 QueueIdx = 0;
	while (QueueIdx < LayoutQueue.Num())
	{
		FLayoutEntry Current = LayoutQueue[QueueIdx++];
		NodesByDepth.FindOrAdd(Current.Depth).Add(Current.Node);

		// Follow output pins to find children
		for (UEdGraphPin* Pin : Current.Node->Pins)
		{
			if (Pin->Direction != EGPD_Output)
			{
				continue;
			}
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				UEdGraphNode* ChildNode = LinkedPin->GetOwningNode();
				if (ChildNode && !DepthMap.Contains(ChildNode))
				{
					int32 ChildDepth = Current.Depth + 1;
					DepthMap.Add(ChildNode, ChildDepth);
					LayoutQueue.Add({ChildNode, ChildDepth});
				}
			}
		}
	}

	// Position nodes: center each level horizontally, stack vertically
	int32 MaxDepth = 0;
	for (const auto& Pair : NodesByDepth)
	{
		if (Pair.Key > MaxDepth)
		{
			MaxDepth = Pair.Key;
		}
	}

	for (const auto& Pair : NodesByDepth)
	{
		int32 Depth = Pair.Key;
		const TArray<UEdGraphNode*>& NodesAtDepth = Pair.Value;
		int32 TotalWidth = NodesAtDepth.Num() * NodeWidth + (NodesAtDepth.Num() - 1) * NodeSpacingX;
		int32 StartX = -TotalWidth / 2;

		for (int32 i = 0; i < NodesAtDepth.Num(); ++i)
		{
			UEdGraphNode* Node = NodesAtDepth[i];
			Node->NodePosX = StartX + i * (NodeWidth + NodeSpacingX);
			Node->NodePosY = Depth * LevelSpacingY;

			// Offset decorators/services above their parent
			UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(Node);
			if (BTNode)
			{
				for (int32 SubIdx = 0; SubIdx < BTNode->Decorators.Num(); ++SubIdx)
				{
					UBehaviorTreeGraphNode* Dec = Cast<UBehaviorTreeGraphNode>(BTNode->Decorators[SubIdx]);
					if (Dec)
					{
						Dec->NodePosX = Node->NodePosX;
						Dec->NodePosY = Node->NodePosY + DecoratorOffsetY * (SubIdx + 1);
					}
				}
				for (int32 SubIdx = 0; SubIdx < BTNode->Services.Num(); ++SubIdx)
				{
					UBehaviorTreeGraphNode* Svc = Cast<UBehaviorTreeGraphNode>(BTNode->Services[SubIdx]);
					if (Svc)
					{
						Svc->NodePosX = Node->NodePosX + NodeWidth / 2;
						Svc->NodePosY = Node->NodePosY + DecoratorOffsetY * (SubIdx + 1);
					}
				}
			}
		}
	}

	BTGraph->NotifyGraphChanged();
	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetNumberField(TEXT("node_count"), BTGraph->Nodes.Num());
	Data->SetBoolField(TEXT("arranged"), true);
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

//=============================================================================
// Inspection Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleGetBTTreeStructure(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
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

	UEdGraph* BTGraph = BT->BTGraph;
	if (!BTGraph)
	{
		return CreateErrorResponse(TEXT("Behavior Tree has no graph"));
	}

	// Find the root node
	UBehaviorTreeGraphNode_Root* RootNode = nullptr;
	for (UEdGraphNode* Node : BTGraph->Nodes)
	{
		RootNode = Cast<UBehaviorTreeGraphNode_Root>(Node);
		if (RootNode)
		{
			break;
		}
	}

	if (!RootNode)
	{
		return CreateErrorResponse(TEXT("Root node not found in behavior tree graph"));
	}

	// Build recursive tree structure starting from root
	TSharedPtr<FJsonObject> TreeJson = BuildNodeJson(RootNode, BT);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetStringField(TEXT("path"), AssetPath);
	Data->SetNumberField(TEXT("total_graph_nodes"), BTGraph->Nodes.Num());
	if (TreeJson.IsValid())
	{
		Data->SetObjectField(TEXT("root"), TreeJson);
	}

	// Also provide a flat list of all node indices and types for reference
	TArray<TSharedPtr<FJsonValue>> FlatList;
	for (int32 i = 0; i < BTGraph->Nodes.Num(); ++i)
	{
		UEdGraphNode* Node = BTGraph->Nodes[i];
		if (!Node)
		{
			continue;
		}

		TSharedPtr<FJsonObject> FlatNode = MakeShared<FJsonObject>();
		FlatNode->SetNumberField(TEXT("index"), i);
		FlatNode->SetStringField(TEXT("class"), Node->GetClass()->GetName());
		FlatNode->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
		FlatList.Add(MakeShared<FJsonValueObject>(FlatNode));
	}
	Data->SetArrayField(TEXT("all_nodes"), FlatList);

	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleGetBTNodeInfo(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 NodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("node_index"), NodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'node_index' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UEdGraphNode* GraphNode = FindGraphNodeByIndex(BT, NodeIndex);
	if (!GraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found at index %d"), NodeIndex));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetNumberField(TEXT("index"), NodeIndex);
	Data->SetStringField(TEXT("class"), GraphNode->GetClass()->GetName());
	Data->SetStringField(TEXT("title"), GraphNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
	Data->SetStringField(TEXT("tooltip"), GraphNode->GetTooltipText().ToString());
	Data->SetNumberField(TEXT("x"), GraphNode->NodePosX);
	Data->SetNumberField(TEXT("y"), GraphNode->NodePosY);
	Data->SetBoolField(TEXT("can_delete"), GraphNode->CanUserDeleteNode());
	Data->SetBoolField(TEXT("can_duplicate"), GraphNode->CanDuplicateNode());

	// Node GUID
	Data->SetStringField(TEXT("guid"), GraphNode->NodeGuid.ToString());

	// Pin information
	TArray<TSharedPtr<FJsonValue>> PinArray;
	for (UEdGraphPin* Pin : GraphNode->Pins)
	{
		if (!Pin)
		{
			continue;
		}

		TSharedPtr<FJsonObject> PinJson = MakeShared<FJsonObject>();
		PinJson->SetStringField(TEXT("name"), Pin->PinName.ToString());
		PinJson->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("Input") : TEXT("Output"));
		PinJson->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());
		PinJson->SetNumberField(TEXT("connections"), Pin->LinkedTo.Num());

		// List connected node indices
		TArray<TSharedPtr<FJsonValue>> ConnectedIndices;
		UEdGraph* BTGraph = BT->BTGraph;
		for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
		{
			if (LinkedPin && LinkedPin->GetOwningNode() && BTGraph)
			{
				int32 LinkedIndex = BTGraph->Nodes.IndexOfByKey(LinkedPin->GetOwningNode());
				ConnectedIndices.Add(MakeShared<FJsonValueNumber>(LinkedIndex));
			}
		}
		PinJson->SetArrayField(TEXT("connected_to"), ConnectedIndices);

		PinArray.Add(MakeShared<FJsonValueObject>(PinJson));
	}
	Data->SetArrayField(TEXT("pins"), PinArray);

	// BT-specific info: sub-nodes (decorators, services)
	UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	if (BTNode)
	{
		// Description from BT node
		Data->SetStringField(TEXT("description"), BTNode->GetDescription().ToString());

		// Decorators
		TArray<TSharedPtr<FJsonValue>> DecArray;
		for (UBehaviorTreeGraphNode* Dec : BTNode->Decorators)
		{
			if (!Dec)
			{
				continue;
			}
			TSharedPtr<FJsonObject> DecJson = MakeShared<FJsonObject>();
			UEdGraph* BTGraph = BT->BTGraph;
			DecJson->SetNumberField(TEXT("index"), BTGraph ? BTGraph->Nodes.IndexOfByKey(Dec) : -1);
			DecJson->SetStringField(TEXT("class"), Dec->GetClass()->GetName());
			DecJson->SetStringField(TEXT("name"), Dec->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			if (Dec->NodeInstance)
			{
				DecJson->SetStringField(TEXT("instance_class"), Dec->NodeInstance->GetClass()->GetName());
			}
			DecArray.Add(MakeShared<FJsonValueObject>(DecJson));
		}
		Data->SetArrayField(TEXT("decorators"), DecArray);

		// Services
		TArray<TSharedPtr<FJsonValue>> SvcArray;
		for (UBehaviorTreeGraphNode* Svc : BTNode->Services)
		{
			if (!Svc)
			{
				continue;
			}
			TSharedPtr<FJsonObject> SvcJson = MakeShared<FJsonObject>();
			UEdGraph* BTGraph = BT->BTGraph;
			SvcJson->SetNumberField(TEXT("index"), BTGraph ? BTGraph->Nodes.IndexOfByKey(Svc) : -1);
			SvcJson->SetStringField(TEXT("class"), Svc->GetClass()->GetName());
			SvcJson->SetStringField(TEXT("name"), Svc->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			if (Svc->NodeInstance)
			{
				SvcJson->SetStringField(TEXT("instance_class"), Svc->NodeInstance->GetClass()->GetName());
			}
			SvcArray.Add(MakeShared<FJsonValueObject>(SvcJson));
		}
		Data->SetArrayField(TEXT("services"), SvcArray);

		// Node instance properties (editable UPROPERTYs)
		if (BTNode->NodeInstance)
		{
			UObject* Instance = BTNode->NodeInstance;
			Data->SetStringField(TEXT("instance_class"), Instance->GetClass()->GetName());

			TArray<TSharedPtr<FJsonValue>> PropArray;
			for (TFieldIterator<FProperty> PropIt(Instance->GetClass()); PropIt; ++PropIt)
			{
				FProperty* Prop = *PropIt;
				if (!Prop->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
				{
					continue;
				}

				TSharedPtr<FJsonObject> PropJson = MakeShared<FJsonObject>();
				PropJson->SetStringField(TEXT("name"), Prop->GetName());
				PropJson->SetStringField(TEXT("type"), Prop->GetCPPType());
				PropJson->SetStringField(TEXT("category"), Prop->GetMetaData(TEXT("Category")));

				// Get current value as string
				FString ValueStr;
				const void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Instance);
				Prop->ExportTextItem_Direct(ValueStr, ValuePtr, nullptr, Instance, PPF_None);
				PropJson->SetStringField(TEXT("value"), ValueStr);

				PropArray.Add(MakeShared<FJsonValueObject>(PropJson));
			}
			Data->SetArrayField(TEXT("properties"), PropArray);
		}
	}

	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPBTStructureCommands::HandleSetBTNodeProperty(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString TreeName;
	if (!Params->TryGetStringField(TEXT("tree_name"), TreeName))
	{
		return CreateErrorResponse(TEXT("Missing 'tree_name' parameter"));
	}

	int32 NodeIndex = -1;
	if (!Params->TryGetNumberField(TEXT("node_index"), NodeIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'node_index' parameter"));
	}

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
	}

	if (!Params->HasField(TEXT("value")))
	{
		return CreateErrorResponse(TEXT("Missing 'value' parameter"));
	}

	FString AssetPath;
	UBehaviorTree* BT = LoadBehaviorTree(TreeName, AssetPath);
	if (!BT)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Behavior Tree not found: %s"), *TreeName));
	}

	UEdGraphNode* GraphNode = FindGraphNodeByIndex(BT, NodeIndex);
	if (!GraphNode)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found at index %d"), NodeIndex));
	}

	UBehaviorTreeGraphNode* BTNode = Cast<UBehaviorTreeGraphNode>(GraphNode);
	if (!BTNode)
	{
		return CreateErrorResponse(TEXT("Node is not a valid BT graph node"));
	}

	UObject* NodeInstance = BTNode->NodeInstance;
	if (!NodeInstance)
	{
		return CreateErrorResponse(TEXT("Node has no instance object (root nodes have no editable properties)"));
	}

	// Find the property using UE reflection
	FProperty* Property = NodeInstance->GetClass()->FindPropertyByName(FName(*PropertyName));
	if (!Property)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Property '%s' not found on node class '%s'"),
			*PropertyName, *NodeInstance->GetClass()->GetName()));
	}

	void* ValuePtr = Property->ContainerPtrToValuePtr<void>(NodeInstance);

	// Get the JSON value
	TSharedPtr<FJsonValue> JsonValue = Params->TryGetField(TEXT("value"));

	// Set the property based on its type
	bool bSuccess = false;
	FString ResultValueStr;

	if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Property))
	{
		float FloatVal = static_cast<float>(JsonValue->AsNumber());
		FloatProp->SetPropertyValue(ValuePtr, FloatVal);
		ResultValueStr = FString::SanitizeFloat(FloatVal);
		bSuccess = true;
	}
	else if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Property))
	{
		double DoubleVal = JsonValue->AsNumber();
		DoubleProp->SetPropertyValue(ValuePtr, DoubleVal);
		ResultValueStr = FString::SanitizeFloat(DoubleVal);
		bSuccess = true;
	}
	else if (FIntProperty* IntProp = CastField<FIntProperty>(Property))
	{
		int32 IntVal = static_cast<int32>(JsonValue->AsNumber());
		IntProp->SetPropertyValue(ValuePtr, IntVal);
		ResultValueStr = FString::FromInt(IntVal);
		bSuccess = true;
	}
	else if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		bool BoolVal = JsonValue->AsBool();
		BoolProp->SetPropertyValue(ValuePtr, BoolVal);
		ResultValueStr = BoolVal ? TEXT("true") : TEXT("false");
		bSuccess = true;
	}
	else if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		FString StrVal = JsonValue->AsString();
		StrProp->SetPropertyValue(ValuePtr, StrVal);
		ResultValueStr = StrVal;
		bSuccess = true;
	}
	else if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		FName NameVal = FName(*JsonValue->AsString());
		NameProp->SetPropertyValue(ValuePtr, NameVal);
		ResultValueStr = NameVal.ToString();
		bSuccess = true;
	}
	else if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		FText TextVal = FText::FromString(JsonValue->AsString());
		TextProp->SetPropertyValue(ValuePtr, TextVal);
		ResultValueStr = TextVal.ToString();
		bSuccess = true;
	}
	else if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		uint8 ByteVal = static_cast<uint8>(JsonValue->AsNumber());
		ByteProp->SetPropertyValue(ValuePtr, ByteVal);
		ResultValueStr = FString::FromInt(ByteVal);
		bSuccess = true;
	}
	else if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		// Try to set by name or numeric value
		FString EnumValueStr = JsonValue->AsString();
		UEnum* Enum = EnumProp->GetEnum();
		if (Enum)
		{
			int64 EnumValue = Enum->GetValueByNameString(EnumValueStr);
			if (EnumValue == INDEX_NONE)
			{
				// Try as numeric
				EnumValue = static_cast<int64>(JsonValue->AsNumber());
			}
			FNumericProperty* UnderlyingProp = EnumProp->GetUnderlyingProperty();
			if (UnderlyingProp)
			{
				UnderlyingProp->SetIntPropertyValue(ValuePtr, EnumValue);
				ResultValueStr = Enum->GetNameStringByValue(EnumValue);
				bSuccess = true;
			}
		}
	}
	else
	{
		// Fallback: try ImportText for complex types
		FString ImportStr = JsonValue->AsString();
		const TCHAR* ImportResult = Property->ImportText_Direct(*ImportStr, ValuePtr, NodeInstance, PPF_None);
		if (ImportResult)
		{
			Property->ExportTextItem_Direct(ResultValueStr, ValuePtr, nullptr, NodeInstance, PPF_None);
			bSuccess = true;
		}
	}

	if (!bSuccess)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to set property '%s' of type '%s'"),
			*PropertyName, *Property->GetCPPType()));
	}

	// Notify the node instance that a property changed
	FPropertyChangedEvent PropertyChangedEvent(Property);
	NodeInstance->PostEditChangeProperty(PropertyChangedEvent);

	BT->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("tree"), TreeName);
	Data->SetNumberField(TEXT("node_index"), NodeIndex);
	Data->SetStringField(TEXT("property"), PropertyName);
	Data->SetStringField(TEXT("value"), ResultValueStr);
	Data->SetStringField(TEXT("type"), Property->GetCPPType());
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Editor-only functionality not available"));
#endif
}

//=============================================================================
// Command Registration
//=============================================================================

void FUnrealMCPBTStructureCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// Connection Commands
	Registry.RegisterCommand(TEXT("connect_bt_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_bt_nodes"), P); });
	Registry.RegisterCommand(TEXT("disconnect_bt_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("disconnect_bt_nodes"), P); });
	Registry.RegisterCommand(TEXT("reorder_bt_children"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("reorder_bt_children"), P); });

	// Layout Commands
	Registry.RegisterCommand(TEXT("delete_bt_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_bt_node"), P); });
	Registry.RegisterCommand(TEXT("move_bt_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("move_bt_node"), P); });
	Registry.RegisterCommand(TEXT("auto_arrange_bt"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("auto_arrange_bt"), P); });

	// Inspection Commands
	Registry.RegisterCommand(TEXT("get_bt_tree_structure"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_bt_tree_structure"), P); });
	Registry.RegisterCommand(TEXT("get_bt_node_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_bt_node_info"), P); });
	Registry.RegisterCommand(TEXT("set_bt_node_property"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_bt_node_property"), P); });
}
