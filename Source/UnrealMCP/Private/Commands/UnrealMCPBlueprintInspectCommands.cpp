// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPBlueprintInspectCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"

#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/SCS_Node.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Logging/TokenizedMessage.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_Event.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_Timeline.h"

//=============================================================================
// HandleCommand - Route to appropriate handler
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleCommand(
	const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("get_blueprint_info"))
	{
		return HandleGetBlueprintInfo(Params);
	}
	else if (CommandType == TEXT("get_blueprint_graphs"))
	{
		return HandleGetBlueprintGraphs(Params);
	}
	else if (CommandType == TEXT("get_graph_nodes"))
	{
		return HandleGetGraphNodes(Params);
	}
	else if (CommandType == TEXT("get_node_info"))
	{
		return HandleGetNodeInfo(Params);
	}
	else if (CommandType == TEXT("get_node_pins"))
	{
		return HandleGetNodePins(Params);
	}
	else if (CommandType == TEXT("get_blueprint_variables"))
	{
		return HandleGetBlueprintVariables(Params);
	}
	else if (CommandType == TEXT("get_blueprint_connections"))
	{
		return HandleGetBlueprintConnections(Params);
	}
	else if (CommandType == TEXT("get_compile_status"))
	{
		return HandleGetCompileStatus(Params);
	}
	else if (CommandType == TEXT("get_unconnected_pins"))
	{
		return HandleGetUnconnectedPins(Params);
	}
	// Sprint 6: Snapshot Commands
	else if (CommandType == TEXT("get_blueprint_snapshot"))
	{
		return HandleGetBlueprintSnapshot(Params);
	}
	else if (CommandType == TEXT("get_graph_snapshot"))
	{
		return HandleGetGraphSnapshot(Params);
	}
	// Blueprint analysis commands
	else if (CommandType == TEXT("read_blueprint_content"))
	{
		return HandleReadBlueprintContent(Params);
	}
	else if (CommandType == TEXT("analyze_blueprint_graph"))
	{
		return HandleAnalyzeBlueprintGraph(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Unknown blueprint inspect command: %s"), *CommandType));
}

//=============================================================================
// HandleGetBlueprintInfo
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetBlueprintInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();

	// Parent class
	if (Blueprint->ParentClass)
	{
		Data->SetStringField(TEXT("parent_class"), Blueprint->ParentClass->GetName());
		Data->SetStringField(TEXT("parent_class_path"), Blueprint->ParentClass->GetPathName());
	}

	// Compile status
	FString StatusStr;
	switch (Blueprint->Status)
	{
		case BS_UpToDate: StatusStr = TEXT("up_to_date"); break;
		case BS_Dirty: StatusStr = TEXT("dirty"); break;
		case BS_Error: StatusStr = TEXT("error"); break;
		case BS_UpToDateWithWarnings: StatusStr = TEXT("warnings"); break;
		default: StatusStr = TEXT("unknown"); break;
	}
	Data->SetStringField(TEXT("compile_status"), StatusStr);

	// Blueprint type
	Data->SetStringField(TEXT("blueprint_type"), Blueprint->GetClass()->GetName());

	// Graphs array (name + type)
	TArray<TSharedPtr<FJsonValue>> GraphsArray;

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetStringField(TEXT("type"), TEXT("EventGraph"));
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetStringField(TEXT("type"), TEXT("FunctionGraph"));
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetStringField(TEXT("type"), TEXT("MacroGraph"));
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	Data->SetArrayField(TEXT("graphs"), GraphsArray);

	// Variable count
	Data->SetNumberField(TEXT("variable_count"), Blueprint->NewVariables.Num());

	// Component count
	int32 ComponentCount = 0;
	if (Blueprint->SimpleConstructionScript)
	{
		ComponentCount = Blueprint->SimpleConstructionScript->GetAllNodes().Num();
	}
	Data->SetNumberField(TEXT("component_count"), ComponentCount);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetBlueprintGraphs
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetBlueprintGraphs(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> GraphsArray;

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetStringField(TEXT("type"), TEXT("EventGraph"));
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphObj->SetStringField(TEXT("schema"), Graph->GetSchema() ? Graph->GetSchema()->GetClass()->GetName() : TEXT("None"));
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetStringField(TEXT("type"), TEXT("FunctionGraph"));
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphObj->SetStringField(TEXT("schema"), Graph->GetSchema() ? Graph->GetSchema()->GetClass()->GetName() : TEXT("None"));
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetStringField(TEXT("type"), TEXT("MacroGraph"));
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphObj->SetStringField(TEXT("schema"), Graph->GetSchema() ? Graph->GetSchema()->GetClass()->GetName() : TEXT("None"));
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	Data->SetArrayField(TEXT("graphs"), GraphsArray);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetGraphNodes
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetGraphNodes(
	const TSharedPtr<FJsonObject>& Params)
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
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> NodesArray;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		TSharedPtr<FJsonObject> NodeJson = FUnrealMCPCommonUtils::NodeToJson(Node, false);
		if (NodeJson.IsValid())
		{
			NodesArray.Add(MakeShared<FJsonValueObject>(NodeJson));
		}
	}

	Data->SetArrayField(TEXT("nodes"), NodesArray);
	Data->SetNumberField(TEXT("node_count"), NodesArray.Num());
	Data->SetStringField(TEXT("graph_name"), Graph->GetName());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetNodeInfo
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetNodeInfo(
	const TSharedPtr<FJsonObject>& Params)
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

	FString NodeGuid;
	if (!Params->TryGetStringField(TEXT("node_guid"), NodeGuid))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_guid' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	UEdGraphNode* Node = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, NodeGuid);
	if (!Node)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Node not found with GUID: %s"), *NodeGuid));
	}

	TSharedPtr<FJsonObject> NodeJson = FUnrealMCPCommonUtils::NodeToJson(Node, true);
	if (!NodeJson.IsValid())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to serialize node to JSON"));
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse(NodeJson);
}

