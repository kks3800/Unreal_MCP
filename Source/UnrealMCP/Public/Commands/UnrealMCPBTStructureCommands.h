// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;
class UBehaviorTree;
class UBehaviorTreeGraphNode;
class UEdGraphNode;

/**
 * Behavior Tree structure manipulation command handlers for Unreal MCP.
 * Provides tree structure operations: connections, reordering, deletion,
 * positioning, inspection, and property editing.
 *
 * EDITOR ONLY - These commands require the BehaviorTreeEditor module.
 *
 * Command Categories:
 * - Connection: connect, disconnect, reorder children
 * - Layout: delete node, move node, auto-arrange
 * - Inspection: tree structure, node info, set node property
 */
class UNREALMCP_API FUnrealMCPBTStructureCommands
{
public:
	FUnrealMCPBTStructureCommands() = default;

	/**
	 * Handle BT structure-related commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all BT structure commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//=========================================================================
	// Connection Commands
	//=========================================================================

	/**
	 * Connect two nodes in a Behavior Tree (parent to child).
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 *   - "parent_index" (int): Index of the parent node in the graph
	 *   - "child_index" (int): Index of the child node to connect
	 */
	TSharedPtr<FJsonObject> HandleConnectBTNodes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Disconnect two nodes in a Behavior Tree.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 *   - "parent_index" (int): Index of the parent node
	 *   - "child_index" (int): Index of the child node to disconnect
	 */
	TSharedPtr<FJsonObject> HandleDisconnectBTNodes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Reorder the children of a composite node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 *   - "parent_index" (int): Index of the parent composite node
	 *   - "child_order" (array of int): New ordering of child indices
	 */
	TSharedPtr<FJsonObject> HandleReorderBTChildren(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Layout Commands
	//=========================================================================

	/**
	 * Delete a node from a Behavior Tree graph.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 *   - "node_index" (int): Index of the node to delete
	 */
	TSharedPtr<FJsonObject> HandleDeleteBTNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Move a node to a new position in the graph editor.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 *   - "node_index" (int): Index of the node to move
	 *   - "x" (float): New X position
	 *   - "y" (float): New Y position
	 */
	TSharedPtr<FJsonObject> HandleMoveBTNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Auto-arrange all nodes in a Behavior Tree graph.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 */
	TSharedPtr<FJsonObject> HandleAutoArrangeBT(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Inspection Commands
	//=========================================================================

	/**
	 * Get the full tree structure as a recursive JSON hierarchy.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 * @return Recursive JSON tree with index, type, name, position,
	 *         decorators, services, and children for each node
	 */
	TSharedPtr<FJsonObject> HandleGetBTTreeStructure(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get detailed information about a single node.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 *   - "node_index" (int): Index of the node to inspect
	 */
	TSharedPtr<FJsonObject> HandleGetBTNodeInfo(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set a property value on a node's runtime instance.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of the behavior tree
	 *   - "node_index" (int): Index of the target node
	 *   - "property_name" (string): Name of the property to set
	 *   - "value" (varies): New value (float, int, bool, string supported)
	 */
	TSharedPtr<FJsonObject> HandleSetBTNodeProperty(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------

	/** Load a Behavior Tree asset by name or path. Uses cache. */
	UBehaviorTree* LoadBehaviorTree(const FString& TreeName, FString& OutPath);

	/** Find a graph node by its index in the BTGraph->Nodes array. */
	UEdGraphNode* FindGraphNodeByIndex(UBehaviorTree* BT, int32 NodeIndex);

	/**
	 * Build recursive JSON representation of a BT node and its children.
	 * Follows output pin links to enumerate children, and includes
	 * decorator/service sub-nodes for each BT graph node.
	 */
	TSharedPtr<FJsonObject> BuildNodeJson(UBehaviorTreeGraphNode* BTNode, UBehaviorTree* BT);

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
