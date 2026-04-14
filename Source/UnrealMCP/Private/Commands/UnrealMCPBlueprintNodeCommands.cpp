#include "Commands/UnrealMCPBlueprintNodeCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "K2Node_Event.h"
#include "K2Node_CallFunction.h"
#include "K2Node_VariableGet.h"
#include "K2Node_InputAction.h"
#include "K2Node_Self.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "GameFramework/InputSettings.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "EdGraphSchema_K2.h"
#include "EdGraphNode_Comment.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchString.h"
#include "K2Node_SwitchEnum.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_VariableSet.h"
#include "K2Node_MakeStruct.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MakeMap.h"
#include "K2Node_MakeSet.h"
#include "K2Node_Knot.h"
#include "K2Node_Timeline.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_AssignDelegate.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_GenericCreateObject.h"
#include "K2Node_FormatText.h"
#include "K2Node_Select.h"
#include "K2Node_Message.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_AddDelegate.h"
#include "EdGraphUtilities.h"
#include "Engine/TimelineTemplate.h"
#include "Curves/CurveFloat.h"
#include "Net/UnrealNetwork.h"

// Declare the log category
DEFINE_LOG_CATEGORY_STATIC(LogUnrealMCP, Log, All);

FUnrealMCPBlueprintNodeCommands::FUnrealMCPBlueprintNodeCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    if (CommandType == TEXT("connect_blueprint_nodes"))
    {
        return HandleConnectBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("add_blueprint_get_self_component_reference"))
    {
        return HandleAddBlueprintGetSelfComponentReference(Params);
    }
    else if (CommandType == TEXT("add_blueprint_event_node"))
    {
        return HandleAddBlueprintEvent(Params);
    }
    else if (CommandType == TEXT("add_blueprint_function_node"))
    {
        return HandleAddBlueprintFunctionCall(Params);
    }
    else if (CommandType == TEXT("add_blueprint_variable"))
    {
        return HandleAddBlueprintVariable(Params);
    }
    else if (CommandType == TEXT("add_blueprint_input_action_node"))
    {
        return HandleAddBlueprintInputActionNode(Params);
    }
    else if (CommandType == TEXT("add_blueprint_self_reference"))
    {
        return HandleAddBlueprintSelfReference(Params);
    }
    else if (CommandType == TEXT("find_blueprint_nodes"))
    {
        return HandleFindBlueprintNodes(Params);
    }
    else if (CommandType == TEXT("delete_node"))
    {
        return HandleDeleteNode(Params);
    }
    else if (CommandType == TEXT("set_node_position"))
    {
        return HandleSetNodePosition(Params);
    }
    else if (CommandType == TEXT("add_comment_node"))
    {
        return HandleAddCommentNode(Params);
    }
    else if (CommandType == TEXT("disconnect_pins"))
    {
        return HandleDisconnectPins(Params);
    }
    else if (CommandType == TEXT("disconnect_all_pins"))
    {
        return HandleDisconnectAllPins(Params);
    }
    else if (CommandType == TEXT("set_pin_default_value"))
    {
        return HandleSetPinDefaultValue(Params);
    }
    // Sprint 4: Flow Control
    else if (CommandType == TEXT("add_branch_node")) { return HandleAddBranchNode(Params); }
    else if (CommandType == TEXT("add_sequence_node")) { return HandleAddSequenceNode(Params); }
    else if (CommandType == TEXT("add_switch_on_int_node")) { return HandleAddSwitchOnIntNode(Params); }
    else if (CommandType == TEXT("add_switch_on_string_node")) { return HandleAddSwitchOnStringNode(Params); }
    else if (CommandType == TEXT("add_switch_on_enum_node")) { return HandleAddSwitchOnEnumNode(Params); }
    else if (CommandType == TEXT("add_for_each_loop_node")) { return HandleAddForEachLoopNode(Params); }
    else if (CommandType == TEXT("add_while_loop_node")) { return HandleAddWhileLoopNode(Params); }
    else if (CommandType == TEXT("add_gate_node")) { return HandleAddGateNode(Params); }
    else if (CommandType == TEXT("add_do_once_node")) { return HandleAddDoOnceNode(Params); }
    else if (CommandType == TEXT("add_flip_flop_node")) { return HandleAddFlipFlopNode(Params); }
    else if (CommandType == TEXT("add_delay_node")) { return HandleAddDelayNode(Params); }
    else if (CommandType == TEXT("add_variable_get_node")) { return HandleAddVariableGetNode(Params); }
    else if (CommandType == TEXT("add_variable_set_node")) { return HandleAddVariableSetNode(Params); }
    else if (CommandType == TEXT("add_custom_event_node")) { return HandleAddCustomEventNode(Params); }
    else if (CommandType == TEXT("add_cast_node")) { return HandleAddCastNode(Params); }
    // Sprint 5: Extended Variables & Structs
    else if (CommandType == TEXT("add_blueprint_variable_extended")) { return HandleAddVariableExtended(Params); }
    else if (CommandType == TEXT("set_variable_default_value")) { return HandleSetVariableDefaultValue(Params); }
    else if (CommandType == TEXT("set_variable_metadata")) { return HandleSetVariableMetadata(Params); }
    else if (CommandType == TEXT("delete_variable")) { return HandleDeleteVariable(Params); }
    else if (CommandType == TEXT("set_blueprint_variable_properties")) { return HandleSetBlueprintVariableProperties(Params); }
    else if (CommandType == TEXT("add_make_struct_node")) { return HandleAddMakeStructNode(Params); }
    else if (CommandType == TEXT("add_break_struct_node")) { return HandleAddBreakStructNode(Params); }
    else if (CommandType == TEXT("add_make_array_node")) { return HandleAddMakeArrayNode(Params); }
    else if (CommandType == TEXT("add_make_map_node")) { return HandleAddMakeMapNode(Params); }
    else if (CommandType == TEXT("add_make_set_node")) { return HandleAddMakeSetNode(Params); }
    else if (CommandType == TEXT("add_literal_node")) { return HandleAddLiteralNode(Params); }
    // Sprint 6: QoL Node Operations
    else if (CommandType == TEXT("validate_connection")) { return HandleValidateConnection(Params); }
    else if (CommandType == TEXT("auto_connect_nodes")) { return HandleAutoConnectNodes(Params); }
    else if (CommandType == TEXT("duplicate_node")) { return HandleDuplicateNode(Params); }
    else if (CommandType == TEXT("copy_nodes_to_graph")) { return HandleCopyNodesToGraph(Params); }
    else if (CommandType == TEXT("add_reroute_node")) { return HandleAddRerouteNode(Params); }
    // Sprint 7: Timelines, Delegates & Advanced Nodes
    else if (CommandType == TEXT("add_timeline_node")) { return HandleAddTimelineNode(Params); }
    else if (CommandType == TEXT("add_event_dispatcher")) { return HandleAddEventDispatcher(Params); }
    else if (CommandType == TEXT("add_call_dispatcher_node")) { return HandleAddCallDispatcherNode(Params); }
    else if (CommandType == TEXT("add_bind_dispatcher_node")) { return HandleAddBindDispatcherNode(Params); }
    else if (CommandType == TEXT("add_create_delegate_node")) { return HandleAddCreateDelegateNode(Params); }
    else if (CommandType == TEXT("add_spawn_actor_node")) { return HandleAddSpawnActorNode(Params); }
    else if (CommandType == TEXT("add_construct_object_node")) { return HandleAddConstructObjectNode(Params); }
    else if (CommandType == TEXT("add_format_text_node")) { return HandleAddFormatTextNode(Params); }
    else if (CommandType == TEXT("add_select_node")) { return HandleAddSelectNode(Params); }
    else if (CommandType == TEXT("add_interface_message_node")) { return HandleAddInterfaceMessageNode(Params); }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown blueprint node command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleConnectBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeId;
    if (!Params->TryGetStringField(TEXT("source_node_id"), SourceNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_id' parameter"));
    }

    FString TargetNodeId;
    if (!Params->TryGetStringField(TEXT("target_node_id"), TargetNodeId))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_id' parameter"));
    }

    FString SourcePinName;
    if (!Params->TryGetStringField(TEXT("source_pin"), SourcePinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin' parameter"));
    }

    FString TargetPinName;
    if (!Params->TryGetStringField(TEXT("target_pin"), TargetPinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the graph (optional graph_name, defaults to EventGraph)
    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);

    UEdGraph* TargetGraph = nullptr;
    if (GraphName.IsEmpty() || GraphName.Equals(TEXT("EventGraph"), ESearchCase::IgnoreCase))
    {
        TargetGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }
    else
    {
        for (UEdGraph* Graph : Blueprint->UbergraphPages)
        {
            if (Graph->GetName() == GraphName) { TargetGraph = Graph; break; }
        }
        if (!TargetGraph)
        {
            for (UEdGraph* Graph : Blueprint->FunctionGraphs)
            {
                if (Graph->GetName() == GraphName) { TargetGraph = Graph; break; }
            }
        }
    }
    if (!TargetGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Graph not found: %s"), *GraphName));
    }

    // Find the nodes
    UEdGraphNode* SourceNode = nullptr;
    UEdGraphNode* TargetNode = nullptr;
    for (UEdGraphNode* Node : TargetGraph->Nodes)
    {
        if (Node->NodeGuid.ToString() == SourceNodeId)
        {
            SourceNode = Node;
        }
        else if (Node->NodeGuid.ToString() == TargetNodeId)
        {
            TargetNode = Node;
        }
    }

    if (!SourceNode || !TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    // Connect the nodes
    if (FUnrealMCPCommonUtils::ConnectGraphNodes(TargetGraph, SourceNode, SourcePinName, TargetNode, TargetPinName))
    {
        // Mark the blueprint as modified
        if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("source_node_id"), SourceNodeId);
        ResultObj->SetStringField(TEXT("target_node_id"), TargetNodeId);
        return ResultObj;
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to connect nodes"));
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintGetSelfComponentReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ComponentName;
    if (!Params->TryGetStringField(TEXT("component_name"), ComponentName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'component_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }
    
    // We'll skip component verification since the GetAllNodes API may have changed in UE5.5
    
    // Create the variable get node directly
    UK2Node_VariableGet* GetComponentNode = NewObject<UK2Node_VariableGet>(EventGraph);
    if (!GetComponentNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create get component node"));
    }
    
    // Set up the variable reference properly for UE5.5
    FMemberReference& VarRef = GetComponentNode->VariableReference;
    VarRef.SetSelfMember(FName(*ComponentName));
    
    // Set node position
    GetComponentNode->NodePosX = NodePosition.X;
    GetComponentNode->NodePosY = NodePosition.Y;
    
    // Add to graph
    EventGraph->AddNode(GetComponentNode);
    GetComponentNode->CreateNewGuid();
    GetComponentNode->PostPlacedNewNode();
    GetComponentNode->AllocateDefaultPins();
    
    // Explicitly reconstruct node for UE5.5
    GetComponentNode->ReconstructNode();
    
    // Mark the blueprint as modified
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(GetComponentNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintEvent(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the event node
    UK2Node_Event* EventNode = FUnrealMCPCommonUtils::CreateEventNode(EventGraph, EventName, NodePosition);
    if (!EventNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create event node"));
    }

    // Mark the blueprint as modified
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(EventNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
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

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Check for target parameter (optional)
    FString Target;
    Params->TryGetStringField(TEXT("target"), Target);

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Resolve the function via the shared lookup helper.
    // Handles: BlueprintFunctionLibrary scans, script-path targets, short
    // names with and without the "U" prefix, and parent-class search.
    UFunction* Function = FUnrealMCPCommonUtils::FindCallableFunction(Target, FunctionName, Blueprint);
    UK2Node_CallFunction* FunctionNode = nullptr;
    if (Function)
    {
        FunctionNode = FUnrealMCPCommonUtils::CreateFunctionCallNode(EventGraph, Function, NodePosition);
    }
    
    if (!FunctionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Function not found: %s in target %s"), *FunctionName, Target.IsEmpty() ? TEXT("Blueprint") : *Target));
    }

    // Set parameters if provided
    if (Params->HasField(TEXT("params")))
    {
        const TSharedPtr<FJsonObject>* ParamsObj;
        if (Params->TryGetObjectField(TEXT("params"), ParamsObj))
        {
            // Process parameters
            for (const TPair<FString, TSharedPtr<FJsonValue>>& Param : (*ParamsObj)->Values)
            {
                const FString& ParamName = Param.Key;
                const TSharedPtr<FJsonValue>& ParamValue = Param.Value;
                
                // Find the parameter pin
                UEdGraphPin* ParamPin = FUnrealMCPCommonUtils::FindPin(FunctionNode, ParamName, EGPD_Input);
                if (ParamPin)
                {
                    UE_LOG(LogTemp, Display, TEXT("Found parameter pin '%s' of category '%s'"), 
                           *ParamName, *ParamPin->PinType.PinCategory.ToString());
                    UE_LOG(LogTemp, Display, TEXT("  Current default value: '%s'"), *ParamPin->DefaultValue);
                    if (ParamPin->PinType.PinSubCategoryObject.IsValid())
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Pin subcategory: '%s'"), 
                               *ParamPin->PinType.PinSubCategoryObject->GetName());
                    }
                    
                    // Set parameter based on type
                    if (ParamValue->Type == EJson::String)
                    {
                        FString StringVal = ParamValue->AsString();
                        UE_LOG(LogTemp, Display, TEXT("  Setting string parameter '%s' to: '%s'"), 
                               *ParamName, *StringVal);
                        
                        // Handle class reference parameters (e.g., ActorClass in GetActorOfClass)
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class)
                        {
                            // For class references, we require the exact class name with proper prefix
                            // - Actor classes must start with 'A' (e.g., ACameraActor)
                            // - Non-actor classes must start with 'U' (e.g., UObject)
                            const FString& ClassName = StringVal;
                            
                            // TODO: This likely won't work in UE5.5+, so don't rely on it.
                            UClass* Class = FindObject<UClass>(nullptr, *ClassName);

                            if (!Class)
                            {
                                Class = LoadObject<UClass>(nullptr, *ClassName);
                                UE_LOG(LogUnrealMCP, Display, TEXT("FindObject<UClass> failed. Assuming soft path  path: %s"), *ClassName);
                            }
                            
                            // If not found, try with Engine module path
                            if (!Class)
                            {
                                FString EngineClassName = FString::Printf(TEXT("/Script/Engine.%s"), *ClassName);
                                Class = LoadObject<UClass>(nullptr, *EngineClassName);
                                UE_LOG(LogUnrealMCP, Display, TEXT("Trying Engine module path: %s"), *EngineClassName);
                            }
                            
                            if (!Class)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to find class '%s'. Make sure to use the exact class name with proper prefix (A for actors, U for non-actors)"), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to find class '%s'"), *ClassName));
                            }

                            const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(EventGraph->GetSchema());
                            if (!K2Schema)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to get K2Schema"));
                                return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get K2Schema"));
                            }

                            K2Schema->TrySetDefaultObject(*ParamPin, Class);
                            if (ParamPin->DefaultObject != Class)
                            {
                                UE_LOG(LogUnrealMCP, Error, TEXT("Failed to set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                                return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to set class reference for pin '%s'"), *ParamPin->PinName.ToString()));
                            }

                            UE_LOG(LogUnrealMCP, Log, TEXT("Successfully set class reference for pin '%s' to '%s'"), *ParamPin->PinName.ToString(), *ClassName);
                            continue;
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Real)
                        {
                            // For floating-point types (UE 5.7 uses PC_Real category)
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"),
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Boolean)
                        {
                            bool BoolValue = ParamValue->AsBool();
                            ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                            UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                                   *ParamName, *ParamPin->DefaultValue);
                        }
                        else if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get())
                        {
                            // Handle array parameters - like Vector parameters
                            const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                            if (ParamValue->TryGetArray(ArrayValue))
                            {
                                // Check if this could be a vector (array of 3 numbers)
                                if (ArrayValue->Num() == 3)
                                {
                                    // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                    float X = (*ArrayValue)[0]->AsNumber();
                                    float Y = (*ArrayValue)[1]->AsNumber();
                                    float Z = (*ArrayValue)[2]->AsNumber();
                                    
                                    FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                    ParamPin->DefaultValue = VectorString;
                                    
                                    UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                           *ParamName, *VectorString);
                                    UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                           *ParamPin->DefaultValue);
                                }
                                else
                                {
                                    UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                                }
                            }
                        }
                    }
                    else if (ParamValue->Type == EJson::Number)
                    {
                        // Handle integer vs float parameters correctly
                        if (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Int)
                        {
                            // Ensure we're using an integer value (no decimal)
                            int32 IntValue = FMath::RoundToInt(ParamValue->AsNumber());
                            ParamPin->DefaultValue = FString::FromInt(IntValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set integer parameter '%s' to: %d (string: '%s')"), 
                                   *ParamName, IntValue, *ParamPin->DefaultValue);
                        }
                        else
                        {
                            // For other numeric types
                            float FloatValue = ParamValue->AsNumber();
                            ParamPin->DefaultValue = FString::SanitizeFloat(FloatValue);
                            UE_LOG(LogTemp, Display, TEXT("  Set float parameter '%s' to: %f (string: '%s')"), 
                                   *ParamName, FloatValue, *ParamPin->DefaultValue);
                        }
                    }
                    else if (ParamValue->Type == EJson::Boolean)
                    {
                        bool BoolValue = ParamValue->AsBool();
                        ParamPin->DefaultValue = BoolValue ? TEXT("true") : TEXT("false");
                        UE_LOG(LogTemp, Display, TEXT("  Set boolean parameter '%s' to: %s"), 
                               *ParamName, *ParamPin->DefaultValue);
                    }
                    else if (ParamValue->Type == EJson::Array)
                    {
                        UE_LOG(LogTemp, Display, TEXT("  Processing array parameter '%s'"), *ParamName);
                        // Handle array parameters - like Vector parameters
                        const TArray<TSharedPtr<FJsonValue>>* ArrayValue;
                        if (ParamValue->TryGetArray(ArrayValue))
                        {
                            // Check if this could be a vector (array of 3 numbers)
                            if (ArrayValue->Num() == 3 && 
                                (ParamPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct) &&
                                (ParamPin->PinType.PinSubCategoryObject == TBaseStructure<FVector>::Get()))
                            {
                                // Create a proper vector string: (X=0.0,Y=0.0,Z=1000.0)
                                float X = (*ArrayValue)[0]->AsNumber();
                                float Y = (*ArrayValue)[1]->AsNumber();
                                float Z = (*ArrayValue)[2]->AsNumber();
                                
                                FString VectorString = FString::Printf(TEXT("(X=%f,Y=%f,Z=%f)"), X, Y, Z);
                                ParamPin->DefaultValue = VectorString;
                                
                                UE_LOG(LogTemp, Display, TEXT("  Set vector parameter '%s' to: %s"), 
                                       *ParamName, *VectorString);
                                UE_LOG(LogTemp, Display, TEXT("  Final pin value: '%s'"), 
                                       *ParamPin->DefaultValue);
                            }
                            else
                            {
                                UE_LOG(LogTemp, Warning, TEXT("Array parameter type not fully supported yet"));
                            }
                        }
                    }
                    // Add handling for other types as needed
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Parameter pin '%s' not found"), *ParamName);
                }
            }
        }
    }

    // Mark the blueprint as modified
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(FunctionNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintVariable(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    // Get optional parameters
    bool IsExposed = false;
    if (Params->HasField(TEXT("is_exposed")))
    {
        IsExposed = Params->GetBoolField(TEXT("is_exposed"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Create variable based on type
    FEdGraphPinType PinType;
    
    // Set up pin type based on variable_type string
    if (VariableType == TEXT("Boolean"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (VariableType == TEXT("Integer") || VariableType == TEXT("Int"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (VariableType == TEXT("Float"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    }
    else if (VariableType == TEXT("String"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (VariableType == TEXT("Vector"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
    }

    // Create the variable
    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    // Set variable properties
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        // Set exposure in editor
        if (IsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit;
        }
    }

    // Mark the blueprint as modified
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintInputActionNode(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActionName;
    if (!Params->TryGetStringField(TEXT("action_name"), ActionName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'action_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the input action node
    UK2Node_InputAction* InputActionNode = FUnrealMCPCommonUtils::CreateInputActionNode(EventGraph, ActionName, NodePosition);
    if (!InputActionNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create input action node"));
    }

    // Mark the blueprint as modified
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(InputActionNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBlueprintSelfReference(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    // Get position parameters (optional)
    FVector2D NodePosition(0.0f, 0.0f);
    if (Params->HasField(TEXT("node_position")))
    {
        NodePosition = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("node_position"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create the self node
    UK2Node_Self* SelfNode = FUnrealMCPCommonUtils::CreateSelfReferenceNode(EventGraph, NodePosition);
    if (!SelfNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create self node"));
    }

    // Mark the blueprint as modified
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(SelfNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleFindBlueprintNodes(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeType;
    if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_type' parameter"));
    }

    // Find the blueprint
    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get the event graph
    UEdGraph* EventGraph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    if (!EventGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get event graph"));
    }

    // Create a JSON array for the node GUIDs
    TArray<TSharedPtr<FJsonValue>> NodeGuidArray;
    
    // Filter nodes by the exact requested type
    if (NodeType == TEXT("Event"))
    {
        FString EventName;
        if (!Params->TryGetStringField(TEXT("event_name"), EventName))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter for Event node search"));
        }
        
        // Look for nodes with exact event name (e.g., ReceiveBeginPlay)
        for (UEdGraphNode* Node : EventGraph->Nodes)
        {
            UK2Node_Event* EventNode = Cast<UK2Node_Event>(Node);
            if (EventNode && EventNode->EventReference.GetMemberName() == FName(*EventName))
            {
                UE_LOG(LogTemp, Display, TEXT("Found event node with name %s: %s"), *EventName, *EventNode->NodeGuid.ToString());
                NodeGuidArray.Add(MakeShared<FJsonValueString>(EventNode->NodeGuid.ToString()));
            }
        }
    }
    // Add other node types as needed (InputAction, etc.)
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("node_guids"), NodeGuidArray);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleDeleteNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeGuid;
    if (!Params->TryGetStringField(TEXT("node_guid"), NodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_guid' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Find the graph (optional parameter, defaults to EventGraph)
    FString GraphName;
    UEdGraph* Graph = nullptr;
    if (Params->TryGetStringField(TEXT("graph_name"), GraphName) && !GraphName.IsEmpty())
    {
        Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    }
    else
    {
        Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* Node = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, NodeGuid);
    if (!Node)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found with GUID: %s"), *NodeGuid));
    }

    FBlueprintEditorUtils::RemoveNode(Blueprint, Node);
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Node '%s' deleted successfully"), *NodeGuid));
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleSetNodePosition(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeGuid;
    if (!Params->TryGetStringField(TEXT("node_guid"), NodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_guid' parameter"));
    }

    if (!Params->HasField(TEXT("position")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'position' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    UEdGraph* Graph = nullptr;
    if (Params->TryGetStringField(TEXT("graph_name"), GraphName) && !GraphName.IsEmpty())
    {
        Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    }
    else
    {
        Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* Node = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, NodeGuid);
    if (!Node)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found with GUID: %s"), *NodeGuid));
    }

    FVector2D Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    Node->NodePosX = Position.X;
    Node->NodePosY = Position.Y;

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_guid"), NodeGuid);
    ResultObj->SetNumberField(TEXT("pos_x"), Node->NodePosX);
    ResultObj->SetNumberField(TEXT("pos_y"), Node->NodePosY);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddCommentNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString Text;
    if (!Params->TryGetStringField(TEXT("text"), Text))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'text' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    UEdGraph* Graph = nullptr;
    if (Params->TryGetStringField(TEXT("graph_name"), GraphName) && !GraphName.IsEmpty())
    {
        Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    }
    else
    {
        Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    // Get position
    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UEdGraphNode_Comment* CommentNode = NewObject<UEdGraphNode_Comment>(Graph);
    CommentNode->NodeComment = Text;
    CommentNode->NodePosX = Position.X;
    CommentNode->NodePosY = Position.Y;

    // Optional size
    if (Params->HasField(TEXT("size")))
    {
        FVector2D Size = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("size"));
        CommentNode->NodeWidth = Size.X;
        CommentNode->NodeHeight = Size.Y;
    }

    // Optional color
    const TArray<TSharedPtr<FJsonValue>>* ColorArray = nullptr;
    if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
    {
        float R = (float)(*ColorArray)[0]->AsNumber();
        float G = (float)(*ColorArray)[1]->AsNumber();
        float B = (float)(*ColorArray)[2]->AsNumber();
        float A = ColorArray->Num() >= 4 ? (float)(*ColorArray)[3]->AsNumber() : 1.0f;
        CommentNode->CommentColor = FLinearColor(R, G, B, A);
    }

    Graph->AddNode(CommentNode, true);
    CommentNode->CreateNewGuid();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(CommentNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleDisconnectPins(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeGuid;
    if (!Params->TryGetStringField(TEXT("source_node_guid"), SourceNodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_node_guid' parameter"));
    }

    FString SourcePinName;
    if (!Params->TryGetStringField(TEXT("source_pin_name"), SourcePinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'source_pin_name' parameter"));
    }

    FString TargetNodeGuid;
    if (!Params->TryGetStringField(TEXT("target_node_guid"), TargetNodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_node_guid' parameter"));
    }

    FString TargetPinName;
    if (!Params->TryGetStringField(TEXT("target_pin_name"), TargetPinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_pin_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    UEdGraph* Graph = nullptr;
    if (Params->TryGetStringField(TEXT("graph_name"), GraphName) && !GraphName.IsEmpty())
    {
        Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    }
    else
    {
        Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* SourceNode = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, SourceNodeGuid);
    if (!SourceNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source node not found: %s"), *SourceNodeGuid));
    }

    UEdGraphNode* TargetNode = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, TargetNodeGuid);
    if (!TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Target node not found: %s"), *TargetNodeGuid));
    }

    UEdGraphPin* SourcePin = FUnrealMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
    if (!SourcePin)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Source pin not found: %s"), *SourcePinName));
    }

    UEdGraphPin* TargetPin = FUnrealMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);
    if (!TargetPin)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Target pin not found: %s"), *TargetPinName));
    }

    SourcePin->BreakLinkTo(TargetPin);
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("message"), TEXT("Pins disconnected successfully"));
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleDisconnectAllPins(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeGuid;
    if (!Params->TryGetStringField(TEXT("node_guid"), NodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_guid' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    UEdGraph* Graph = nullptr;
    if (Params->TryGetStringField(TEXT("graph_name"), GraphName) && !GraphName.IsEmpty())
    {
        Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    }
    else
    {
        Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* Node = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, NodeGuid);
    if (!Node)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found with GUID: %s"), *NodeGuid));
    }

    FString PinName;
    int32 DisconnectedCount = 0;

    if (Params->TryGetStringField(TEXT("pin_name"), PinName) && !PinName.IsEmpty())
    {
        // Disconnect specific pin
        UEdGraphPin* Pin = FUnrealMCPCommonUtils::FindPin(Node, PinName);
        if (!Pin)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s"), *PinName));
        }
        DisconnectedCount = Pin->LinkedTo.Num();
        Pin->BreakAllPinLinks();
    }
    else
    {
        // Disconnect all pins on the node
        for (UEdGraphPin* Pin : Node->Pins)
        {
            DisconnectedCount += Pin->LinkedTo.Num();
            Pin->BreakAllPinLinks();
        }
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Disconnected %d link(s)"), DisconnectedCount));
    ResultObj->SetNumberField(TEXT("disconnected_count"), DisconnectedCount);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleSetPinDefaultValue(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeGuid;
    if (!Params->TryGetStringField(TEXT("node_guid"), NodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_guid' parameter"));
    }

    FString PinName;
    if (!Params->TryGetStringField(TEXT("pin_name"), PinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pin_name' parameter"));
    }

    FString Value;
    if (!Params->TryGetStringField(TEXT("value"), Value))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    UEdGraph* Graph = nullptr;
    if (Params->TryGetStringField(TEXT("graph_name"), GraphName) && !GraphName.IsEmpty())
    {
        Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    }
    else
    {
        Graph = FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
    }

    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* Node = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, NodeGuid);
    if (!Node)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found with GUID: %s"), *NodeGuid));
    }

    UEdGraphPin* Pin = FUnrealMCPCommonUtils::FindPin(Node, PinName, EGPD_Input);
    if (!Pin)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Pin not found: %s"), *PinName));
    }

    const UEdGraphSchema_K2* Schema = Cast<const UEdGraphSchema_K2>(Graph->GetSchema());
    if (!Schema)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get K2 schema"));
    }

    // For object references, try loading the object first
    if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Object ||
        Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Class ||
        Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftObject ||
        Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_SoftClass)
    {
        UObject* LoadedObject = LoadObject<UObject>(nullptr, *Value);
        if (LoadedObject)
        {
            Schema->TrySetDefaultObject(*Pin, LoadedObject);
        }
        else
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Could not load object: %s"), *Value));
        }
    }
    else
    {
        Schema->TrySetDefaultValue(*Pin, Value);
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("node_guid"), NodeGuid);
    ResultObj->SetStringField(TEXT("pin_name"), PinName);
    ResultObj->SetStringField(TEXT("value"), Value);
    ResultObj->SetStringField(TEXT("message"), TEXT("Pin default value set successfully"));
    return ResultObj;
}

//=============================================================================
// Helper: CreateMacroInstanceNode
//=============================================================================
UEdGraphNode* FUnrealMCPBlueprintNodeCommands::CreateMacroInstanceNode(UEdGraph* Graph, const FString& MacroName, const FVector2D& Position)
{
    UBlueprint* MacroBP = LoadObject<UBlueprint>(nullptr, TEXT("/Engine/EditorResources/StandardMacros.StandardMacros"));
    if (!MacroBP)
    {
        UE_LOG(LogUnrealMCP, Error, TEXT("Failed to load StandardMacros blueprint"));
        return nullptr;
    }

    UEdGraph* MacroGraph = nullptr;
    for (UEdGraph* MGraph : MacroBP->MacroGraphs)
    {
        if (MGraph && MGraph->GetName() == MacroName)
        {
            MacroGraph = MGraph;
            break;
        }
    }
    if (!MacroGraph)
    {
        UE_LOG(LogUnrealMCP, Error, TEXT("Macro '%s' not found in StandardMacros"), *MacroName);
        return nullptr;
    }

    UK2Node_MacroInstance* MacroNode = NewObject<UK2Node_MacroInstance>(Graph);
    MacroNode->SetMacroGraph(MacroGraph);
    MacroNode->NodePosX = Position.X;
    MacroNode->NodePosY = Position.Y;
    Graph->AddNode(MacroNode, true);
    MacroNode->CreateNewGuid();
    MacroNode->PostPlacedNewNode();
    MacroNode->AllocateDefaultPins();

    return MacroNode;
}

//=============================================================================
// Sprint 4: Flow Control Handlers
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBranchNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_IfThenElse* BranchNode = NewObject<UK2Node_IfThenElse>(Graph);
    BranchNode->NodePosX = Position.X;
    BranchNode->NodePosY = Position.Y;
    Graph->AddNode(BranchNode, true);
    BranchNode->CreateNewGuid();
    BranchNode->PostPlacedNewNode();
    BranchNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(BranchNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddSequenceNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    int32 NumOutputs = 2;
    if (Params->HasField(TEXT("num_outputs")))
    {
        NumOutputs = static_cast<int32>(Params->GetNumberField(TEXT("num_outputs")));
    }

    UK2Node_ExecutionSequence* SeqNode = NewObject<UK2Node_ExecutionSequence>(Graph);
    SeqNode->NodePosX = Position.X;
    SeqNode->NodePosY = Position.Y;
    Graph->AddNode(SeqNode, true);
    SeqNode->CreateNewGuid();
    SeqNode->PostPlacedNewNode();
    SeqNode->AllocateDefaultPins();

    // Sequence node starts with 2 outputs by default; add more if requested
    for (int32 i = 2; i < NumOutputs; ++i)
    {
        SeqNode->AddInputPin();
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::NodeToCompactJson(SeqNode);
    ResultObj->SetNumberField(TEXT("num_outputs"), NumOutputs);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddSwitchOnIntNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_SwitchInteger* SwitchNode = NewObject<UK2Node_SwitchInteger>(Graph);
    SwitchNode->NodePosX = Position.X;
    SwitchNode->NodePosY = Position.Y;
    Graph->AddNode(SwitchNode, true);
    SwitchNode->CreateNewGuid();
    SwitchNode->PostPlacedNewNode();
    SwitchNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(SwitchNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddSwitchOnStringNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_SwitchString* SwitchNode = NewObject<UK2Node_SwitchString>(Graph);
    SwitchNode->NodePosX = Position.X;
    SwitchNode->NodePosY = Position.Y;
    Graph->AddNode(SwitchNode, true);
    SwitchNode->CreateNewGuid();
    SwitchNode->PostPlacedNewNode();
    SwitchNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(SwitchNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddSwitchOnEnumNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EnumName;
    if (!Params->TryGetStringField(TEXT("enum_name"), EnumName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'enum_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UEnum* EnumType = FindObject<UEnum>(nullptr, *EnumName);
    if (!EnumType)
    {
        EnumType = FindObject<UEnum>(nullptr, *(TEXT("/Script/Engine.") + EnumName));
    }
    if (!EnumType)
    {
        EnumType = LoadObject<UEnum>(nullptr, *EnumName);
    }
    if (!EnumType)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Enum not found: %s"), *EnumName));
    }

    UK2Node_SwitchEnum* SwitchNode = NewObject<UK2Node_SwitchEnum>(Graph);
    SwitchNode->SetEnum(EnumType);
    SwitchNode->NodePosX = Position.X;
    SwitchNode->NodePosY = Position.Y;
    Graph->AddNode(SwitchNode, true);
    SwitchNode->CreateNewGuid();
    SwitchNode->PostPlacedNewNode();
    SwitchNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(SwitchNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddForEachLoopNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UEdGraphNode* MacroNode = CreateMacroInstanceNode(Graph, TEXT("ForEachLoop"), Position);
    if (!MacroNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create ForEachLoop macro node"));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MacroNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddWhileLoopNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UEdGraphNode* MacroNode = CreateMacroInstanceNode(Graph, TEXT("WhileLoop"), Position);
    if (!MacroNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create WhileLoop macro node"));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MacroNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddGateNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UEdGraphNode* MacroNode = CreateMacroInstanceNode(Graph, TEXT("Gate"), Position);
    if (!MacroNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create Gate macro node"));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MacroNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddDoOnceNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UEdGraphNode* MacroNode = CreateMacroInstanceNode(Graph, TEXT("DoOnce"), Position);
    if (!MacroNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create DoOnce macro node"));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MacroNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddFlipFlopNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UEdGraphNode* MacroNode = CreateMacroInstanceNode(Graph, TEXT("FlipFlop"), Position);
    if (!MacroNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create FlipFlop macro node"));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MacroNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddDelayNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    double Duration = 0.2;
    if (Params->HasField(TEXT("duration")))
    {
        Duration = Params->GetNumberField(TEXT("duration"));
    }

    // Delay is a latent function on UKismetSystemLibrary
    UClass* KismetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.KismetSystemLibrary"));
    if (!KismetClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load KismetSystemLibrary"));
    }

    UFunction* DelayFunction = KismetClass->FindFunctionByName(TEXT("Delay"));
    if (!DelayFunction)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Delay function not found on KismetSystemLibrary"));
    }

    UK2Node_CallFunction* DelayNode = NewObject<UK2Node_CallFunction>(Graph);
    DelayNode->FunctionReference.SetExternalMember(TEXT("Delay"), KismetClass);
    DelayNode->NodePosX = Position.X;
    DelayNode->NodePosY = Position.Y;
    Graph->AddNode(DelayNode, true);
    DelayNode->CreateNewGuid();
    DelayNode->PostPlacedNewNode();
    DelayNode->AllocateDefaultPins();

    // Set the duration default value
    UEdGraphPin* DurationPin = FUnrealMCPCommonUtils::FindPin(DelayNode, TEXT("Duration"), EGPD_Input);
    if (DurationPin)
    {
        DurationPin->DefaultValue = FString::SanitizeFloat(Duration);
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(DelayNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddVariableGetNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_VariableGet* GetNode = FUnrealMCPCommonUtils::CreateVariableGetNode(Graph, Blueprint, VariableName, Position);
    if (!GetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create variable get node for: %s"), *VariableName));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(GetNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddVariableSetNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_VariableSet* SetNode = FUnrealMCPCommonUtils::CreateVariableSetNode(Graph, Blueprint, VariableName, Position);
    if (!SetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create variable set node for: %s"), *VariableName));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(SetNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddCustomEventNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString EventName;
    if (!Params->TryGetStringField(TEXT("event_name"), EventName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'event_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_CustomEvent* CustomEventNode = NewObject<UK2Node_CustomEvent>(Graph);
    CustomEventNode->CustomFunctionName = FName(*EventName);
    CustomEventNode->NodePosX = Position.X;
    CustomEventNode->NodePosY = Position.Y;
    Graph->AddNode(CustomEventNode, true);
    CustomEventNode->CreateNewGuid();
    CustomEventNode->PostPlacedNewNode();
    CustomEventNode->AllocateDefaultPins();

    // Add user-defined parameters if provided
    if (Params->HasField(TEXT("parameters")))
    {
        const TArray<TSharedPtr<FJsonValue>>* ParametersArray;
        if (Params->TryGetArrayField(TEXT("parameters"), ParametersArray))
        {
            for (const TSharedPtr<FJsonValue>& ParamValue : *ParametersArray)
            {
                const TSharedPtr<FJsonObject>* ParamObj;
                if (ParamValue->TryGetObject(ParamObj))
                {
                    FString ParamName;
                    FString ParamType;
                    (*ParamObj)->TryGetStringField(TEXT("name"), ParamName);
                    (*ParamObj)->TryGetStringField(TEXT("type"), ParamType);

                    if (!ParamName.IsEmpty() && !ParamType.IsEmpty())
                    {
                        FEdGraphPinType PinType;
                        if (ParamType == TEXT("Boolean")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean; }
                        else if (ParamType == TEXT("Integer")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Int; }
                        else if (ParamType == TEXT("Float") || ParamType == TEXT("Double"))
                        {
                            PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
                            PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
                        }
                        else if (ParamType == TEXT("String")) { PinType.PinCategory = UEdGraphSchema_K2::PC_String; }
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
                        else if (ParamType == TEXT("Object"))
                        {
                            PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
                            PinType.PinSubCategoryObject = UObject::StaticClass();
                        }
                        else
                        {
                            PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
                            PinType.PinSubCategoryObject = UObject::StaticClass();
                        }

                        FName PinName = FName(*ParamName);
                        CustomEventNode->CreateUserDefinedPin(PinName, PinType, EGPD_Output);
                    }
                }
            }
        }
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(CustomEventNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddCastNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString TargetClassName;
    if (!Params->TryGetStringField(TEXT("target_class"), TargetClassName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'target_class' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    // Find the target class using various name patterns
    UClass* TargetClass = FindObject<UClass>(nullptr, *TargetClassName);
    if (!TargetClass)
    {
        TargetClass = LoadObject<UClass>(nullptr, *TargetClassName);
    }
    if (!TargetClass)
    {
        TargetClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + TargetClassName));
    }
    if (!TargetClass)
    {
        TargetClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/CoreUObject.") + TargetClassName));
    }
    if (!TargetClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Target class not found: %s"), *TargetClassName));
    }

    UK2Node_DynamicCast* CastNode = NewObject<UK2Node_DynamicCast>(Graph);
    CastNode->TargetType = TargetClass;
    CastNode->NodePosX = Position.X;
    CastNode->NodePosY = Position.Y;
    Graph->AddNode(CastNode, true);
    CastNode->CreateNewGuid();
    CastNode->PostPlacedNewNode();
    CastNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(CastNode);
}

//=============================================================================
// Sprint 5: Extended Variables & Struct/Container Operations
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddVariableExtended(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString VariableType;
    if (!Params->TryGetStringField(TEXT("variable_type"), VariableType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString SubType;
    Params->TryGetStringField(TEXT("sub_type"), SubType);

    FString ContainerType;
    Params->TryGetStringField(TEXT("container_type"), ContainerType);

    bool bIsExposed = true;
    if (Params->HasField(TEXT("is_exposed")))
    {
        bIsExposed = Params->GetBoolField(TEXT("is_exposed"));
    }

    bool bIsReplicated = false;
    if (Params->HasField(TEXT("is_replicated")))
    {
        bIsReplicated = Params->GetBoolField(TEXT("is_replicated"));
    }

    FString Category;
    Params->TryGetStringField(TEXT("category"), Category);

    // Build the pin type
    FEdGraphPinType PinType;

    if (VariableType == TEXT("Boolean")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean; }
    else if (VariableType == TEXT("Byte")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Byte; }
    else if (VariableType == TEXT("Integer")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Int; }
    else if (VariableType == TEXT("Integer64")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Int64; }
    else if (VariableType == TEXT("Float") || VariableType == TEXT("Double"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
        PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
    }
    else if (VariableType == TEXT("Name")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Name; }
    else if (VariableType == TEXT("String")) { PinType.PinCategory = UEdGraphSchema_K2::PC_String; }
    else if (VariableType == TEXT("Text")) { PinType.PinCategory = UEdGraphSchema_K2::PC_Text; }
    else if (VariableType == TEXT("Vector"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FVector>::Get();
    }
    else if (VariableType == TEXT("Rotator"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FRotator>::Get();
    }
    else if (VariableType == TEXT("Transform"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = TBaseStructure<FTransform>::Get();
    }
    else if (VariableType == TEXT("Object"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        if (!SubType.IsEmpty())
        {
            UClass* ObjClass = FindObject<UClass>(nullptr, *SubType);
            if (!ObjClass) { ObjClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + SubType)); }
            if (!ObjClass) { ObjClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/CoreUObject.") + SubType)); }
            PinType.PinSubCategoryObject = ObjClass;
        }
    }
    else if (VariableType == TEXT("Class"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
        if (!SubType.IsEmpty())
        {
            UClass* ObjClass = FindObject<UClass>(nullptr, *SubType);
            if (!ObjClass) { ObjClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + SubType)); }
            PinType.PinSubCategoryObject = ObjClass;
        }
    }
    else if (VariableType == TEXT("Struct"))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        if (!SubType.IsEmpty())
        {
            UScriptStruct* Struct = FindObject<UScriptStruct>(nullptr, *SubType);
            if (!Struct) { Struct = LoadObject<UScriptStruct>(nullptr, *SubType); }
            PinType.PinSubCategoryObject = Struct;
        }
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unsupported variable type: %s"), *VariableType));
    }

    // Container type
    if (ContainerType == TEXT("Array")) { PinType.ContainerType = EPinContainerType::Array; }
    else if (ContainerType == TEXT("Set")) { PinType.ContainerType = EPinContainerType::Set; }
    else if (ContainerType == TEXT("Map")) { PinType.ContainerType = EPinContainerType::Map; }

    FBlueprintEditorUtils::AddMemberVariable(Blueprint, FName(*VariableName), PinType);

    // Configure variable properties
    FBPVariableDescription* NewVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            NewVar = &Variable;
            break;
        }
    }

    if (NewVar)
    {
        if (bIsExposed)
        {
            NewVar->PropertyFlags |= CPF_Edit | CPF_BlueprintVisible;
            NewVar->PropertyFlags &= ~CPF_DisableEditOnTemplate;
        }
        if (bIsReplicated)
        {
            NewVar->PropertyFlags |= CPF_Net;
        }
        if (!Category.IsEmpty())
        {
            NewVar->Category = FText::FromString(Category);
        }
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("variable_type"), VariableType);
    if (!ContainerType.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("container_type"), ContainerType);
    }
    ResultObj->SetBoolField(TEXT("is_exposed"), bIsExposed);
    ResultObj->SetBoolField(TEXT("is_replicated"), bIsReplicated);
    if (!Category.IsEmpty())
    {
        ResultObj->SetStringField(TEXT("category"), Category);
    }
    if (NewVar)
    {
        ResultObj->SetStringField(TEXT("property_flags"), FString::Printf(TEXT("0x%llx"), (uint64)NewVar->PropertyFlags));
    }
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleSetVariableDefaultValue(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    FString DefaultValue;
    if (!Params->TryGetStringField(TEXT("default_value"), DefaultValue))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'default_value' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FBPVariableDescription* TargetVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            TargetVar = &Variable;
            break;
        }
    }

    if (!TargetVar)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Variable not found: %s"), *VariableName));
    }

    TargetVar->DefaultValue = DefaultValue;

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetStringField(TEXT("default_value"), DefaultValue);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleSetVariableMetadata(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FBPVariableDescription* TargetVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            TargetVar = &Variable;
            break;
        }
    }

    if (!TargetVar)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Variable not found: %s"), *VariableName));
    }

    FString CategoryStr;
    if (Params->TryGetStringField(TEXT("category"), CategoryStr))
    {
        TargetVar->Category = FText::FromString(CategoryStr);
    }

    FString Tooltip;
    if (Params->TryGetStringField(TEXT("tooltip"), Tooltip))
    {
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("tooltip"), Tooltip);
    }

    if (Params->HasField(TEXT("is_exposed")))
    {
        bool bIsExposed = Params->GetBoolField(TEXT("is_exposed"));
        if (bIsExposed)
        {
            TargetVar->PropertyFlags |= CPF_Edit | CPF_BlueprintVisible;
            TargetVar->PropertyFlags &= ~CPF_DisableEditOnTemplate;
        }
        else
        {
            TargetVar->PropertyFlags &= ~(CPF_Edit | CPF_BlueprintVisible);
        }
    }

    if (Params->HasField(TEXT("is_read_only")))
    {
        bool bIsReadOnly = Params->GetBoolField(TEXT("is_read_only"));
        if (bIsReadOnly)
        {
            TargetVar->PropertyFlags |= CPF_BlueprintReadOnly;
        }
        else
        {
            TargetVar->PropertyFlags &= ~CPF_BlueprintReadOnly;
        }
    }

    if (Params->HasField(TEXT("is_replicated")))
    {
        bool bIsReplicated = Params->GetBoolField(TEXT("is_replicated"));
        if (bIsReplicated)
        {
            TargetVar->PropertyFlags |= CPF_Net;
        }
        else
        {
            TargetVar->PropertyFlags &= ~CPF_Net;
        }
    }

    FString RepNotifyFunc;
    if (Params->TryGetStringField(TEXT("rep_notify_func"), RepNotifyFunc))
    {
        TargetVar->RepNotifyFunc = FName(*RepNotifyFunc);
        TargetVar->PropertyFlags |= CPF_Net | CPF_RepNotify;
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleDeleteVariable(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, FName(*VariableName));
    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetBoolField(TEXT("deleted"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleSetBlueprintVariableProperties(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString VariableName;
    if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FBPVariableDescription* TargetVar = nullptr;
    for (FBPVariableDescription& Variable : Blueprint->NewVariables)
    {
        if (Variable.VarName == FName(*VariableName))
        {
            TargetVar = &Variable;
            break;
        }
    }

    if (!TargetVar)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Variable not found: %s"), *VariableName));
    }

    TArray<FString> ChangedProperties;

    // ---- Visibility / Access ----

    if (Params->HasField(TEXT("is_blueprint_readable")))
    {
        bool bVal = Params->GetBoolField(TEXT("is_blueprint_readable"));
        if (bVal)
        {
            TargetVar->PropertyFlags |= CPF_BlueprintVisible;
        }
        else
        {
            TargetVar->PropertyFlags &= ~CPF_BlueprintVisible;
        }
        ChangedProperties.Add(TEXT("is_blueprint_readable"));
    }

    if (Params->HasField(TEXT("is_blueprint_writable")))
    {
        bool bVal = Params->GetBoolField(TEXT("is_blueprint_writable"));
        if (bVal)
        {
            TargetVar->PropertyFlags &= ~CPF_BlueprintReadOnly;
        }
        else
        {
            TargetVar->PropertyFlags |= CPF_BlueprintReadOnly;
        }
        ChangedProperties.Add(TEXT("is_blueprint_writable"));
    }

    if (Params->HasField(TEXT("is_exposed")))
    {
        bool bVal = Params->GetBoolField(TEXT("is_exposed"));
        if (bVal)
        {
            TargetVar->PropertyFlags |= CPF_Edit | CPF_BlueprintVisible;
            TargetVar->PropertyFlags &= ~CPF_DisableEditOnTemplate;
        }
        else
        {
            TargetVar->PropertyFlags &= ~(CPF_Edit | CPF_BlueprintVisible);
        }
        ChangedProperties.Add(TEXT("is_exposed"));
    }

    if (Params->HasField(TEXT("is_editable_in_instance")))
    {
        bool bVal = Params->GetBoolField(TEXT("is_editable_in_instance"));
        if (bVal)
        {
            TargetVar->PropertyFlags &= ~CPF_DisableEditOnInstance;
        }
        else
        {
            TargetVar->PropertyFlags |= CPF_DisableEditOnInstance;
        }
        ChangedProperties.Add(TEXT("is_editable_in_instance"));
    }

    if (Params->HasField(TEXT("is_read_only")))
    {
        bool bVal = Params->GetBoolField(TEXT("is_read_only"));
        if (bVal)
        {
            TargetVar->PropertyFlags |= CPF_BlueprintReadOnly;
        }
        else
        {
            TargetVar->PropertyFlags &= ~CPF_BlueprintReadOnly;
        }
        ChangedProperties.Add(TEXT("is_read_only"));
    }

    // ---- Expose flags ----

    if (Params->HasField(TEXT("expose_on_spawn")))
    {
        bool bVal = Params->GetBoolField(TEXT("expose_on_spawn"));
        if (bVal)
        {
            TargetVar->PropertyFlags |= CPF_ExposeOnSpawn;
            FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("ExposeOnSpawn"), TEXT("true"));
        }
        else
        {
            TargetVar->PropertyFlags &= ~CPF_ExposeOnSpawn;
            FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("ExposeOnSpawn"), TEXT("false"));
        }
        ChangedProperties.Add(TEXT("expose_on_spawn"));
    }

    if (Params->HasField(TEXT("expose_to_cinematics")))
    {
        bool bVal = Params->GetBoolField(TEXT("expose_to_cinematics"));
        if (bVal)
        {
            TargetVar->PropertyFlags |= CPF_Interp;
        }
        else
        {
            TargetVar->PropertyFlags &= ~CPF_Interp;
        }
        ChangedProperties.Add(TEXT("expose_to_cinematics"));
    }

    // ---- Replication ----

    if (Params->HasField(TEXT("replication_enabled")))
    {
        bool bVal = Params->GetBoolField(TEXT("replication_enabled"));
        if (bVal)
        {
            TargetVar->PropertyFlags |= CPF_Net;
        }
        else
        {
            TargetVar->PropertyFlags &= ~(CPF_Net | CPF_RepNotify);
            TargetVar->RepNotifyFunc = NAME_None;
        }
        ChangedProperties.Add(TEXT("replication_enabled"));
    }

    if (Params->HasField(TEXT("replication_condition")))
    {
        int32 ConditionInt = static_cast<int32>(Params->GetNumberField(TEXT("replication_condition")));
        if (ConditionInt >= 0 && ConditionInt <= 7)
        {
            TargetVar->ReplicationCondition = static_cast<ELifetimeCondition>(ConditionInt);
            TargetVar->PropertyFlags |= CPF_Net;
            ChangedProperties.Add(TEXT("replication_condition"));
        }
    }

    FString RepNotifyFunc;
    if (Params->TryGetStringField(TEXT("rep_notify_func"), RepNotifyFunc))
    {
        TargetVar->RepNotifyFunc = FName(*RepNotifyFunc);
        TargetVar->PropertyFlags |= CPF_Net | CPF_RepNotify;
        ChangedProperties.Add(TEXT("rep_notify_func"));
    }

    // ---- Category / Tooltip / Default ----

    FString CategoryStr;
    if (Params->TryGetStringField(TEXT("category"), CategoryStr))
    {
        TargetVar->Category = FText::FromString(CategoryStr);
        ChangedProperties.Add(TEXT("category"));
    }

    FString Tooltip;
    if (Params->TryGetStringField(TEXT("tooltip"), Tooltip))
    {
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("tooltip"), Tooltip);
        ChangedProperties.Add(TEXT("tooltip"));
    }

    FString DefaultValue;
    if (Params->TryGetStringField(TEXT("default_value"), DefaultValue))
    {
        TargetVar->DefaultValue = DefaultValue;
        ChangedProperties.Add(TEXT("default_value"));
    }

    // ---- UI Ranges (metadata) ----

    if (Params->HasField(TEXT("slider_range_min")))
    {
        FString Val = FString::SanitizeFloat(Params->GetNumberField(TEXT("slider_range_min")));
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("UIMin"), Val);
        ChangedProperties.Add(TEXT("slider_range_min"));
    }

    if (Params->HasField(TEXT("slider_range_max")))
    {
        FString Val = FString::SanitizeFloat(Params->GetNumberField(TEXT("slider_range_max")));
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("UIMax"), Val);
        ChangedProperties.Add(TEXT("slider_range_max"));
    }

    if (Params->HasField(TEXT("value_range_min")))
    {
        FString Val = FString::SanitizeFloat(Params->GetNumberField(TEXT("value_range_min")));
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("ClampMin"), Val);
        ChangedProperties.Add(TEXT("value_range_min"));
    }

    if (Params->HasField(TEXT("value_range_max")))
    {
        FString Val = FString::SanitizeFloat(Params->GetNumberField(TEXT("value_range_max")));
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("ClampMax"), Val);
        ChangedProperties.Add(TEXT("value_range_max"));
    }

    // ---- Units ----

    FString Units;
    if (Params->TryGetStringField(TEXT("units"), Units))
    {
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr, TEXT("Units"), Units);
        ChangedProperties.Add(TEXT("units"));
    }

    // ---- Bitmask ----

    if (Params->HasField(TEXT("bitmask")))
    {
        bool bVal = Params->GetBoolField(TEXT("bitmask"));
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr,
            TEXT("Bitmask"), bVal ? TEXT("true") : TEXT(""));
        ChangedProperties.Add(TEXT("bitmask"));
    }

    FString BitmaskEnum;
    if (Params->TryGetStringField(TEXT("bitmask_enum"), BitmaskEnum))
    {
        FBlueprintEditorUtils::SetBlueprintVariableMetaData(Blueprint, TargetVar->VarName, nullptr,
            TEXT("BitmaskEnum"), BitmaskEnum);
        ChangedProperties.Add(TEXT("bitmask_enum"));
    }

    // ---- Finalize ----

    if (!FMCPBlueprintContext::Get().IsEditing())
    {
        FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("variable_name"), VariableName);
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetNumberField(TEXT("properties_changed"), ChangedProperties.Num());

    TArray<TSharedPtr<FJsonValue>> ChangedArray;
    for (const FString& Prop : ChangedProperties)
    {
        ChangedArray.Add(MakeShared<FJsonValueString>(Prop));
    }
    ResultObj->SetArrayField(TEXT("changed"), ChangedArray);

    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddMakeStructNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString StructType;
    if (!Params->TryGetStringField(TEXT("struct_type"), StructType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'struct_type' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UScriptStruct* Struct = FindObject<UScriptStruct>(nullptr, *StructType);
    if (!Struct) { Struct = LoadObject<UScriptStruct>(nullptr, *StructType); }
    if (!Struct) { Struct = LoadObject<UScriptStruct>(nullptr, *(TEXT("/Script/CoreUObject.") + StructType)); }
    if (!Struct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Struct type not found: %s"), *StructType));
    }

    UK2Node_MakeStruct* MakeStructNode = NewObject<UK2Node_MakeStruct>(Graph);
    MakeStructNode->StructType = Struct;
    MakeStructNode->NodePosX = Position.X;
    MakeStructNode->NodePosY = Position.Y;
    Graph->AddNode(MakeStructNode, true);
    MakeStructNode->CreateNewGuid();
    MakeStructNode->PostPlacedNewNode();
    MakeStructNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MakeStructNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBreakStructNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString StructType;
    if (!Params->TryGetStringField(TEXT("struct_type"), StructType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'struct_type' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UScriptStruct* Struct = FindObject<UScriptStruct>(nullptr, *StructType);
    if (!Struct) { Struct = LoadObject<UScriptStruct>(nullptr, *StructType); }
    if (!Struct) { Struct = LoadObject<UScriptStruct>(nullptr, *(TEXT("/Script/CoreUObject.") + StructType)); }
    if (!Struct)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Struct type not found: %s"), *StructType));
    }

    UK2Node_BreakStruct* BreakStructNode = NewObject<UK2Node_BreakStruct>(Graph);
    BreakStructNode->StructType = Struct;
    BreakStructNode->NodePosX = Position.X;
    BreakStructNode->NodePosY = Position.Y;
    Graph->AddNode(BreakStructNode, true);
    BreakStructNode->CreateNewGuid();
    BreakStructNode->PostPlacedNewNode();
    BreakStructNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(BreakStructNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddMakeArrayNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_MakeArray* MakeArrayNode = NewObject<UK2Node_MakeArray>(Graph);
    MakeArrayNode->NodePosX = Position.X;
    MakeArrayNode->NodePosY = Position.Y;
    Graph->AddNode(MakeArrayNode, true);
    MakeArrayNode->CreateNewGuid();
    MakeArrayNode->PostPlacedNewNode();
    MakeArrayNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MakeArrayNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddMakeMapNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_MakeMap* MakeMapNode = NewObject<UK2Node_MakeMap>(Graph);
    MakeMapNode->NodePosX = Position.X;
    MakeMapNode->NodePosY = Position.Y;
    Graph->AddNode(MakeMapNode, true);
    MakeMapNode->CreateNewGuid();
    MakeMapNode->PostPlacedNewNode();
    MakeMapNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MakeMapNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddMakeSetNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_MakeSet* MakeSetNode = NewObject<UK2Node_MakeSet>(Graph);
    MakeSetNode->NodePosX = Position.X;
    MakeSetNode->NodePosY = Position.Y;
    Graph->AddNode(MakeSetNode, true);
    MakeSetNode->CreateNewGuid();
    MakeSetNode->PostPlacedNewNode();
    MakeSetNode->AllocateDefaultPins();

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MakeSetNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddLiteralNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString LiteralType;
    if (!Params->TryGetStringField(TEXT("type"), LiteralType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    FString Value;
    if (!Params->TryGetStringField(TEXT("value"), Value))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'value' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    FName FuncName;
    UClass* TargetClass = nullptr;

    if (LiteralType == TEXT("Float") || LiteralType == TEXT("Double"))
    {
        TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.KismetMathLibrary"));
        FuncName = TEXT("MakeLiteralFloat");
    }
    else if (LiteralType == TEXT("Integer") || LiteralType == TEXT("Int"))
    {
        TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.KismetMathLibrary"));
        FuncName = TEXT("MakeLiteralInt");
    }
    else if (LiteralType == TEXT("Boolean") || LiteralType == TEXT("Bool"))
    {
        TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.KismetMathLibrary"));
        FuncName = TEXT("MakeLiteralBool");
    }
    else if (LiteralType == TEXT("String"))
    {
        TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
        FuncName = TEXT("MakeLiteralString");
    }
    else if (LiteralType == TEXT("Name"))
    {
        TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.KismetStringLibrary"));
        FuncName = TEXT("MakeLiteralName");
    }
    else if (LiteralType == TEXT("Text"))
    {
        TargetClass = LoadObject<UClass>(nullptr, TEXT("/Script/Engine.KismetTextLibrary"));
        FuncName = TEXT("MakeLiteralText");
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unsupported literal type: %s"), *LiteralType));
    }

    if (!TargetClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to load Kismet library class for literal"));
    }

    UK2Node_CallFunction* LiteralNode = NewObject<UK2Node_CallFunction>(Graph);
    LiteralNode->FunctionReference.SetExternalMember(FuncName, TargetClass);
    LiteralNode->NodePosX = Position.X;
    LiteralNode->NodePosY = Position.Y;
    Graph->AddNode(LiteralNode, true);
    LiteralNode->CreateNewGuid();
    LiteralNode->PostPlacedNewNode();
    LiteralNode->AllocateDefaultPins();

    // Set the value on the input pin
    UEdGraphPin* ValuePin = FUnrealMCPCommonUtils::FindPin(LiteralNode, TEXT("Value"), EGPD_Input);
    if (ValuePin)
    {
        ValuePin->DefaultValue = Value;
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(LiteralNode);
}

//=============================================================================
// Sprint 6: QoL Node Operations
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleValidateConnection(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeGuid, SourcePinName, TargetNodeGuid, TargetPinName;
    if (!Params->TryGetStringField(TEXT("source_node_guid"), SourceNodeGuid) ||
        !Params->TryGetStringField(TEXT("source_pin_name"), SourcePinName) ||
        !Params->TryGetStringField(TEXT("target_node_guid"), TargetNodeGuid) ||
        !Params->TryGetStringField(TEXT("target_pin_name"), TargetPinName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required pin parameters"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* SourceNode = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, SourceNodeGuid);
    UEdGraphNode* TargetNode = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, TargetNodeGuid);
    if (!SourceNode || !TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    UEdGraphPin* SourcePin = FUnrealMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Output);
    UEdGraphPin* TargetPin = FUnrealMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Input);
    if (!SourcePin)
    {
        SourcePin = FUnrealMCPCommonUtils::FindPin(SourceNode, SourcePinName, EGPD_Input);
    }
    if (!TargetPin)
    {
        TargetPin = FUnrealMCPCommonUtils::FindPin(TargetNode, TargetPinName, EGPD_Output);
    }
    if (!SourcePin || !TargetPin)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target pin not found"));
    }

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    FPinConnectionResponse Response = Schema->CanCreateConnection(SourcePin, TargetPin);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("can_connect"), Response.CanSafeConnect());
    ResultObj->SetStringField(TEXT("message"), Response.Message.ToString());
    ResultObj->SetNumberField(TEXT("response_type"), static_cast<int32>(Response.Response));
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAutoConnectNodes(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceNodeGuid, TargetNodeGuid;
    if (!Params->TryGetStringField(TEXT("source_node_guid"), SourceNodeGuid) ||
        !Params->TryGetStringField(TEXT("target_node_guid"), TargetNodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing source_node_guid or target_node_guid"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* SourceNode = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, SourceNodeGuid);
    UEdGraphNode* TargetNode = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, TargetNodeGuid);
    if (!SourceNode || !TargetNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target node not found"));
    }

    const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
    int32 ConnectionsMade = 0;
    TArray<TSharedPtr<FJsonValue>> ConnectionsArray;

    // First pass: connect exec pins
    for (UEdGraphPin* OutPin : SourceNode->Pins)
    {
        if (OutPin->Direction != EGPD_Output || OutPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec || OutPin->LinkedTo.Num() > 0)
        {
            continue;
        }

        for (UEdGraphPin* InPin : TargetNode->Pins)
        {
            if (InPin->Direction != EGPD_Input || InPin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec || InPin->LinkedTo.Num() > 0)
            {
                continue;
            }

            FPinConnectionResponse Response = Schema->CanCreateConnection(OutPin, InPin);
            if (Response.CanSafeConnect())
            {
                OutPin->MakeLinkTo(InPin);
                ConnectionsMade++;

                TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                ConnObj->SetStringField(TEXT("source_pin"), OutPin->PinName.ToString());
                ConnObj->SetStringField(TEXT("target_pin"), InPin->PinName.ToString());
                ConnObj->SetStringField(TEXT("type"), TEXT("exec"));
                ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
                break;
            }
        }
    }

    // Second pass: connect compatible data pins
    for (UEdGraphPin* OutPin : SourceNode->Pins)
    {
        if (OutPin->Direction != EGPD_Output || OutPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec || OutPin->bHidden || OutPin->LinkedTo.Num() > 0)
        {
            continue;
        }

        for (UEdGraphPin* InPin : TargetNode->Pins)
        {
            if (InPin->Direction != EGPD_Input || InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Exec || InPin->bHidden || InPin->LinkedTo.Num() > 0)
            {
                continue;
            }

            FPinConnectionResponse Response = Schema->CanCreateConnection(OutPin, InPin);
            if (Response.CanSafeConnect())
            {
                OutPin->MakeLinkTo(InPin);
                ConnectionsMade++;

                TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
                ConnObj->SetStringField(TEXT("source_pin"), OutPin->PinName.ToString());
                ConnObj->SetStringField(TEXT("target_pin"), InPin->PinName.ToString());
                ConnObj->SetStringField(TEXT("type"), TEXT("data"));
                ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
                break;
            }
        }
    }

    if (ConnectionsMade > 0)
    {
        if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetNumberField(TEXT("connections_made"), ConnectionsMade);
    ResultObj->SetArrayField(TEXT("connections"), ConnectionsArray);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleDuplicateNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString NodeGuid;
    if (!Params->TryGetStringField(TEXT("node_guid"), NodeGuid))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_guid' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    UEdGraphNode* SourceNode = FUnrealMCPCommonUtils::FindNodeByGuid(Graph, NodeGuid);
    if (!SourceNode)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Node not found"));
    }

    if (!SourceNode->CanDuplicateNode())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("This node cannot be duplicated"));
    }

    // Get offset
    float OffsetX = 200.0f, OffsetY = 50.0f;
    if (Params->HasField(TEXT("offset")))
    {
        FVector2D Offset = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("offset"));
        OffsetX = Offset.X;
        OffsetY = Offset.Y;
    }

    // Export and import to duplicate
    TSet<UObject*> NodesToExport;
    NodesToExport.Add(SourceNode);
    FString ExportedText;
    FEdGraphUtilities::ExportNodesToText(NodesToExport, ExportedText);

    TSet<UEdGraphNode*> ImportedNodes;
    FEdGraphUtilities::ImportNodesFromText(Graph, ExportedText, ImportedNodes);

    TArray<TSharedPtr<FJsonValue>> DuplicatedArray;
    for (UEdGraphNode* NewNode : ImportedNodes)
    {
        NewNode->NodePosX += OffsetX;
        NewNode->NodePosY += OffsetY;

        TSharedPtr<FJsonObject> NodeObj = FUnrealMCPCommonUtils::NodeToCompactJson(NewNode);
        DuplicatedArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("duplicated_nodes"), DuplicatedArray);
    ResultObj->SetNumberField(TEXT("count"), DuplicatedArray.Num());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleCopyNodesToGraph(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString SourceGraphName, TargetGraphName;
    if (!Params->TryGetStringField(TEXT("source_graph_name"), SourceGraphName) ||
        !Params->TryGetStringField(TEXT("target_graph_name"), TargetGraphName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing source_graph_name or target_graph_name"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    UEdGraph* SourceGraph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, SourceGraphName);
    UEdGraph* TargetGraph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, TargetGraphName);
    if (!SourceGraph || !TargetGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Source or target graph not found"));
    }

    // Get node GUIDs to copy
    const TArray<TSharedPtr<FJsonValue>>* NodeGuids = nullptr;
    if (!Params->TryGetArrayField(TEXT("node_guids"), NodeGuids) || NodeGuids->Num() == 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or empty 'node_guids' array"));
    }

    TSet<UObject*> NodesToExport;
    for (const TSharedPtr<FJsonValue>& GuidVal : *NodeGuids)
    {
        FString GuidStr = GuidVal->AsString();
        UEdGraphNode* Node = FUnrealMCPCommonUtils::FindNodeByGuid(SourceGraph, GuidStr);
        if (Node && Node->CanDuplicateNode())
        {
            NodesToExport.Add(Node);
        }
    }

    if (NodesToExport.Num() == 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No valid nodes to copy"));
    }

    FString ExportedText;
    FEdGraphUtilities::ExportNodesToText(NodesToExport, ExportedText);

    TSet<UEdGraphNode*> ImportedNodes;
    FEdGraphUtilities::ImportNodesFromText(TargetGraph, ExportedText, ImportedNodes);

    TArray<TSharedPtr<FJsonValue>> CopiedArray;
    for (UEdGraphNode* NewNode : ImportedNodes)
    {
        TSharedPtr<FJsonObject> NodeObj = FUnrealMCPCommonUtils::NodeToCompactJson(NewNode);
        CopiedArray.Add(MakeShared<FJsonValueObject>(NodeObj));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    ResultObj->SetArrayField(TEXT("copied_nodes"), CopiedArray);
    ResultObj->SetNumberField(TEXT("count"), CopiedArray.Num());
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddRerouteNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_Knot* KnotNode = NewObject<UK2Node_Knot>(Graph);
    KnotNode->CreateNewGuid();
    KnotNode->NodePosX = Position.X;
    KnotNode->NodePosY = Position.Y;
    KnotNode->AllocateDefaultPins();
    KnotNode->PostPlacedNewNode();
    KnotNode->SetFlags(RF_Transactional);
    Graph->AddNode(KnotNode, false, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(KnotNode);
}

//=============================================================================
// Sprint 7: Timelines, Delegates & Advanced Nodes
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddTimelineNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString TimelineName;
    if (!Params->TryGetStringField(TEXT("timeline_name"), TimelineName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'timeline_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_Timeline* TimelineNode = NewObject<UK2Node_Timeline>(Graph);
    TimelineNode->TimelineName = FName(*TimelineName);
    TimelineNode->CreateNewGuid();
    TimelineNode->NodePosX = Position.X;
    TimelineNode->NodePosY = Position.Y;
    TimelineNode->AllocateDefaultPins();
    TimelineNode->PostPlacedNewNode();
    TimelineNode->SetFlags(RF_Transactional);
    Graph->AddNode(TimelineNode, true, false);

    // Set optional properties
    bool bAutoPlay = false;
    if (Params->TryGetBoolField(TEXT("auto_play"), bAutoPlay) && bAutoPlay)
    {
        TimelineNode->bAutoPlay = true;
    }
    bool bLoop = false;
    if (Params->TryGetBoolField(TEXT("loop"), bLoop) && bLoop)
    {
        TimelineNode->bLoop = true;
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::NodeToCompactJson(TimelineNode);
    ResultObj->SetStringField(TEXT("timeline_name"), TimelineName);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddEventDispatcher(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString DispatcherName;
    if (!Params->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FName DispatcherFName(*DispatcherName);

    // Create the multicast delegate variable
    FEdGraphPinType DelegatePinType;
    DelegatePinType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;

    FBlueprintEditorUtils::AddMemberVariable(Blueprint, DispatcherFName, DelegatePinType);

    // Create the delegate signature graph (required for compilation)
    const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
    UEdGraph* SigGraph = FBlueprintEditorUtils::CreateNewGraph(
        Blueprint, DispatcherFName, UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());

    if (!SigGraph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create delegate signature graph"));
    }

    SigGraph->bEditable = false;
    K2Schema->CreateDefaultNodesForGraph(*SigGraph);
    K2Schema->CreateFunctionGraphTerminators(*SigGraph, static_cast<UClass*>(nullptr));
    K2Schema->AddExtraFunctionFlags(SigGraph, FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public);
    K2Schema->MarkFunctionEntryAsEditable(SigGraph, true);

    Blueprint->DelegateSignatureGraphs.Add(SigGraph);

    // Add parameters to the delegate signature if provided
    int32 ParamCount = 0;
    const TArray<TSharedPtr<FJsonValue>>* ParametersArray = nullptr;
    if (Params->TryGetArrayField(TEXT("parameters"), ParametersArray))
    {
        // Find the function entry node in the signature graph
        UK2Node_FunctionEntry* EntryNode = nullptr;
        for (UEdGraphNode* Node : SigGraph->Nodes)
        {
            EntryNode = Cast<UK2Node_FunctionEntry>(Node);
            if (EntryNode)
            {
                break;
            }
        }

        if (EntryNode)
        {
            for (const TSharedPtr<FJsonValue>& ParamValue : *ParametersArray)
            {
                const TSharedPtr<FJsonObject>* ParamObj = nullptr;
                if (ParamValue->TryGetObject(ParamObj))
                {
                    FString ParamName;
                    FString ParamType;
                    (*ParamObj)->TryGetStringField(TEXT("name"), ParamName);
                    (*ParamObj)->TryGetStringField(TEXT("type"), ParamType);

                    if (!ParamName.IsEmpty() && !ParamType.IsEmpty())
                    {
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
                        else if (ParamType == TEXT("Object"))
                        {
                            PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
                            PinType.PinSubCategoryObject = UObject::StaticClass();
                        }
                        else
                        {
                            PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
                            PinType.PinSubCategoryObject = UObject::StaticClass();
                        }

                        EntryNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
                        ParamCount++;
                    }
                }
            }
        }
    }

    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetStringField(TEXT("dispatcher_name"), DispatcherName);
    ResultObj->SetNumberField(TEXT("parameter_count"), ParamCount);
    ResultObj->SetStringField(TEXT("message"), FString::Printf(TEXT("Event dispatcher '%s' added with signature graph and %d parameters"), *DispatcherName, ParamCount));
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddCallDispatcherNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString DispatcherName;
    if (!Params->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    // Find the delegate property on the generated class
    FProperty* DelegateProp = nullptr;
    if (Blueprint->GeneratedClass)
    {
        DelegateProp = FindFProperty<FProperty>(Blueprint->GeneratedClass, FName(*DispatcherName));
    }
    if (!DelegateProp)
    {
        // Try the skeleton class
        if (Blueprint->SkeletonGeneratedClass)
        {
            DelegateProp = FindFProperty<FProperty>(Blueprint->SkeletonGeneratedClass, FName(*DispatcherName));
        }
    }

    UK2Node_CallDelegate* CallNode = NewObject<UK2Node_CallDelegate>(Graph);
    if (DelegateProp)
    {
        CallNode->SetFromProperty(DelegateProp, true, Blueprint->GeneratedClass);
    }
    CallNode->CreateNewGuid();
    CallNode->NodePosX = Position.X;
    CallNode->NodePosY = Position.Y;
    CallNode->AllocateDefaultPins();
    CallNode->PostPlacedNewNode();
    CallNode->SetFlags(RF_Transactional);
    Graph->AddNode(CallNode, true, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(CallNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddBindDispatcherNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString DispatcherName;
    if (!Params->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    FProperty* DelegateProp = nullptr;
    if (Blueprint->GeneratedClass)
    {
        DelegateProp = FindFProperty<FProperty>(Blueprint->GeneratedClass, FName(*DispatcherName));
    }
    if (!DelegateProp && Blueprint->SkeletonGeneratedClass)
    {
        DelegateProp = FindFProperty<FProperty>(Blueprint->SkeletonGeneratedClass, FName(*DispatcherName));
    }

    UK2Node_AddDelegate* BindNode = NewObject<UK2Node_AddDelegate>(Graph);
    if (DelegateProp)
    {
        BindNode->SetFromProperty(DelegateProp, true, Blueprint->GeneratedClass);
    }
    BindNode->CreateNewGuid();
    BindNode->NodePosX = Position.X;
    BindNode->NodePosY = Position.Y;
    BindNode->AllocateDefaultPins();
    BindNode->PostPlacedNewNode();
    BindNode->SetFlags(RF_Transactional);
    Graph->AddNode(BindNode, true, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(BindNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddCreateDelegateNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_CreateDelegate* DelegateNode = NewObject<UK2Node_CreateDelegate>(Graph);
    DelegateNode->CreateNewGuid();
    DelegateNode->NodePosX = Position.X;
    DelegateNode->NodePosY = Position.Y;
    DelegateNode->AllocateDefaultPins();
    DelegateNode->PostPlacedNewNode();
    DelegateNode->SetFlags(RF_Transactional);
    Graph->AddNode(DelegateNode, true, false);

    // Set selected function if provided
    FString FunctionName;
    if (Params->TryGetStringField(TEXT("function_name"), FunctionName))
    {
        DelegateNode->SetFunction(FName(*FunctionName));
    }

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(DelegateNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddSpawnActorNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_SpawnActorFromClass* SpawnNode = NewObject<UK2Node_SpawnActorFromClass>(Graph);
    SpawnNode->CreateNewGuid();
    SpawnNode->NodePosX = Position.X;
    SpawnNode->NodePosY = Position.Y;
    SpawnNode->AllocateDefaultPins();
    SpawnNode->PostPlacedNewNode();
    SpawnNode->SetFlags(RF_Transactional);
    Graph->AddNode(SpawnNode, true, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(SpawnNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddConstructObjectNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_GenericCreateObject* ConstructNode = NewObject<UK2Node_GenericCreateObject>(Graph);
    ConstructNode->CreateNewGuid();
    ConstructNode->NodePosX = Position.X;
    ConstructNode->NodePosY = Position.Y;
    ConstructNode->AllocateDefaultPins();
    ConstructNode->PostPlacedNewNode();
    ConstructNode->SetFlags(RF_Transactional);
    Graph->AddNode(ConstructNode, true, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(ConstructNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddFormatTextNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_FormatText* FormatNode = NewObject<UK2Node_FormatText>(Graph);
    FormatNode->CreateNewGuid();
    FormatNode->NodePosX = Position.X;
    FormatNode->NodePosY = Position.Y;
    FormatNode->AllocateDefaultPins();
    FormatNode->PostPlacedNewNode();
    FormatNode->SetFlags(RF_Transactional);
    Graph->AddNode(FormatNode, true, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(FormatNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddSelectNode(const TSharedPtr<FJsonObject>& Params)
{
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    UK2Node_Select* SelectNode = NewObject<UK2Node_Select>(Graph);
    SelectNode->CreateNewGuid();
    SelectNode->NodePosX = Position.X;
    SelectNode->NodePosY = Position.Y;
    SelectNode->AllocateDefaultPins();
    SelectNode->PostPlacedNewNode();
    SelectNode->SetFlags(RF_Transactional);
    Graph->AddNode(SelectNode, true, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(SelectNode);
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintNodeCommands::HandleAddInterfaceMessageNode(const TSharedPtr<FJsonObject>& Params)
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

    FString GraphName;
    Params->TryGetStringField(TEXT("graph_name"), GraphName);
    UEdGraph* Graph = GraphName.IsEmpty() ? FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint) : FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
    if (!Graph)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
    }

    FVector2D Position(0.0f, 0.0f);
    if (Params->HasField(TEXT("position")))
    {
        Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
    }

    // Interface message nodes use K2Node_Message
    UK2Node_Message* MessageNode = NewObject<UK2Node_Message>(Graph);
    // Find the function
    UFunction* TargetFunc = nullptr;
    FString InterfaceName;
    Params->TryGetStringField(TEXT("interface_name"), InterfaceName);

    if (!InterfaceName.IsEmpty())
    {
        UClass* InterfaceClass = FindObject<UClass>(nullptr, *InterfaceName);
        if (!InterfaceClass)
        {
            InterfaceClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + InterfaceName));
        }
        if (InterfaceClass)
        {
            TargetFunc = InterfaceClass->FindFunctionByName(FName(*FunctionName));
        }
    }

    if (TargetFunc)
    {
        MessageNode->FunctionReference.SetFromField<UFunction>(TargetFunc, false);
    }

    MessageNode->CreateNewGuid();
    MessageNode->NodePosX = Position.X;
    MessageNode->NodePosY = Position.Y;
    MessageNode->AllocateDefaultPins();
    MessageNode->PostPlacedNewNode();
    MessageNode->SetFlags(RF_Transactional);
    Graph->AddNode(MessageNode, true, false);

    if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

    return FUnrealMCPCommonUtils::NodeToCompactJson(MessageNode);
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPBlueprintNodeCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("connect_blueprint_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_blueprint_nodes"), P); });
	Registry.RegisterCommand(TEXT("add_blueprint_get_self_component_reference"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_get_self_component_reference"), P); });
	Registry.RegisterCommand(TEXT("add_blueprint_self_reference"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_self_reference"), P); });
	Registry.RegisterCommand(TEXT("find_blueprint_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("find_blueprint_nodes"), P); });
	Registry.RegisterCommand(TEXT("add_blueprint_event_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_event_node"), P); });
	Registry.RegisterCommand(TEXT("add_blueprint_input_action_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_input_action_node"), P); });
	Registry.RegisterCommand(TEXT("add_blueprint_function_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_function_node"), P); });
	Registry.RegisterCommand(TEXT("add_blueprint_get_component_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_get_component_node"), P); });
	Registry.RegisterCommand(TEXT("add_blueprint_variable"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_variable"), P); });
	Registry.RegisterCommand(TEXT("delete_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_node"), P); });
	Registry.RegisterCommand(TEXT("set_node_position"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_node_position"), P); });
	Registry.RegisterCommand(TEXT("add_comment_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_comment_node"), P); });
	Registry.RegisterCommand(TEXT("disconnect_pins"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("disconnect_pins"), P); });
	Registry.RegisterCommand(TEXT("disconnect_all_pins"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("disconnect_all_pins"), P); });
	Registry.RegisterCommand(TEXT("set_pin_default_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_pin_default_value"), P); });
	// Flow Control
	Registry.RegisterCommand(TEXT("add_branch_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_branch_node"), P); });
	Registry.RegisterCommand(TEXT("add_sequence_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_sequence_node"), P); });
	Registry.RegisterCommand(TEXT("add_switch_on_int_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_switch_on_int_node"), P); });
	Registry.RegisterCommand(TEXT("add_switch_on_string_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_switch_on_string_node"), P); });
	Registry.RegisterCommand(TEXT("add_switch_on_enum_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_switch_on_enum_node"), P); });
	Registry.RegisterCommand(TEXT("add_for_each_loop_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_for_each_loop_node"), P); });
	Registry.RegisterCommand(TEXT("add_while_loop_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_while_loop_node"), P); });
	Registry.RegisterCommand(TEXT("add_gate_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_gate_node"), P); });
	Registry.RegisterCommand(TEXT("add_do_once_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_do_once_node"), P); });
	Registry.RegisterCommand(TEXT("add_flip_flop_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_flip_flop_node"), P); });
	Registry.RegisterCommand(TEXT("add_delay_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_delay_node"), P); });
	Registry.RegisterCommand(TEXT("add_variable_get_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_variable_get_node"), P); });
	Registry.RegisterCommand(TEXT("add_variable_set_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_variable_set_node"), P); });
	Registry.RegisterCommand(TEXT("add_custom_event_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_custom_event_node"), P); });
	Registry.RegisterCommand(TEXT("add_cast_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_cast_node"), P); });
	// Extended Variables & Structs
	Registry.RegisterCommand(TEXT("add_blueprint_variable_extended"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_blueprint_variable_extended"), P); });
	Registry.RegisterCommand(TEXT("set_variable_default_value"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_variable_default_value"), P); });
	Registry.RegisterCommand(TEXT("set_variable_metadata"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_variable_metadata"), P); });
	Registry.RegisterCommand(TEXT("delete_variable"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_variable"), P); });
	Registry.RegisterCommand(TEXT("set_blueprint_variable_properties"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_blueprint_variable_properties"), P); });
	Registry.RegisterCommand(TEXT("add_make_struct_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_make_struct_node"), P); });
	Registry.RegisterCommand(TEXT("add_break_struct_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_break_struct_node"), P); });
	Registry.RegisterCommand(TEXT("add_make_array_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_make_array_node"), P); });
	Registry.RegisterCommand(TEXT("add_make_map_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_make_map_node"), P); });
	Registry.RegisterCommand(TEXT("add_make_set_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_make_set_node"), P); });
	Registry.RegisterCommand(TEXT("add_literal_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_literal_node"), P); });
	// QoL Node Operations
	Registry.RegisterCommand(TEXT("validate_connection"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("validate_connection"), P); });
	Registry.RegisterCommand(TEXT("auto_connect_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("auto_connect_nodes"), P); });
	Registry.RegisterCommand(TEXT("duplicate_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("duplicate_node"), P); });
	Registry.RegisterCommand(TEXT("copy_nodes_to_graph"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("copy_nodes_to_graph"), P); });
	Registry.RegisterCommand(TEXT("add_reroute_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_reroute_node"), P); });
	// Timelines, Delegates & Advanced Nodes
	Registry.RegisterCommand(TEXT("add_timeline_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_timeline_node"), P); });
	Registry.RegisterCommand(TEXT("add_event_dispatcher"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_event_dispatcher"), P); });
	Registry.RegisterCommand(TEXT("add_call_dispatcher_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_call_dispatcher_node"), P); });
	Registry.RegisterCommand(TEXT("add_bind_dispatcher_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bind_dispatcher_node"), P); });
	Registry.RegisterCommand(TEXT("add_create_delegate_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_create_delegate_node"), P); });
	Registry.RegisterCommand(TEXT("add_spawn_actor_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_spawn_actor_node"), P); });
	Registry.RegisterCommand(TEXT("add_construct_object_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_construct_object_node"), P); });
	Registry.RegisterCommand(TEXT("add_format_text_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_format_text_node"), P); });
	Registry.RegisterCommand(TEXT("add_select_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_select_node"), P); });
	Registry.RegisterCommand(TEXT("add_interface_message_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_interface_message_node"), P); });
}