//=============================================================================
// HandleGetNodePins
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetNodePins(
	const TSharedPtr<FJsonObject>& Params)
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

	FString NodeGuid;
	if (!Params->TryGetStringField(TEXT("node_guid"), NodeGuid))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_guid' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	UEdGraphNode* Node = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, NodeGuid);
	if (!Node)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Node not found with GUID: %s"), *NodeGuid));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> PinsArray;

	for (UEdGraphPin* Pin : Node->Pins)
	{
		TSharedPtr<FJsonObject> PinJson = FUnrealMCPCommonUtils::PinToJson(Pin, true);
		if (PinJson.IsValid())
		{
			PinsArray.Add(MakeShared<FJsonValueObject>(PinJson));
		}
	}

	Data->SetArrayField(TEXT("pins"), PinsArray);
	Data->SetNumberField(TEXT("pin_count"), PinsArray.Num());
	Data->SetStringField(TEXT("node_guid"), NodeGuid);
	Data->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetBlueprintVariables
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetBlueprintVariables(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> VariablesArray;

	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
		VarObj->SetStringField(TEXT("name"), Var.VarName.ToString());
		VarObj->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());

		if (Var.VarType.PinSubCategoryObject.IsValid())
		{
			VarObj->SetStringField(TEXT("type_object"), Var.VarType.PinSubCategoryObject->GetName());
		}

		if (!Var.VarType.PinSubCategory.IsNone())
		{
			VarObj->SetStringField(TEXT("sub_type"), Var.VarType.PinSubCategory.ToString());
		}

		// Container type
		if (Var.VarType.ContainerType != EPinContainerType::None)
		{
			FString ContainerStr;
			switch (Var.VarType.ContainerType)
			{
				case EPinContainerType::Array: ContainerStr = TEXT("Array"); break;
				case EPinContainerType::Set: ContainerStr = TEXT("Set"); break;
				case EPinContainerType::Map: ContainerStr = TEXT("Map"); break;
				default: ContainerStr = TEXT("None"); break;
			}
			VarObj->SetStringField(TEXT("container_type"), ContainerStr);
		}

		VarObj->SetStringField(TEXT("category"), Var.Category.ToString());
		VarObj->SetStringField(TEXT("default_value"), Var.DefaultValue);

		// Property flags
		VarObj->SetBoolField(TEXT("is_reference"), Var.VarType.bIsReference);
		VarObj->SetBoolField(TEXT("is_const"), Var.VarType.bIsConst);

		// Replication
		bool bIsReplicated = !Var.RepNotifyFunc.IsNone();
		VarObj->SetBoolField(TEXT("is_replicated"), bIsReplicated);
		if (bIsReplicated)
		{
			VarObj->SetStringField(TEXT("rep_notify_func"), Var.RepNotifyFunc.ToString());
		}

		// Property flags as string for additional info
		VarObj->SetStringField(TEXT("property_flags"),
			FString::Printf(TEXT("0x%llX"), (uint64)Var.PropertyFlags));

		VariablesArray.Add(MakeShared<FJsonValueObject>(VarObj));
	}

	Data->SetArrayField(TEXT("variables"), VariablesArray);
	Data->SetNumberField(TEXT("variable_count"), VariablesArray.Num());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetBlueprintConnections
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetBlueprintConnections(
	const TSharedPtr<FJsonObject>& Params)
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
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> ConnectionsArray;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			// Only emit from output pins to avoid duplicates
			if (Pin->Direction != EGPD_Output)
			{
				continue;
			}

			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
				ConnObj->SetStringField(TEXT("source_node_guid"), Node->NodeGuid.ToString());
				ConnObj->SetStringField(TEXT("source_node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
				ConnObj->SetStringField(TEXT("source_pin"), Pin->PinName.ToString());
				ConnObj->SetStringField(TEXT("target_node_guid"), LinkedPin->GetOwningNode()->NodeGuid.ToString());
				ConnObj->SetStringField(TEXT("target_node_title"), LinkedPin->GetOwningNode()->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
				ConnObj->SetStringField(TEXT("target_pin"), LinkedPin->PinName.ToString());
				ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
			}
		}
	}

	Data->SetArrayField(TEXT("connections"), ConnectionsArray);
	Data->SetNumberField(TEXT("connection_count"), ConnectionsArray.Num());
	Data->SetStringField(TEXT("graph_name"), Graph->GetName());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetCompileStatus
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetCompileStatus(
	const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	// Compile and capture messages
	FCompilerResultsLog Results;
	Results.bSilentMode = true;
	FKismetEditorUtilities::CompileBlueprint(Blueprint, EBlueprintCompileOptions::None, &Results);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();

	// Check status after compile
	FString StatusStr;
	switch (Blueprint->Status)
	{
		case BS_UpToDate: StatusStr = TEXT("up_to_date"); break;
		case BS_Dirty: StatusStr = TEXT("dirty"); break;
		case BS_Error: StatusStr = TEXT("error"); break;
		case BS_UpToDateWithWarnings: StatusStr = TEXT("warnings"); break;
		default: StatusStr = TEXT("unknown"); break;
	}
	Data->SetStringField(TEXT("status"), StatusStr);
	Data->SetNumberField(TEXT("num_errors"), Results.NumErrors);
	Data->SetNumberField(TEXT("num_warnings"), Results.NumWarnings);

	// Collect messages
	TArray<TSharedPtr<FJsonValue>> MessagesArray;
	for (const TSharedRef<FTokenizedMessage>& Message : Results.Messages)
	{
		TSharedPtr<FJsonObject> MsgObj = MakeShared<FJsonObject>();

		FString SeverityStr;
		switch (Message->GetSeverity())
		{
			case EMessageSeverity::Error: SeverityStr = TEXT("error"); break;
			case EMessageSeverity::Warning: SeverityStr = TEXT("warning"); break;
			case EMessageSeverity::PerformanceWarning: SeverityStr = TEXT("performance_warning"); break;
			case EMessageSeverity::Info: SeverityStr = TEXT("info"); break;
			default: SeverityStr = TEXT("unknown"); break;
		}

		MsgObj->SetStringField(TEXT("severity"), SeverityStr);
		MsgObj->SetStringField(TEXT("message"), Message->ToText().ToString());
		MessagesArray.Add(MakeShared<FJsonValueObject>(MsgObj));
	}

	Data->SetArrayField(TEXT("messages"), MessagesArray);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Compile status is only available in editor builds"));
#endif
}

