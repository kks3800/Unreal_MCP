// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;

/**
 * Procedural Content Generation (PCG) command handlers for Unreal MCP.
 * Provides programmatic authoring of PCG graphs, nodes, and instances.
 *
 * EDITOR ONLY - Requires PCG and PCGEditor modules.
 *
 * Phase 1 command coverage:
 *  - pcg_ping                                           : bridge wiring test
 *  - create/delete/duplicate/rename/list/save_pcg_graph : asset-level CRUD
 *  - list_pcg_node_types, get_pcg_node_schema           : reflection discovery
 *  - begin_pcg_edit, end_pcg_edit                       : batched session
 *  - add/delete/move_pcg_node                           : graph construction
 *  - connect_pcg_nodes, disconnect_pcg_pins             : edge management
 *  - auto_layout_pcg_graph                              : layering layout
 *  - set/get_pcg_node_property                          : scalar + nested access
 *  - add/remove/clear_pcg_array_item                    : array manipulation
 *  - get_pcg_graph_snapshot, get_pcg_node_info,
 *    list_pcg_node_pins                                 : introspection
 *
 * Phase 2 (component application, spec round-trip, subgraphs) arrives later.
 */
class UNREALMCP_API FUnrealMCPPCGCommands
{
public:
	FUnrealMCPPCGCommands() = default;

	/** Handle PCG-related commands. */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all PCG commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//=========================================================================
	// Handlers
	//=========================================================================

	TSharedPtr<FJsonObject> HandlePCGPing(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleCreatePCGGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeletePCGGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDuplicatePCGGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleRenamePCGGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListPCGGraphs(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSavePCGGraph(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleListPCGNodeTypes(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetPCGNodeSchema(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleBeginPCGEdit(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleEndPCGEdit(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleAddPCGNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeletePCGNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleMovePCGNode(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleSetPCGNodeProperty(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetPCGNodeProperty(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleAddPCGArrayItem(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleRemovePCGArrayItem(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleClearPCGArray(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleConnectPCGNodes(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDisconnectPCGPins(const TSharedPtr<FJsonObject>& Params);

	TSharedPtr<FJsonObject> HandleAutoLayoutPCGGraph(const TSharedPtr<FJsonObject>& Params);

	// Introspection / round-trip
	TSharedPtr<FJsonObject> HandleGetPCGGraphSnapshot(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetPCGNodeInfo(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListPCGNodePins(const TSharedPtr<FJsonObject>& Params);

	// Display / annotation — node titles, sticky-note comments, and comment-box frames
	TSharedPtr<FJsonObject> HandleSetPCGNodeTitle(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetPCGNodeComment(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddPCGCommentBox(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleFramePCGNodes(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Response Helpers
	//-------------------------------------------------------------------------

	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);
};
