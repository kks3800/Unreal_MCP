// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;
class AAIController;
class UWorld;

/**
 * Behavior Tree runtime control and Blackboard value manipulation during PIE.
 * Provides commands to run/stop/pause BTs and get/set blackboard values on live AI actors.
 *
 * PIE ONLY - These commands require Play-In-Editor to be active.
 *
 * Command Categories:
 * - BT Runtime Control: run, stop, pause, resume, restart, get state
 * - Blackboard Values: set, get, get all, clear
 */
class UNREALMCP_API FUnrealMCPBTRuntimeCommands
{
public:
	FUnrealMCPBTRuntimeCommands() = default;

	/**
	 * Handle BT runtime-related commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all BT runtime commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//=========================================================================
	// Behavior Tree Runtime Commands
	//=========================================================================

	/**
	 * Run a Behavior Tree on an AI-controlled pawn.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 *   - "tree_name" (string): Name or path of the Behavior Tree asset to run
	 */
	TSharedPtr<FJsonObject> HandleRunBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Stop the currently running Behavior Tree.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 *   - "stop_mode" (string, optional): "Safe" (default) or "Forced"
	 */
	TSharedPtr<FJsonObject> HandleStopBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Pause the currently running Behavior Tree.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 *   - "reason" (string, optional): Reason for pausing (default: "MCP")
	 */
	TSharedPtr<FJsonObject> HandlePauseBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Resume a previously paused Behavior Tree.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 *   - "reason" (string, optional): Reason for resuming (default: "MCP")
	 */
	TSharedPtr<FJsonObject> HandleResumeBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Restart the currently running Behavior Tree from the root.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 */
	TSharedPtr<FJsonObject> HandleRestartBehaviorTree(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get the runtime state of a Behavior Tree (current tree, active node, paused, etc.).
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 */
	TSharedPtr<FJsonObject> HandleGetBTRuntimeState(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Blackboard Value Commands
	//=========================================================================

	/**
	 * Set a value on the Blackboard component of an AI-controlled pawn.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 *   - "key_name" (string): Blackboard key to set
	 *   - "value_type" (string): Type: Bool, Float, Int, Vector, String, Object, Name, Rotator, Enum, Class
	 *   - "value" (varies): Value to set (type depends on value_type)
	 */
	TSharedPtr<FJsonObject> HandleSetBlackboardValue(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get a single value from the Blackboard component.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 *   - "key_name" (string): Blackboard key to read
	 */
	TSharedPtr<FJsonObject> HandleGetBlackboardValue(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all Blackboard key values for an AI-controlled pawn.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 */
	TSharedPtr<FJsonObject> HandleGetBlackboardState(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Clear (reset to default) a Blackboard key value.
	 *
	 * @param Params JSON with:
	 *   - "actor_name" (string): Name of the pawn or AI controller
	 *   - "key_name" (string): Blackboard key to clear
	 */
	TSharedPtr<FJsonObject> HandleClearBlackboardValue(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------

	/** Find the AI Controller for an actor by name in the PIE world. */
	AAIController* FindAIControllerForActor(UWorld* World, const FString& ActorName);

	/** Get the PIE (Play-In-Editor) world from GEditor. */
	UWorld* GetPIEWorld();

	/** Create a success response JSON object. */
	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);

	/** Create an error response JSON object. */
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);
};