//=============================================================================
// HandleGetUnconnectedPins
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetUnconnectedPins(
	const TSharedPtr<FJsonObject>& Params)
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
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	// Optional filter: "exec", "data", or "all" (default)
	FString Filter = TEXT("all");
	Params->TryGetStringField(TEXT("filter"), Filter);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> UnconnectedArray;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (Pin->bHidden || Pin->LinkedTo.Num() > 0)
			{
				continue;
			}

			bool bIsExec = (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);

			if (Filter == TEXT("exec") && !bIsExec)
			{
				continue;
			}
			if (Filter == TEXT("data") && bIsExec)
			{
				continue;
			}

			TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
			PinObj->SetStringField(TEXT("node_guid"), Node->NodeGuid.ToString());
			PinObj->SetStringField(TEXT("node_title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			PinObj->SetStringField(TEXT("pin_name"), Pin->PinName.ToString());
			PinObj->SetStringField(TEXT("pin_direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
			PinObj->SetStringField(TEXT("pin_category"), Pin->PinType.PinCategory.ToString());
			PinObj->SetBoolField(TEXT("is_exec"), bIsExec);
			UnconnectedArray.Add(MakeShared<FJsonValueObject>(PinObj));
		}
	}

	Data->SetArrayField(TEXT("unconnected_pins"), UnconnectedArray);
	Data->SetNumberField(TEXT("count"), UnconnectedArray.Num());
	Data->SetStringField(TEXT("graph_name"), Graph->GetName());
	Data->SetStringField(TEXT("filter"), Filter);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetBlueprintSnapshot
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetBlueprintSnapshot(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();

	Data->SetStringField(TEXT("name"), Blueprint->GetName());

	// Parent class
	if (Blueprint->ParentClass)
	{
		Data->SetStringField(TEXT("parent_class"), Blueprint->ParentClass->GetName());
	}

	// Compile status
	FString StatusStr;
	switch (Blueprint->Status)
	{
		case BS_UpToDate: StatusStr = TEXT("up_to_date"); break;
		case BS_Dirty: StatusStr = TEXT("dirty"); break;
		case BS_Error: StatusStr = TEXT("error"); break;
		case BS_UpToDateWithWarnings: StatusStr = TEXT("warnings"); break;
		default: StatusStr = TEXT("unknown"); break;
	}
	Data->SetStringField(TEXT("compile_status"), StatusStr);

	// Variables
	TArray<TSharedPtr<FJsonValue>> VariablesArray;
	for (const FBPVariableDescription& Var : Blueprint->NewVariables)
	{
		TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
		VarObj->SetStringField(TEXT("name"), Var.VarName.ToString());
		VarObj->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());

		bool bIsExposed = (Var.PropertyFlags & CPF_ExposeOnSpawn) != 0
			|| (Var.PropertyFlags & CPF_BlueprintVisible) != 0;
		VarObj->SetBoolField(TEXT("is_exposed"), bIsExposed);
		VarObj->SetStringField(TEXT("default_value"), Var.DefaultValue);
		VarObj->SetStringField(TEXT("category"), Var.Category.ToString());

		VariablesArray.Add(MakeShared<FJsonValueObject>(VarObj));
	}
	Data->SetArrayField(TEXT("variables"), VariablesArray);

	// Components
	TArray<TSharedPtr<FJsonValue>> ComponentsArray;
	if (Blueprint->SimpleConstructionScript)
	{
		const TArray<USCS_Node*>& AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
		for (USCS_Node* SCSNode : AllNodes)
		{
			if (!SCSNode || !SCSNode->ComponentTemplate)
			{
				continue;
			}

			TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
			CompObj->SetStringField(TEXT("name"), SCSNode->GetVariableName().ToString());
			CompObj->SetStringField(TEXT("class"), SCSNode->ComponentTemplate->GetClass()->GetName());

			// Find parent name
			FString ParentName = TEXT("None");
			if (SCSNode->ParentComponentOrVariableName != NAME_None)
			{
				ParentName = SCSNode->ParentComponentOrVariableName.ToString();
			}
			CompObj->SetStringField(TEXT("parent"), ParentName);

			ComponentsArray.Add(MakeShared<FJsonValueObject>(CompObj));
		}
	}
	Data->SetArrayField(TEXT("components"), ComponentsArray);

	// Graphs
	TArray<TSharedPtr<FJsonValue>> GraphsArray;

	for (UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphObj->SetStringField(TEXT("type"), TEXT("event_graph"));
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	for (UEdGraph* Graph : Blueprint->FunctionGraphs)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphObj->SetStringField(TEXT("type"), TEXT("function"));
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	for (UEdGraph* Graph : Blueprint->MacroGraphs)
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());
		GraphObj->SetStringField(TEXT("type"), TEXT("macro"));
		GraphsArray.Add(MakeShared<FJsonValueObject>(GraphObj));
	}

	Data->SetArrayField(TEXT("graphs"), GraphsArray);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetGraphSnapshot
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleGetGraphSnapshot(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString GraphName = TEXT("EventGraph");
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("graph_name"), Graph->GetName());
	Data->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

	TArray<TSharedPtr<FJsonValue>> NodesArray;

	for (UEdGraphNode* Node : Graph->Nodes)
	{
		if (!Node)
		{
			continue;
		}

		TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
		NodeObj->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
		NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
		NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
		NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
		NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);

		// Pins with inline connection data
		TArray<TSharedPtr<FJsonValue>> PinsArray;

		for (UEdGraphPin* Pin : Node->Pins)
		{
			if (!Pin || Pin->bHidden)
			{
				continue;
			}

			TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
			PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
			PinObj->SetStringField(TEXT("direction"), Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
			PinObj->SetStringField(TEXT("type"), Pin->PinType.PinCategory.ToString());

			// Inline connection data: "NodeGUID:PinName" for each linked pin
			TArray<TSharedPtr<FJsonValue>> ConnectedToArray;
			for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
			{
				if (!LinkedPin || !LinkedPin->GetOwningNode())
				{
					continue;
				}

				FString ConnectionStr = FString::Printf(TEXT("%s:%s"),
					*LinkedPin->GetOwningNode()->NodeGuid.ToString(),
					*LinkedPin->PinName.ToString());
				ConnectedToArray.Add(MakeShared<FJsonValueString>(ConnectionStr));
			}
			PinObj->SetArrayField(TEXT("connected_to"), ConnectedToArray);

			PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
		}

		NodeObj->SetArrayField(TEXT("pins"), PinsArray);
		NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
	}

	Data->SetArrayField(TEXT("nodes"), NodesArray);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPBlueprintInspectCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("get_blueprint_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blueprint_info"), P); });
	Registry.RegisterCommand(TEXT("get_blueprint_graphs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blueprint_graphs"), P); });
	Registry.RegisterCommand(TEXT("get_graph_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_graph_nodes"), P); });
	Registry.RegisterCommand(TEXT("get_node_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_node_info"), P); });
	Registry.RegisterCommand(TEXT("get_node_pins"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_node_pins"), P); });
	Registry.RegisterCommand(TEXT("get_blueprint_variables"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blueprint_variables"), P); });
	Registry.RegisterCommand(TEXT("get_blueprint_connections"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blueprint_connections"), P); });
	Registry.RegisterCommand(TEXT("get_compile_status"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_compile_status"), P); });
	Registry.RegisterCommand(TEXT("get_unconnected_pins"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_unconnected_pins"), P); });
	Registry.RegisterCommand(TEXT("get_blueprint_snapshot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_blueprint_snapshot"), P); });
	Registry.RegisterCommand(TEXT("get_graph_snapshot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_graph_snapshot"), P); });
	Registry.RegisterCommand(TEXT("read_blueprint_content"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("read_blueprint_content"), P); });
	Registry.RegisterCommand(TEXT("analyze_blueprint_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("analyze_blueprint_graph"), P); });
}

// ============================================================================
// SEMANTIC NODE HELPER
// ============================================================================

void FUnrealMCPBlueprintInspectCommands::AddNodeSemanticData(
	UEdGraphNode* Node, TSharedPtr<FJsonObject>& NodeObj)
{
	if (!Node) return;

	// CallFunction — which function is being called
	if (UK2Node_CallFunction* FuncNode = Cast<UK2Node_CallFunction>(Node))
	{
		UFunction* Func = FuncNode->GetTargetFunction();
		if (Func)
		{
			NodeObj->SetStringField(TEXT("function_name"), Func->GetName());
			NodeObj->SetStringField(TEXT("function_owner"), Func->GetOwnerClass() ? Func->GetOwnerClass()->GetName() : TEXT("Unknown"));
			NodeObj->SetBoolField(TEXT("is_pure"), FuncNode->IsNodePure());
		}
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("CallFunction"));
	}
	// VariableGet
	else if (UK2Node_VariableGet* VarGet = Cast<UK2Node_VariableGet>(Node))
	{
		NodeObj->SetStringField(TEXT("variable_name"), VarGet->GetVarName().ToString());
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("VariableGet"));
	}
	// VariableSet
	else if (UK2Node_VariableSet* VarSet = Cast<UK2Node_VariableSet>(Node))
	{
		NodeObj->SetStringField(TEXT("variable_name"), VarSet->GetVarName().ToString());
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("VariableSet"));
	}
	// Event (BeginPlay, Tick, etc.)
	else if (UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node))
	{
		UFunction* EventFunc = EventNode->FindEventSignatureFunction();
		if (EventFunc)
		{
			NodeObj->SetStringField(TEXT("event_name"), EventFunc->GetName());
		}
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("Event"));
	}
	// CustomEvent
	else if (UK2Node_CustomEvent* CustomEvent = Cast<UK2Node_CustomEvent>(Node))
	{
		NodeObj->SetStringField(TEXT("event_name"), CustomEvent->CustomFunctionName.ToString());
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("CustomEvent"));
	}
	// DynamicCast
	else if (UK2Node_DynamicCast* CastNode = Cast<UK2Node_DynamicCast>(Node))
	{
		if (CastNode->TargetType)
		{
			NodeObj->SetStringField(TEXT("cast_target"), CastNode->TargetType->GetName());
		}
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("Cast"));
	}
	// MacroInstance
	else if (UK2Node_MacroInstance* MacroNode = Cast<UK2Node_MacroInstance>(Node))
	{
		if (MacroNode->GetMacroGraph())
		{
			NodeObj->SetStringField(TEXT("macro_name"), MacroNode->GetMacroGraph()->GetName());
		}
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("MacroInstance"));
	}
	// FunctionEntry
	else if (UK2Node_FunctionEntry* EntryNode = Cast<UK2Node_FunctionEntry>(Node))
	{
		if (UEdGraph* Graph = EntryNode->GetGraph())
		{
			NodeObj->SetStringField(TEXT("function_name"), Graph->GetName());
		}
		// Access specifier
		FString AccessStr;
		switch (EntryNode->GetFunctionFlags() & (FUNC_Public | FUNC_Protected | FUNC_Private))
		{
			case FUNC_Public: AccessStr = TEXT("Public"); break;
			case FUNC_Protected: AccessStr = TEXT("Protected"); break;
			case FUNC_Private: AccessStr = TEXT("Private"); break;
			default: AccessStr = TEXT("Public"); break;
		}
		NodeObj->SetStringField(TEXT("access"), AccessStr);
		NodeObj->SetBoolField(TEXT("is_pure"), (EntryNode->GetFunctionFlags() & FUNC_BlueprintPure) != 0);
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("FunctionEntry"));
	}
	// FunctionResult
	else if (Cast<UK2Node_FunctionResult>(Node))
	{
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("FunctionResult"));
	}
	// InputAction
	else if (UK2Node_InputAction* InputNode = Cast<UK2Node_InputAction>(Node))
	{
		NodeObj->SetStringField(TEXT("action_name"), InputNode->InputActionName.ToString());
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("InputAction"));
	}
	// Timeline
	else if (UK2Node_Timeline* TimelineNode = Cast<UK2Node_Timeline>(Node))
	{
		NodeObj->SetStringField(TEXT("timeline_name"), TimelineNode->TimelineName.ToString());
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("Timeline"));
	}
	// SpawnActor
	else if (UK2Node_SpawnActorFromClass* SpawnNode = Cast<UK2Node_SpawnActorFromClass>(Node))
	{
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("SpawnActor"));
	}
	// Self
	else if (Cast<UK2Node_Self>(Node))
	{
		NodeObj->SetStringField(TEXT("semantic_type"), TEXT("Self"));
	}
}

