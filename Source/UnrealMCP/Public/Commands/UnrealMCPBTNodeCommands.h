// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;
class UBehaviorTree;
class UBehaviorTreeGraph;
class UEdGraphNode;

/**
 * Behavior Tree node command handlers for Unreal MCP.
 * Provides composite, task, decorator, and service node creation
 * within an existing Behavior Tree graph.
 *
 * EDITOR ONLY - These commands require the BehaviorTreeEditor module.
 *
 * Command Categories:
 * - Composites: selector, sequence, simple parallel
 * - Tasks: wait, move to, play sound, play animation, run behavior,
 *          run EQS, finish with result, make noise, rotate to face, set key value
 * - Decorators: blackboard, cooldown, loop, time limit, force success,
 *               compare BB, cone check, does path exist, is at location, tag cooldown
 * - Services: default focus, run EQS
 */
class UNREALMCP_API FUnrealMCPBTNodeCommands
{
public:
	FUnrealMCPBTNodeCommands() = default;

	/**
	 * Handle BT node-related commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all BT node commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//=========================================================================
	// Generic Node Creation (works with ANY BTNode subclass)
	//=========================================================================

	/**
	 * Add any BT node by class name. Resolves the class via UE reflection,
	 * auto-detects whether it is a task, decorator, service, or composite,
	 * and creates/attaches it accordingly.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "class_name" (string): UClass name (e.g. "BTTask_MoveTo", "BTDecorator_Blackboard")
	 *   - "parent_node_index" (int, optional): Parent composite index for tasks/composites
	 *   - "target_node_index" (int, optional): Node to attach decorator/service to
	 *   - "properties" (object, optional): Key-value pairs to set via reflection.
	 *       Values can be numbers, bools, or strings. String values for
	 *       FBlackboardKeySelector properties set SelectedKeyName.
	 */
	TSharedPtr<FJsonObject> HandleAddBTNodeByClass(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Composite Node Commands
	//=========================================================================

	/**
	 * Add a Selector composite node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root, default: -1)
	 */
	TSharedPtr<FJsonObject> HandleAddBTSelector(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a Sequence composite node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root, default: -1)
	 */
	TSharedPtr<FJsonObject> HandleAddBTSequence(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a Simple Parallel composite node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root, default: -1)
	 *   - "finish_mode" (string, optional): "Immediate" or "Delayed" (default: "Immediate")
	 */
	TSharedPtr<FJsonObject> HandleAddBTSimpleParallel(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Task Node Commands
	//=========================================================================

	/**
	 * Add a Wait task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "wait_time" (float, optional): Wait duration in seconds (default: 5.0)
	 *   - "random_deviation" (float, optional): Random time added to wait (default: 0.0)
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskWait(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a MoveTo task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "bb_key" (string, optional): Blackboard key name for target
	 *   - "acceptable_radius" (float, optional): Acceptance radius (default: 5.0)
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskMoveTo(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a PlaySound task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "sound_path" (string, optional): Asset path to the sound cue
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskPlaySound(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a PlayAnimation task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "anim_path" (string, optional): Asset path to the animation
	 *   - "looping" (bool, optional): Whether animation loops (default: false)
	 *   - "non_blocking" (bool, optional): Fire and forget mode (default: false)
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskPlayAnimation(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a RunBehavior task node (sub-tree).
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "sub_tree_name" (string): Name or path of the sub-tree to run
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskRunBehavior(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a RunEQS task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "query_template" (string, optional): EQS query template asset path
	 *   - "bb_key" (string, optional): Blackboard key for storing result
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskRunEQS(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a FinishWithResult task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "result" (string, optional): "Succeeded", "Failed", or "Aborted" (default: "Succeeded")
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskFinishWithResult(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a MakeNoise task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "loudness" (float, optional): Loudness 0.0-1.0 (default: 0.5)
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskMakeNoise(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a RotateToFaceBBEntry task node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "bb_key" (string, optional): Blackboard key to face toward
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskRotateToFace(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a task node to set a blackboard key value.
	 * Uses BTTask_Wait as a placeholder since UBTTask_SetKeyValue may not exist.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "parent_node_index" (int, optional): Parent node index (-1 for root)
	 *   - "bb_key" (string, optional): Blackboard key name
	 *   - "value" (string, optional): Value to set (as string representation)
	 */
	TSharedPtr<FJsonObject> HandleAddBTTaskSetKeyValue(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Decorator Node Commands
	//=========================================================================

	/**
	 * Add a Blackboard decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "bb_key" (string, optional): Blackboard key to observe
	 *   - "notify_observer" (string, optional): "ValueChange" or "ResultChange" (default: "ResultChange")
	 *   - "flow_abort_mode" (string, optional): "None", "Self", "LowerPriority", "Both" (default: "None")
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorBlackboard(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a Cooldown decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "cooldown_time" (float, optional): Cooldown duration in seconds (default: 5.0)
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorCooldown(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a Loop decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "num_loops" (int, optional): Number of loops 1-255 (default: 3)
	 *   - "infinite_loop" (bool, optional): Enable infinite looping (default: false)
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorLoop(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a TimeLimit decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "time_limit" (float, optional): Max execution time in seconds (default: 5.0)
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorTimeLimit(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a ForceSuccess decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorForceSuccess(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a CompareBBEntries decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "key_a" (string, optional): First blackboard key name
	 *   - "key_b" (string, optional): Second blackboard key name
	 *   - "operator" (string, optional): "Equal" or "NotEqual" (default: "Equal")
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorCompareBB(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a ConeCheck decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "cone_half_angle" (float, optional): Half angle of cone in degrees (default: 45.0)
	 *   - "cone_origin_key" (string, optional): Blackboard key for cone origin
	 *   - "observed_key" (string, optional): Blackboard key for observed target
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorConeCheck(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a DoesPathExist decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "bb_key_a" (string, optional): First blackboard key (path start)
	 *   - "bb_key_b" (string, optional): Second blackboard key (path end)
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorDoesPathExist(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add an IsAtLocation decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "bb_key" (string, optional): Blackboard key for location
	 *   - "acceptable_radius" (float, optional): Distance threshold (default: 50.0)
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorIsAtLocation(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a TagCooldown decorator node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach decorator to
	 *   - "cooldown_tag" (string): Gameplay tag for cooldown
	 *   - "cooldown_duration" (float, optional): Cooldown duration in seconds (default: 5.0)
	 *   - "add_to_existing" (bool, optional): Add to existing duration (default: true)
	 */
	TSharedPtr<FJsonObject> HandleAddBTDecoratorTagCooldown(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Service Node Commands
	//=========================================================================

	/**
	 * Add a DefaultFocus service node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach service to
	 *   - "bb_key" (string, optional): Blackboard key for focus target
	 *   - "interval" (float, optional): Tick interval in seconds (default: 0.5)
	 *   - "random_deviation" (float, optional): Random range added to interval (default: 0.1)
	 */
	TSharedPtr<FJsonObject> HandleAddBTServiceDefaultFocus(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a RunEQS service node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "target_node_index" (int): Node index to attach service to
	 *   - "query_template" (string, optional): EQS query template asset path
	 *   - "bb_key" (string, optional): Blackboard key for storing result
	 *   - "interval" (float, optional): Tick interval in seconds (default: 0.5)
	 *   - "random_deviation" (float, optional): Random range added to interval (default: 0.1)
	 */
	TSharedPtr<FJsonObject> HandleAddBTServiceRunEQS(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------

	/** Load a Behavior Tree asset by name or path. */
	UBehaviorTree* LoadBehaviorTree(const FString& TreeName, FString& OutPath);

	/** Find a graph node by its index in the BTGraph->Nodes array. */
	UEdGraphNode* FindGraphNodeByIndex(UBehaviorTreeGraph* BTGraph, int32 NodeIndex);

	/** Create a success response JSON object. */
	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);

	/** Create an error response JSON object. */
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);

	//-------------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------------

	/** Cache of loaded behavior trees. */
	TMap<FString, TWeakObjectPtr<UBehaviorTree>> ActiveTrees;
};
