// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;
class UBehaviorTree;
class UBlackboardData;

/**
 * Behavior Tree and Blackboard asset management command handlers for Unreal MCP.
 * Provides BT/BB creation, deletion, listing, and blackboard key manipulation.
 *
 * EDITOR ONLY - These commands require the BehaviorTreeEditor module.
 *
 * Command Categories:
 * - BT Asset Management: create, delete, list, info, save, open
 * - Blackboard Management: create, delete, list, info, save, key CRUD
 */
class UNREALMCP_API FUnrealMCPBTAssetCommands
{
public:
	FUnrealMCPBTAssetCommands() = default;

	/**
	 * Handle BT/BB asset-related commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all BT/BB asset commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//=========================================================================
	// Behavior Tree Asset Commands
	//=========================================================================

	/**
	 * Create a new Behavior Tree asset.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name for the new behavior tree
	 *   - "path" (string, optional): Content path (default: /Game/AI)
	 *   - "blackboard_asset" (string, optional): Blackboard to associate
	 */
	TSharedPtr<FJsonObject> HandleCreateBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Delete a Behavior Tree asset.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of tree to delete
	 */
	TSharedPtr<FJsonObject> HandleDeleteBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List Behavior Tree assets in a path.
	 *
	 * @param Params JSON with:
	 *   - "path" (string, optional): Content path to search (default: /Game)
	 *   - "recursive" (bool, optional): Search recursively (default: true)
	 */
	TSharedPtr<FJsonObject> HandleListBehaviorTrees(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get information about a Behavior Tree.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of tree to query
	 */
	TSharedPtr<FJsonObject> HandleGetBehaviorTreeInfo(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Save a Behavior Tree asset.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of tree to save
	 */
	TSharedPtr<FJsonObject> HandleSaveBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Open a Behavior Tree for editing.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Name or path of tree to open
	 */
	TSharedPtr<FJsonObject> HandleOpenBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set the Blackboard asset for a Behavior Tree.
	 *
	 * @param Params JSON with:
	 *   - "tree_name" (string): Target behavior tree
	 *   - "blackboard_name" (string): Blackboard asset to associate
	 */
	TSharedPtr<FJsonObject> HandleSetBehaviorTreeBlackboard(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Blackboard Asset Commands
	//=========================================================================

	/**
	 * Create a new Blackboard Data asset.
	 *
	 * @param Params JSON with:
	 *   - "bb_name" (string): Name for the new blackboard
	 *   - "path" (string, optional): Content path (default: /Game/AI)
	 *   - "parent_bb" (string, optional): Parent blackboard for key inheritance
	 */
	TSharedPtr<FJsonObject> HandleCreateBlackboard(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Delete a Blackboard Data asset.
	 *
	 * @param Params JSON with:
	 *   - "bb_name" (string): Name or path of blackboard to delete
	 */
	TSharedPtr<FJsonObject> HandleDeleteBlackboard(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List Blackboard Data assets in a path.
	 *
	 * @param Params JSON with:
	 *   - "path" (string, optional): Content path to search (default: /Game)
	 *   - "recursive" (bool, optional): Search recursively (default: true)
	 */
	TSharedPtr<FJsonObject> HandleListBlackboards(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get information about a Blackboard asset.
	 *
	 * @param Params JSON with:
	 *   - "bb_name" (string): Name or path of blackboard to query
	 */
	TSharedPtr<FJsonObject> HandleGetBlackboardInfo(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Save a Blackboard Data asset.
	 *
	 * @param Params JSON with:
	 *   - "bb_name" (string): Name or path of blackboard to save
	 */
	TSharedPtr<FJsonObject> HandleSaveBlackboard(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a key to a Blackboard asset.
	 *
	 * @param Params JSON with:
	 *   - "bb_name" (string): Target blackboard
	 *   - "key_name" (string): Name for the new key
	 *   - "key_type" (string): Type: Bool, Float, Int, Vector, Object, String, Class, Enum, Name, Rotator
	 *   - "description" (string, optional): Key description
	 *   - "instance_synced" (bool, optional): Sync across instances (default: false)
	 */
	TSharedPtr<FJsonObject> HandleAddBlackboardKey(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove a key from a Blackboard asset.
	 *
	 * @param Params JSON with:
	 *   - "bb_name" (string): Target blackboard
	 *   - "key_name" (string): Key to remove
	 */
	TSharedPtr<FJsonObject> HandleRemoveBlackboardKey(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Modify an existing Blackboard key.
	 *
	 * @param Params JSON with:
	 *   - "bb_name" (string): Target blackboard
	 *   - "key_name" (string): Key to modify
	 *   - "new_name" (string, optional): Rename the key
	 *   - "description" (string, optional): Update description
	 *   - "instance_synced" (bool, optional): Update sync setting
	 */
	TSharedPtr<FJsonObject> HandleModifyBlackboardKey(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------

	/** Load a Behavior Tree asset by name or path. */
	UBehaviorTree* LoadBehaviorTree(const FString& TreeName, FString& OutPath);

	/** Load a Blackboard Data asset by name or path. */
	UBlackboardData* LoadBlackboardData(const FString& BBName, FString& OutPath);

	/** Create a success response JSON object. */
	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);

	/** Create an error response JSON object. */
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);

	//-------------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------------

	/** Cache of loaded behavior trees. */
	TMap<FString, TWeakObjectPtr<UBehaviorTree>> ActiveTrees;

	/** Cache of loaded blackboard data assets. */
	TMap<FString, TWeakObjectPtr<UBlackboardData>> ActiveBlackboards;
};