// ============================================================================
// HandleReadBlueprintContent
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleReadBlueprintContent(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	// Optional flags (default: all true)
	bool bIncludeEventGraph = true, bIncludeFunctions = true, bIncludeVariables = true;
	bool bIncludeComponents = true, bIncludeInterfaces = true;
	Params->TryGetBoolField(TEXT("include_event_graph"), bIncludeEventGraph);
	Params->TryGetBoolField(TEXT("include_functions"), bIncludeFunctions);
	Params->TryGetBoolField(TEXT("include_variables"), bIncludeVariables);
	Params->TryGetBoolField(TEXT("include_components"), bIncludeComponents);
	Params->TryGetBoolField(TEXT("include_interfaces"), bIncludeInterfaces);

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("name"), Blueprint->GetName());
	Data->SetStringField(TEXT("path"), Blueprint->GetPathName());
	Data->SetStringField(TEXT("blueprint_type"), Blueprint->GetClass()->GetName());

	// Parent class
	if (Blueprint->ParentClass)
	{
		Data->SetStringField(TEXT("parent_class"), Blueprint->ParentClass->GetName());
		Data->SetStringField(TEXT("parent_class_path"), Blueprint->ParentClass->GetPathName());
	}

	// Compile status
	FString StatusStr;
	switch (Blueprint->Status)
	{
		case BS_UpToDate: StatusStr = TEXT("up_to_date"); break;
		case BS_Dirty: StatusStr = TEXT("dirty"); break;
		case BS_Error: StatusStr = TEXT("error"); break;
		case BS_UpToDateWithWarnings: StatusStr = TEXT("warnings"); break;
		default: StatusStr = TEXT("unknown"); break;
	}
	Data->SetStringField(TEXT("compile_status"), StatusStr);

	// ---- INTERFACES ----
	if (bIncludeInterfaces)
	{
		TArray<TSharedPtr<FJsonValue>> InterfacesArray;
		for (const FBPInterfaceDescription& Interface : Blueprint->ImplementedInterfaces)
		{
			TSharedPtr<FJsonObject> IntObj = MakeShared<FJsonObject>();
			if (Interface.Interface)
			{
				IntObj->SetStringField(TEXT("name"), Interface.Interface->GetName());
				IntObj->SetStringField(TEXT("path"), Interface.Interface->GetPathName());
			}
			IntObj->SetNumberField(TEXT("graph_count"), Interface.Graphs.Num());
			InterfacesArray.Add(MakeShared<FJsonValueObject>(IntObj));
		}
		Data->SetArrayField(TEXT("interfaces"), InterfacesArray);
	}

	// ---- EVENT DISPATCHERS ----
	{
		TArray<TSharedPtr<FJsonValue>> DispatchersArray;
		for (UEdGraph* Graph : Blueprint->DelegateSignatureGraphs)
		{
			if (!Graph) continue;
			TSharedPtr<FJsonObject> DispObj = MakeShared<FJsonObject>();
			DispObj->SetStringField(TEXT("name"), Graph->GetName());
			DispatchersArray.Add(MakeShared<FJsonValueObject>(DispObj));
		}
		Data->SetArrayField(TEXT("event_dispatchers"), DispatchersArray);
	}

	// ---- VARIABLES ----
	if (bIncludeVariables)
	{
		TArray<TSharedPtr<FJsonValue>> VariablesArray;
		for (const FBPVariableDescription& Var : Blueprint->NewVariables)
		{
			TSharedPtr<FJsonObject> VarObj = MakeShared<FJsonObject>();
			VarObj->SetStringField(TEXT("name"), Var.VarName.ToString());
			VarObj->SetStringField(TEXT("type"), Var.VarType.PinCategory.ToString());
			if (Var.VarType.PinSubCategoryObject.IsValid())
			{
				VarObj->SetStringField(TEXT("sub_type"), Var.VarType.PinSubCategoryObject->GetName());
			}
			VarObj->SetStringField(TEXT("default_value"), Var.DefaultValue);
			VarObj->SetStringField(TEXT("category"), Var.Category.ToString());
			VarObj->SetBoolField(TEXT("is_replicated"), (Var.PropertyFlags & CPF_Net) != 0);

			bool bIsExposed = (Var.PropertyFlags & CPF_ExposeOnSpawn) != 0
				|| (Var.PropertyFlags & CPF_BlueprintVisible) != 0;
			VarObj->SetBoolField(TEXT("is_exposed"), bIsExposed);

			VariablesArray.Add(MakeShared<FJsonValueObject>(VarObj));
		}
		Data->SetArrayField(TEXT("variables"), VariablesArray);
	}

	// ---- COMPONENTS ----
	if (bIncludeComponents)
	{
		TArray<TSharedPtr<FJsonValue>> ComponentsArray;
		if (Blueprint->SimpleConstructionScript)
		{
			const TArray<USCS_Node*>& AllNodes = Blueprint->SimpleConstructionScript->GetAllNodes();
			for (USCS_Node* SCSNode : AllNodes)
			{
				if (!SCSNode || !SCSNode->ComponentTemplate) continue;

				TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
				CompObj->SetStringField(TEXT("name"), SCSNode->GetVariableName().ToString());
				CompObj->SetStringField(TEXT("class"), SCSNode->ComponentTemplate->GetClass()->GetName());

				FString ParentName = TEXT("None");
				if (SCSNode->ParentComponentOrVariableName != NAME_None)
				{
					ParentName = SCSNode->ParentComponentOrVariableName.ToString();
				}
				CompObj->SetStringField(TEXT("parent"), ParentName);

				ComponentsArray.Add(MakeShared<FJsonValueObject>(CompObj));
			}
		}
		Data->SetArrayField(TEXT("components"), ComponentsArray);
	}

	// ---- GRAPHS with semantic node data ----
	auto BuildGraphJson = [](UEdGraph* Graph, const FString& GraphType) -> TSharedPtr<FJsonObject>
	{
		TSharedPtr<FJsonObject> GraphObj = MakeShared<FJsonObject>();
		GraphObj->SetStringField(TEXT("name"), Graph->GetName());
		GraphObj->SetStringField(TEXT("type"), GraphType);
		GraphObj->SetNumberField(TEXT("node_count"), Graph->Nodes.Num());

		TArray<TSharedPtr<FJsonValue>> NodesArray;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node) continue;

			TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
			NodeObj->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
			NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
			NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
			NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
			NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);

			// Semantic metadata
			FUnrealMCPBlueprintInspectCommands::AddNodeSemanticData(Node, NodeObj);

			NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
		}
		GraphObj->SetArrayField(TEXT("nodes"), NodesArray);
		return GraphObj;
	};

	TArray<TSharedPtr<FJsonValue>> GraphsArray;

	if (bIncludeEventGraph)
	{
		for (UEdGraph* Graph : Blueprint->UbergraphPages)
		{
			GraphsArray.Add(MakeShared<FJsonValueObject>(BuildGraphJson(Graph, TEXT("EventGraph"))));
		}
	}

	if (bIncludeFunctions)
	{
		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			GraphsArray.Add(MakeShared<FJsonValueObject>(BuildGraphJson(Graph, TEXT("FunctionGraph"))));
		}

		for (UEdGraph* Graph : Blueprint->MacroGraphs)
		{
			GraphsArray.Add(MakeShared<FJsonValueObject>(BuildGraphJson(Graph, TEXT("MacroGraph"))));
		}
	}

	Data->SetArrayField(TEXT("graphs"), GraphsArray);

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

