// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;

/**
 * Handler class for Blueprint inspection MCP commands.
 * All commands are read-only - they inspect but don't modify blueprints.
 */
class UNREALMCP_API FUnrealMCPBlueprintInspectCommands
{
public:
	FUnrealMCPBlueprintInspectCommands() = default;

	/**
	 * Handle blueprint inspection commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all blueprint inspect commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

private:
	/** Get high-level blueprint info: parent class, compile status, graphs, variables, components. */
	TSharedPtr<FJsonObject> HandleGetBlueprintInfo(const TSharedPtr<FJsonObject>& Params);

	/** Get all graphs in a blueprint with name, type, and node count. */
	TSharedPtr<FJsonObject> HandleGetBlueprintGraphs(const TSharedPtr<FJsonObject>& Params);

	/** Get all nodes in a specific graph. */
	TSharedPtr<FJsonObject> HandleGetGraphNodes(const TSharedPtr<FJsonObject>& Params);

	/** Get detailed info about a specific node including all pins. */
	TSharedPtr<FJsonObject> HandleGetNodeInfo(const TSharedPtr<FJsonObject>& Params);

	/** Get all pins for a specific node. */
	TSharedPtr<FJsonObject> HandleGetNodePins(const TSharedPtr<FJsonObject>& Params);

	/** Get all variables defined in a blueprint. */
	TSharedPtr<FJsonObject> HandleGetBlueprintVariables(const TSharedPtr<FJsonObject>& Params);

	/** Get all connections (wires) in a graph. */
	TSharedPtr<FJsonObject> HandleGetBlueprintConnections(const TSharedPtr<FJsonObject>& Params);

	/** Compile blueprint and return status with any error/warning messages. */
	TSharedPtr<FJsonObject> HandleGetCompileStatus(const TSharedPtr<FJsonObject>& Params);

	// Sprint 6: QoL Inspection
	/** Find all unconnected (dangling) pins in a graph. */
	TSharedPtr<FJsonObject> HandleGetUnconnectedPins(const TSharedPtr<FJsonObject>& Params);

	// Sprint 6: Snapshot Commands
	/** Get combined blueprint info, variables, components, graphs, and compile status in a single call. */
	TSharedPtr<FJsonObject> HandleGetBlueprintSnapshot(const TSharedPtr<FJsonObject>& Params);

	/** Get all nodes in a graph with inline connection data per pin. */
	TSharedPtr<FJsonObject> HandleGetGraphSnapshot(const TSharedPtr<FJsonObject>& Params);

	// Blueprint analysis commands
	/** Full blueprint content dump: interfaces, dispatchers, function metadata, component props, semantic nodes. */
	TSharedPtr<FJsonObject> HandleReadBlueprintContent(const TSharedPtr<FJsonObject>& Params);

	/** Analyze a specific graph: semantic node identity and execution flow tracing. */
	TSharedPtr<FJsonObject> HandleAnalyzeBlueprintGraph(const TSharedPtr<FJsonObject>& Params);

	// Helper: extract semantic metadata from a node (function called, variable name, event, etc.)
	static void AddNodeSemanticData(UEdGraphNode* Node, TSharedPtr<FJsonObject>& NodeObj);
};
