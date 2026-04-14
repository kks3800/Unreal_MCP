#include "Commands/UnrealMCPBlueprintGraphCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Tunnel.h"

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_function_graph"))
	{
		return HandleCreateFunctionGraph(Params);
	}
	else if (CommandType == TEXT("delete_function_graph"))
	{
		return HandleDeleteFunctionGraph(Params);
	}
	else if (CommandType == TEXT("create_macro_graph"))
	{
		return HandleCreateMacroGraph(Params);
	}
	else if (CommandType == TEXT("delete_macro_graph"))
	{
		return HandleDeleteMacroGraph(Params);
	}
	else if (CommandType == TEXT("auto_layout_graph"))
	{
		return HandleAutoLayoutGraph(Params);
	}
	// Sprint 6: Function-level operations
	else if (CommandType == TEXT("set_function_access_specifier"))
	{
		return HandleSetFunctionAccessSpecifier(Params);
	}
	else if (CommandType == TEXT("set_function_flags"))
	{
		return HandleSetFunctionFlags(Params);
	}
	else if (CommandType == TEXT("add_local_variable"))
	{
		return HandleAddLocalVariable(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint graph command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleCreateFunctionGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Check if a function graph with this name already exists
	UEdGraph* ExistingGraph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, FunctionName);
	if (ExistingGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph with name '%s' already exists"), *FunctionName));
	}

	// Create the function graph
	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(*FunctionName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!NewGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create function graph"));
	}

	// Add to blueprint (this also creates entry/result nodes automatically)
	FBlueprintEditorUtils::AddFunctionGraph(Blueprint, NewGraph, true, (UClass*)nullptr);

	// Find the entry and result nodes
	UK2Node_FunctionEntry* EntryNode = nullptr;
	UK2Node_FunctionResult* ResultNode = nullptr;
	for (UEdGraphNode* Node : NewGraph->Nodes)
	{
		if (!EntryNode)
		{
			EntryNode = Cast<UK2Node_FunctionEntry>(Node);
		}
		if (!ResultNode)
		{
			ResultNode = Cast<UK2Node_FunctionResult>(Node);
		}
		if (EntryNode && ResultNode)
		{
			break;
		}
	}

	// Handle is_pure flag
	bool bIsPure = false;
	if (Params->HasField(TEXT("is_pure")))
	{
		bIsPure = Params->GetBoolField(TEXT("is_pure"));
	}
	if (bIsPure && EntryNode)
	{
		EntryNode->AddExtraFlags(FUNC_BlueprintPure);
		// Remove the exec pins for pure functions
		EntryNode->ReconstructNode();
	}

	// Handle input parameters
	const TArray<TSharedPtr<FJsonValue>>* InputsArray = nullptr;
	if (Params->TryGetArrayField(TEXT("inputs"), InputsArray) && EntryNode)
	{
		for (const TSharedPtr<FJsonValue>& Input : *InputsArray)
		{
			const TSharedPtr<FJsonObject>* InputObj = nullptr;
			if (Input->TryGetObject(InputObj))
			{
				FString ParamName = (*InputObj)->GetStringField(TEXT("name"));
				FString ParamType = (*InputObj)->GetStringField(TEXT("type"));

				FEdGraphPinType PinType;
				if (ParamType == TEXT("Boolean"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
				}
				else if (ParamType == TEXT("Integer") || ParamType == TEXT("Int"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
				}
				else if (ParamType == TEXT("Float") || ParamType == TEXT("Double"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
					PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
				}
				else if (ParamType == TEXT("String"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_String;
				}
				else if (ParamType == TEXT("Name"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
				}
				else if (ParamType == TEXT("Text"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
				}
				else if (ParamType == TEXT("Vector"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
					PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
				}
				else if (ParamType == TEXT("Rotator"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
					PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
				}
				else if (ParamType == TEXT("Transform"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
					PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
				}
				else
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
					PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
				}

				TSharedPtr<FUserPinInfo> NewPin = MakeShared<FUserPinInfo>();
				NewPin->PinName = FName(*ParamName);
				NewPin->PinType = PinType;
				NewPin->DesiredPinDirection = EGPD_Output;
				EntryNode->UserDefinedPins.Add(NewPin);
			}
		}
		EntryNode->ReconstructNode();
	}

	// Handle output parameters
	const TArray<TSharedPtr<FJsonValue>>* OutputsArray = nullptr;
	if (Params->TryGetArrayField(TEXT("outputs"), OutputsArray))
	{
		// If no result node exists yet, create one
		if (!ResultNode)
		{
			ResultNode = NewObject<UK2Node_FunctionResult>(NewGraph);
			ResultNode->NodePosX = 400;
			ResultNode->NodePosY = 0;
			NewGraph->AddNode(ResultNode, true);
			ResultNode->CreateNewGuid();
			ResultNode->PostPlacedNewNode();
			ResultNode->AllocateDefaultPins();
		}

		for (const TSharedPtr<FJsonValue>& Output : *OutputsArray)
		{
			const TSharedPtr<FJsonObject>* OutputObj = nullptr;
			if (Output->TryGetObject(OutputObj))
			{
				FString ParamName = (*OutputObj)->GetStringField(TEXT("name"));
				FString ParamType = (*OutputObj)->GetStringField(TEXT("type"));

				FEdGraphPinType PinType;
				if (ParamType == TEXT("Boolean"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
				}
				else if (ParamType == TEXT("Integer") || ParamType == TEXT("Int"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
				}
				else if (ParamType == TEXT("Float") || ParamType == TEXT("Double"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
					PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
				}
				else if (ParamType == TEXT("String"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_String;
				}
				else if (ParamType == TEXT("Name"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
				}
				else if (ParamType == TEXT("Text"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
				}
				else if (ParamType == TEXT("Vector"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
					PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
				}
				else if (ParamType == TEXT("Rotator"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
					PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
				}
				else if (ParamType == TEXT("Transform"))
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
					PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
				}
				else
				{
					PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
					PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
				}

				TSharedPtr<FUserPinInfo> NewPin = MakeShared<FUserPinInfo>();
				NewPin->PinName = FName(*ParamName);
				NewPin->PinType = PinType;
				NewPin->DesiredPinDirection = EGPD_Input;
				ResultNode->UserDefinedPins.Add(NewPin);
			}
		}
		ResultNode->ReconstructNode();
	}

	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("graph_name"), FunctionName);
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Function graph '%s' created successfully"), *FunctionName));
	if (EntryNode)
	{
		ResultObj->SetStringField(TEXT("entry_node_guid"), EntryNode->NodeGuid.ToString());
	}
	if (ResultNode)
	{
		ResultObj->SetStringField(TEXT("result_node_guid"), ResultNode->NodeGuid.ToString());
	}
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleDeleteFunctionGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Search function graphs specifically
	UEdGraph* GraphToRemove = nullptr;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph->GetName() == FunctionName)
		{
			GraphToRemove = Graph;
			break;
		}
	}

	if (!GraphToRemove)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function graph not found: %s"), *FunctionName));
	}

	FBlueprintEditorUtils::RemoveGraph(Blueprint, GraphToRemove);
	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Function graph '%s' deleted successfully"), *FunctionName));
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleCreateMacroGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString MacroName;
	if (!Params->TryGetStringField(TEXT("macro_name"), MacroName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'macro_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Check if a macro graph with this name already exists
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph->GetName() == MacroName)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Macro graph '%s' already exists"), *MacroName));
		}
	}

	UEdGraph* NewGraph = FBlueprintEditorUtils::CreateNewGraph(Blueprint, FName(*MacroName), UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!NewGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create macro graph"));
	}

	FBlueprintEditorUtils::AddMacroGraph(Blueprint, NewGraph, true, nullptr);
	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("graph_name"), MacroName);
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Macro graph '%s' created successfully"), *MacroName));
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleDeleteMacroGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString MacroName;
	if (!Params->TryGetStringField(TEXT("macro_name"), MacroName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'macro_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Search macro graphs specifically
	UEdGraph* GraphToRemove = nullptr;
	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		if (Graph->GetName() == MacroName)
		{
			GraphToRemove = Graph;
			break;
		}
	}

	if (!GraphToRemove)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Macro graph not found: %s"), *MacroName));
	}

	FBlueprintEditorUtils::RemoveGraph(Blueprint, GraphToRemove);
	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Macro graph '%s' deleted successfully"), *MacroName));
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleAutoLayoutGraph(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString GraphName;
	if (!Params->TryGetStringField(TEXT("graph_name"), GraphName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'graph_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	if (Graph->Nodes.Num() == 0)
	{
		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetStringField(TEXT("message"), TEXT("Graph has no nodes to layout"));
		return ResultObj;
	}

	// Simple column-based auto-layout using BFS
	const int32 HorizontalSpacing = 300;
	const int32 VerticalSpacing = 200;

	// Build adjacency: find which nodes connect to which via exec pins
	TMap<UEdGraphNode*, TArray<UEdGraphNode*>> Adjacency;
	TSet<UEdGraphNode*> HasIncomingExec;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->Direction == EGPD_Output && Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec)
			{
				for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
				{
					UEdGraphNode* TargetNode = LinkedPin->GetOwningNode();
					Adjacency.FindOrAdd(Node).AddUnique(TargetNode);
					HasIncomingExec.Add(TargetNode);
				}
			}
		}
	}

	// Find root nodes (no incoming exec connections)
	TArray<UEdGraphNode*> Roots;
	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!HasIncomingExec.Contains(Node))
		{
			Roots.Add(Node);
		}
	}

	// If no roots found (circular graph), use first node
	if (Roots.Num() == 0 && Graph->Nodes.Num() > 0)
	{
		Roots.Add(Graph->Nodes[0]);
	}

	// BFS to assign layers
	TMap<UEdGraphNode*, int32> NodeLayer;
	TSet<UEdGraphNode*> Visited;
	TQueue<UEdGraphNode*> Queue;

	for (UEdGraphNode* Root : Roots)
	{
		if (!Visited.Contains(Root))
		{
			Queue.Enqueue(Root);
			Visited.Add(Root);
			NodeLayer.Add(Root, 0);
		}
	}

	while (!Queue.IsEmpty())
	{
		UEdGraphNode* Current = nullptr;
		Queue.Dequeue(Current);
		int32 CurrentLayer = NodeLayer[Current];

		TArray<UEdGraphNode*>* Neighbors = Adjacency.Find(Current);
		if (Neighbors)
		{
			for (UEdGraphNode* Neighbor : *Neighbors)
			{
				if (!Visited.Contains(Neighbor))
				{
					Visited.Add(Neighbor);
					NodeLayer.Add(Neighbor, CurrentLayer + 1);
					Queue.Enqueue(Neighbor);
				}
			}
		}
	}

	// Assign any unvisited nodes to their own layers at the end
	int32 MaxLayer = 0;
	for (const TPair<UEdGraphNode*, int32>& Pair : NodeLayer)
	{
		if (Pair.Value > MaxLayer)
		{
			MaxLayer = Pair.Value;
		}
	}

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!NodeLayer.Contains(Node))
		{
			MaxLayer++;
			NodeLayer.Add(Node, MaxLayer);
		}
	}

	// Group nodes by layer
	TMap<int32, TArray<UEdGraphNode*>> Layers;
	for (const TPair<UEdGraphNode*, int32>& Pair : NodeLayer)
	{
		Layers.FindOrAdd(Pair.Value).Add(Pair.Key);
	}

	// Position nodes
	int32 NodesPositioned = 0;
	for (TPair<int32, TArray<UEdGraphNode*>>& LayerPair : Layers)
	{
		int32 Layer = LayerPair.Key;
		TArray<UEdGraphNode*>& LayerNodes = LayerPair.Value;

		for (int32 i = 0; i < LayerNodes.Num(); i++)
		{
			LayerNodes[i]->NodePosX = Layer * HorizontalSpacing;
			LayerNodes[i]->NodePosY = i * VerticalSpacing;
			NodesPositioned++;
		}
	}

	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Auto-layout applied to %d nodes in graph '%s'"), NodesPositioned, *GraphName));
	ResultObj->SetNumberField(TEXT("nodes_positioned"), NodesPositioned);
	return ResultObj;
}

//=============================================================================
// Sprint 6: Function-level operations
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleSetFunctionAccessSpecifier(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	FString Access;
	if (!Params->TryGetStringField(TEXT("access"), Access))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'access' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Find the function graph
	UEdGraph* FuncGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph->GetName() == FunctionName)
		{
			FuncGraph = Graph;
			break;
		}
	}

	if (!FuncGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function graph not found: %s"), *FunctionName));
	}

	// Find the entry node to modify flags
	UK2Node_FunctionEntry* EntryNode = nullptr;
	for (UEdGraphNode* Node : FuncGraph->Nodes)
	{
		EntryNode = Cast<UK2Node_FunctionEntry>(Node);
		if (EntryNode)
		{
			break;
		}
	}

	if (!EntryNode)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Function entry node not found"));
	}

	// Set access specifier via function flags
	uint32 CurrentFlags = EntryNode->GetExtraFlags();

	// Clear existing access flags
	CurrentFlags &= ~(FUNC_Public | FUNC_Protected | FUNC_Private);

	if (Access == TEXT("Public"))
	{
		CurrentFlags |= FUNC_Public;
	}
	else if (Access == TEXT("Protected"))
	{
		CurrentFlags |= FUNC_Protected;
	}
	else if (Access == TEXT("Private"))
	{
		CurrentFlags |= FUNC_Private;
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Invalid access specifier: %s. Use 'Public', 'Protected', or 'Private'."), *Access));
	}

	EntryNode->SetExtraFlags(CurrentFlags);
	EntryNode->ReconstructNode();
	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Function '%s' access set to %s"), *FunctionName, *Access));
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleSetFunctionFlags(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* FuncGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph->GetName() == FunctionName)
		{
			FuncGraph = Graph;
			break;
		}
	}

	if (!FuncGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function graph not found: %s"), *FunctionName));
	}

	UK2Node_FunctionEntry* EntryNode = nullptr;
	for (UEdGraphNode* Node : FuncGraph->Nodes)
	{
		EntryNode = Cast<UK2Node_FunctionEntry>(Node);
		if (EntryNode)
		{
			break;
		}
	}

	if (!EntryNode)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Function entry node not found"));
	}

	TArray<FString> FlagsSet;

	if (Params->HasField(TEXT("is_pure")))
	{
		bool bPure = Params->GetBoolField(TEXT("is_pure"));
		if (bPure)
		{
			EntryNode->AddExtraFlags(FUNC_BlueprintPure);
			FlagsSet.Add(TEXT("BlueprintPure"));
		}
		else
		{
			uint32 Flags = EntryNode->GetExtraFlags();
			Flags &= ~FUNC_BlueprintPure;
			EntryNode->SetExtraFlags(Flags);
			FlagsSet.Add(TEXT("~BlueprintPure"));
		}
	}

	if (Params->HasField(TEXT("is_const")))
	{
		bool bConst = Params->GetBoolField(TEXT("is_const"));
		if (bConst)
		{
			EntryNode->AddExtraFlags(FUNC_Const);
			FlagsSet.Add(TEXT("Const"));
		}
		else
		{
			uint32 Flags = EntryNode->GetExtraFlags();
			Flags &= ~FUNC_Const;
			EntryNode->SetExtraFlags(Flags);
			FlagsSet.Add(TEXT("~Const"));
		}
	}

	if (Params->HasField(TEXT("is_static")))
	{
		bool bStatic = Params->GetBoolField(TEXT("is_static"));
		if (bStatic)
		{
			EntryNode->AddExtraFlags(FUNC_Static);
			FlagsSet.Add(TEXT("Static"));
		}
		else
		{
			uint32 Flags = EntryNode->GetExtraFlags();
			Flags &= ~FUNC_Static;
			EntryNode->SetExtraFlags(Flags);
			FlagsSet.Add(TEXT("~Static"));
		}
	}

	if (Params->HasField(TEXT("is_callable_in_editor")))
	{
		bool bCallable = Params->GetBoolField(TEXT("is_callable_in_editor"));
		if (bCallable)
		{
			EntryNode->AddExtraFlags(FUNC_EditorOnly);
			FlagsSet.Add(TEXT("EditorOnly"));
		}
		else
		{
			uint32 Flags = EntryNode->GetExtraFlags();
			Flags &= ~FUNC_EditorOnly;
			EntryNode->SetExtraFlags(Flags);
			FlagsSet.Add(TEXT("~EditorOnly"));
		}
	}

	EntryNode->ReconstructNode();
	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Function '%s' flags updated: %s"), *FunctionName, *FString::Join(FlagsSet, TEXT(", "))));
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintGraphCommands::HandleAddLocalVariable(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	FString VarName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VarName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FString VarType;
	if (!Params->TryGetStringField(TEXT("variable_type"), VarType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* FuncGraph = nullptr;
	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		if (Graph->GetName() == FunctionName)
		{
			FuncGraph = Graph;
			break;
		}
	}

	if (!FuncGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function graph not found: %s"), *FunctionName));
	}

	// Build the pin type
	FEdGraphPinType PinType;
	if (VarType == TEXT("Boolean"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
	}
	else if (VarType == TEXT("Integer") || VarType == TEXT("Int"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
	}
	else if (VarType == TEXT("Float") || VarType == TEXT("Double"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
	}
	else if (VarType == TEXT("String"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_String;
	}
	else if (VarType == TEXT("Name"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
	}
	else if (VarType == TEXT("Text"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
	}
	else if (VarType == TEXT("Vector"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
	}
	else if (VarType == TEXT("Rotator"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
	}
	else if (VarType == TEXT("Transform"))
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
		PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
	}
	else
	{
		PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
	}

	// Use FBlueprintEditorUtils::AddLocalVariable
	bool bResult = FBlueprintEditorUtils::AddLocalVariable(Blueprint, FuncGraph, FName(*VarName), PinType);
	if (!bResult)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to add local variable '%s' to function '%s'"), *VarName, *FunctionName));
	}

	// Set default value if provided
	FString DefaultValue;
	if (Params->TryGetStringField(TEXT("default_value"), DefaultValue))
	{
		// Find the variable description and set default
		for (FBPVariableDescription& VarDesc : Blueprint->NewVariables)
		{
			if (VarDesc.VarName == FName(*VarName))
			{
				VarDesc.DefaultValue = DefaultValue;
				break;
			}
		}
	}

	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Local variable '%s' (%s) added to function '%s'"), *VarName, *VarType, *FunctionName));
	ResultObj->SetStringField(TEXT("variable_name"), VarName);
	return ResultObj;
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPBlueprintGraphCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("create_function_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_function_graph"), P); });
	Registry.RegisterCommand(TEXT("delete_function_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_function_graph"), P); });
	Registry.RegisterCommand(TEXT("create_macro_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_macro_graph"), P); });
	Registry.RegisterCommand(TEXT("delete_macro_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_macro_graph"), P); });
	Registry.RegisterCommand(TEXT("auto_layout_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("auto_layout_graph"), P); });
	Registry.RegisterCommand(TEXT("set_function_access_specifier"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_function_access_specifier"), P); });
	Registry.RegisterCommand(TEXT("set_function_flags"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_function_flags"), P); });
	Registry.RegisterCommand(TEXT("add_local_variable"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_local_variable"), P); });
}