// ============================================================================
// HandleAnalyzeBlueprintGraph
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintInspectCommands::HandleAnalyzeBlueprintGraph(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString GraphName = TEXT("EventGraph");
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	bool bIncludeNodeDetails = true, bIncludePinConnections = true, bTraceExecFlow = true;
	Params->TryGetBoolField(TEXT("include_node_details"), bIncludeNodeDetails);
	Params->TryGetBoolField(TEXT("include_pin_connections"), bIncludePinConnections);
	Params->TryGetBoolField(TEXT("trace_execution_flow"), bTraceExecFlow);

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* TargetGraph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!TargetGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("graph_name"), TargetGraph->GetName());
	Data->SetNumberField(TEXT("node_count"), TargetGraph->Nodes.Num());

	// ---- NODES with semantic data + pins ----
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UEdGraphNode* Node : TargetGraph->Nodes)
	{
		if (!Node) continue;

		TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
		NodeObj->SetStringField(TEXT("guid"), Node->NodeGuid.ToString());
		NodeObj->SetStringField(TEXT("class"), Node->GetClass()->GetName());
		NodeObj->SetStringField(TEXT("title"), Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
		NodeObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
		NodeObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);

		if (!Node->NodeComment.IsEmpty())
		{
			NodeObj->SetStringField(TEXT("comment"), Node->NodeComment);
		}

		// Semantic metadata
		AddNodeSemanticData(Node, NodeObj);

		// Pins with connection info and type classification
		if (bIncludeNodeDetails)
		{
			TArray<TSharedPtr<FJsonValue>> PinsArray;
			for (UEdGraphPin* Pin : Node->Pins)
			{
				if (!Pin || Pin->bHidden) continue;

				TSharedPtr<FJsonObject> PinObj = MakeShared<FJsonObject>();
				PinObj->SetStringField(TEXT("name"), Pin->PinName.ToString());
				PinObj->SetStringField(TEXT("direction"),
					Pin->Direction == EGPD_Input ? TEXT("input") : TEXT("output"));
				PinObj->SetStringField(TEXT("category"), Pin->PinType.PinCategory.ToString());
				PinObj->SetBoolField(TEXT("is_exec"),
					Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec);

				if (!Pin->DefaultValue.IsEmpty())
				{
					PinObj->SetStringField(TEXT("default_value"), Pin->DefaultValue);
				}

				if (bIncludePinConnections && Pin->LinkedTo.Num() > 0)
				{
					TArray<TSharedPtr<FJsonValue>> ConnArray;
					for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
					{
						if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;
						TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
						ConnObj->SetStringField(TEXT("node_guid"),
							LinkedPin->GetOwningNode()->NodeGuid.ToString());
						ConnObj->SetStringField(TEXT("pin_name"), LinkedPin->PinName.ToString());
						ConnArray.Add(MakeShared<FJsonValueObject>(ConnObj));
					}
					PinObj->SetArrayField(TEXT("connections"), ConnArray);
				}

				PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
			}
			NodeObj->SetArrayField(TEXT("pins"), PinsArray);
		}

		NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
	}
	Data->SetArrayField(TEXT("nodes"), NodesArray);

	// ---- EXECUTION FLOW TRACING ----
	if (bTraceExecFlow)
	{
		// Find all entry points (Event nodes, CustomEvent nodes, FunctionEntry nodes)
		TArray<TSharedPtr<FJsonValue>> ExecPathsArray;

		for (UEdGraphNode* Node : TargetGraph->Nodes)
		{
			if (!Node) continue;

			bool bIsEntry = Cast<UK2Node_Event>(Node) != nullptr
				|| Cast<UK2Node_CustomEvent>(Node) != nullptr
				|| Cast<UK2Node_FunctionEntry>(Node) != nullptr;

			if (!bIsEntry) continue;

			TSharedPtr<FJsonObject> PathObj = MakeShared<FJsonObject>();
			PathObj->SetStringField(TEXT("entry_node_guid"), Node->NodeGuid.ToString());
			PathObj->SetStringField(TEXT("entry_title"),
				Node->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

			// Walk exec pins depth-first
			TArray<TSharedPtr<FJsonValue>> StepsArray;
			TSet<FGuid> Visited;
			TArray<UEdGraphNode*> Stack;
			Stack.Push(Node);

			while (Stack.Num() > 0)
			{
				UEdGraphNode* Current = Stack.Pop();
				if (!Current || Visited.Contains(Current->NodeGuid)) continue;
				Visited.Add(Current->NodeGuid);

				TSharedPtr<FJsonObject> StepObj = MakeShared<FJsonObject>();
				StepObj->SetStringField(TEXT("guid"), Current->NodeGuid.ToString());
				StepObj->SetStringField(TEXT("title"),
					Current->GetNodeTitle(ENodeTitleType::FullTitle).ToString());
				StepObj->SetStringField(TEXT("class"), Current->GetClass()->GetName());

				AddNodeSemanticData(Current, StepObj);

				// Find exec output connections
				TArray<TSharedPtr<FJsonValue>> NextArray;
				for (UEdGraphPin* Pin : Current->Pins)
				{
					if (!Pin || Pin->Direction != EGPD_Output
						|| Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
					{
						continue;
					}
					for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
					{
						if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;
						UEdGraphNode* NextNode = LinkedPin->GetOwningNode();

						TSharedPtr<FJsonObject> NextObj = MakeShared<FJsonObject>();
						NextObj->SetStringField(TEXT("pin_name"), Pin->PinName.ToString());
						NextObj->SetStringField(TEXT("target_guid"), NextNode->NodeGuid.ToString());
						NextArray.Add(MakeShared<FJsonValueObject>(NextObj));

						if (!Visited.Contains(NextNode->NodeGuid))
						{
							Stack.Push(NextNode);
						}
					}
				}
				StepObj->SetArrayField(TEXT("exec_outputs"), NextArray);

				StepsArray.Add(MakeShared<FJsonValueObject>(StepObj));
			}

			PathObj->SetArrayField(TEXT("steps"), StepsArray);
			PathObj->SetNumberField(TEXT("step_count"), StepsArray.Num());
			ExecPathsArray.Add(MakeShared<FJsonValueObject>(PathObj));
		}

		Data->SetArrayField(TEXT("execution_paths"), ExecPathsArray);
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}
