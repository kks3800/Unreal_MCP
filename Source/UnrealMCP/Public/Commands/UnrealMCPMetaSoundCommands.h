// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;

#include "MetasoundBuilderSubsystem.h"
#include "MetasoundSource.h"

/**
 * MetaSound creation and editing command handlers for Unreal MCP.
 * Provides full MetaSound Source graph creation via MCP commands.
 *
 * EDITOR ONLY - These commands require the MetasoundEditor module.
 *
 * Command Categories:
 * - Asset Creation: CreateMetaSoundSource, CreatePreset, Delete, GetInfo
 * - Graph I/O: AddInput, AddOutput, GetInputs, GetOutputs, SetInputDefault
 * - Node Management: AddNode, DeleteNode, GetNodes, FindNode
 * - Connections: ConnectNodes, DisconnectNodes, ConnectToGraphOutput
 * - Variables: AddVariable, GetVariableGetter, GetVariableSetter
 * - Building: BuildMetaSound, BuildAndOverwrite, Validate
 * - Utilities: ListNodeTypes, GetDataTypes, AutoLayout
 */
class UNREALMCP_API FUnrealMCPMetaSoundCommands
{
public:
	FUnrealMCPMetaSoundCommands() = default;

	/**
	 * Handle MetaSound-related commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all MetaSound commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//-------------------------------------------------------------------------
	// Asset Creation Commands
	//-------------------------------------------------------------------------

	/**
	 * Create a new MetaSound Source asset.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Name for the new MetaSound
	 *   - "path" (string, optional): Content path (default: /Game/MetaSounds)
	 *   - "output_format" (string, optional): Mono, Stereo, Quad, FiveDotOne (default: Mono)
	 *   - "is_one_shot" (bool, optional): Whether this is a one-shot sound (default: true)
	 */
	TSharedPtr<FJsonObject> HandleCreateMetaSoundSource(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Create a MetaSound preset from an existing source.
	 *
	 * @param Params JSON with:
	 *   - "preset_name" (string): Name for the new preset
	 *   - "source_name" (string): Source MetaSound to copy from
	 *   - "path" (string, optional): Content path (default: /Game/MetaSounds)
	 */
	TSharedPtr<FJsonObject> HandleCreateMetaSoundPreset(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Delete a MetaSound asset.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Name of MetaSound to delete
	 */
	TSharedPtr<FJsonObject> HandleDeleteMetaSound(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get information about a MetaSound asset.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Name of MetaSound to query
	 */
	TSharedPtr<FJsonObject> HandleGetMetaSoundInfo(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List MetaSound assets in a path.
	 *
	 * @param Params JSON with:
	 *   - "path" (string, optional): Content path to search (default: /Game)
	 *   - "recursive" (bool, optional): Search recursively (default: true)
	 */
	TSharedPtr<FJsonObject> HandleListMetaSoundAssets(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Open an existing MetaSound for editing.
	 * Creates a builder from the asset so you can add/modify nodes, then save.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Name or path of the MetaSound to open
	 *   - "builder_name" (string, optional): Name for the builder (default: same as asset name)
	 */
	TSharedPtr<FJsonObject> HandleOpenMetaSound(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Graph Input/Output Commands
	//-------------------------------------------------------------------------

	/**
	 * Add an input to the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "input_name" (string): Name for the input
	 *   - "data_type" (string): Float, Int32, Bool, String, Audio, Time, Trigger, WaveAsset
	 *   - "default_value" (varies, optional): Default value for the input
	 *   - "is_constructor" (bool, optional): Whether this is a constructor input (default: false)
	 */
	TSharedPtr<FJsonObject> HandleAddMetaSoundInput(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add an output to the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "output_name" (string): Name for the output
	 *   - "data_type" (string): Data type for the output
	 */
	TSharedPtr<FJsonObject> HandleAddMetaSoundOutput(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove an input from the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "input_name" (string): Name of input to remove
	 */
	TSharedPtr<FJsonObject> HandleRemoveMetaSoundInput(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove an output from the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "output_name" (string): Name of output to remove
	 */
	TSharedPtr<FJsonObject> HandleRemoveMetaSoundOutput(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all inputs in a MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 */
	TSharedPtr<FJsonObject> HandleGetMetaSoundInputs(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all outputs in a MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 */
	TSharedPtr<FJsonObject> HandleGetMetaSoundOutputs(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set the default value for a graph input.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "input_name" (string): Name of input
	 *   - "value" (varies): New default value
	 */
	TSharedPtr<FJsonObject> HandleSetInputDefault(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Node Management Commands
	//-------------------------------------------------------------------------

	/**
	 * Add a node to the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "node_type" (string): Node class name (e.g., "UE.Generators.Sine")
	 *   - "node_name" (string, optional): Display name for the node
	 *   - "position" (array, optional): [X, Y] position in graph
	 */
	TSharedPtr<FJsonObject> HandleAddMetaSoundNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Delete a node from the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "node_id" (string): ID of node to delete
	 */
	TSharedPtr<FJsonObject> HandleDeleteMetaSoundNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all nodes in a MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 */
	TSharedPtr<FJsonObject> HandleGetMetaSoundNodes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Find a node by name or type.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "node_name" (string, optional): Name to search for
	 *   - "node_type" (string, optional): Type to search for
	 */
	TSharedPtr<FJsonObject> HandleFindMetaSoundNode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get input pins for a node.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "node_id" (string): Target node
	 */
	TSharedPtr<FJsonObject> HandleGetNodeInputs(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get output pins for a node.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "node_id" (string): Target node
	 */
	TSharedPtr<FJsonObject> HandleGetNodeOutputs(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set the default value for a node input pin.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "node_id" (string): Target node
	 *   - "input_name" (string): Name of input pin
	 *   - "value" (varies): New default value
	 */
	TSharedPtr<FJsonObject> HandleSetNodeInputDefault(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Connection Commands
	//-------------------------------------------------------------------------

	/**
	 * Connect two nodes in the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "from_node" (string): Source node ID
	 *   - "from_output" (string): Output pin name
	 *   - "to_node" (string): Destination node ID
	 *   - "to_input" (string): Input pin name
	 */
	TSharedPtr<FJsonObject> HandleConnectMetaSoundNodes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Disconnect two nodes in the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "from_node" (string): Source node ID
	 *   - "from_output" (string): Output pin name
	 *   - "to_node" (string): Destination node ID
	 *   - "to_input" (string): Input pin name
	 */
	TSharedPtr<FJsonObject> HandleDisconnectMetaSoundNodes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Connect a node to a graph output.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "from_node" (string): Source node ID
	 *   - "from_output" (string): Output pin name
	 *   - "graph_output" (string): Graph output name
	 */
	TSharedPtr<FJsonObject> HandleConnectToGraphOutput(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get connections for a node.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "node_id" (string): Target node
	 */
	TSharedPtr<FJsonObject> HandleGetNodeConnections(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Variable Commands
	//-------------------------------------------------------------------------

	/**
	 * Add a variable to the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "variable_name" (string): Name for the variable
	 *   - "data_type" (string): Data type for the variable
	 *   - "default_value" (varies, optional): Default value
	 */
	TSharedPtr<FJsonObject> HandleAddMetaSoundVariable(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a variable getter node.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "variable_name" (string): Variable to get
	 */
	TSharedPtr<FJsonObject> HandleAddVariableGetter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a variable setter node.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 *   - "variable_name" (string): Variable to set
	 */
	TSharedPtr<FJsonObject> HandleAddVariableSetter(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Build Commands
	//-------------------------------------------------------------------------

	/**
	 * Build the MetaSound and save as a new asset.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Builder name to build from
	 *   - "asset_name" (string, optional): Name for saved asset (default: same as builder)
	 *   - "path" (string, optional): Save path (default: /Game/MetaSounds)
	 */
	TSharedPtr<FJsonObject> HandleBuildMetaSound(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Build and overwrite an existing MetaSound asset.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Builder name to build from
	 *   - "target_asset" (string): Asset to overwrite
	 */
	TSharedPtr<FJsonObject> HandleBuildAndOverwriteMetaSound(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Validate a MetaSound for errors.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): MetaSound to validate
	 */
	TSharedPtr<FJsonObject> HandleValidateMetaSound(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Utility Commands
	//-------------------------------------------------------------------------

	/**
	 * List all registered MetaSound node types.
	 *
	 * @param Params JSON with:
	 *   - "category" (string, optional): Filter by category (Generators, Filters, Math, etc.)
	 */
	TSharedPtr<FJsonObject> HandleListMetaSoundNodeTypes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get available MetaSound data types.
	 */
	TSharedPtr<FJsonObject> HandleGetMetaSoundDataTypes(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Auto-layout nodes in the MetaSound graph.
	 *
	 * @param Params JSON with:
	 *   - "sound_name" (string): Target MetaSound
	 */
	TSharedPtr<FJsonObject> HandleAutoLayoutMetaSound(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------

	/** Get or create a builder for the given sound name */
	UMetaSoundSourceBuilder* GetOrCreateBuilder(const FString& SoundName, EMetaSoundBuilderResult& OutResult);

	/** Find an existing builder by name */
	UMetaSoundBuilderBase* FindBuilder(const FString& SoundName);

	/** Load a MetaSound Source asset by name or path */
	UMetaSoundSource* LoadMetaSoundSource(const FString& SoundName, FString& OutPath);

	/** Parse a node class name string into FMetasoundFrontendClassName */
	FMetasoundFrontendClassName ParseClassName(const FString& NodeType);

	/** Convert string to EMetaSoundOutputAudioFormat */
	EMetaSoundOutputAudioFormat StringToOutputFormat(const FString& FormatStr);

	/** Create a literal value from JSON */
	FMetasoundFrontendLiteral CreateLiteralFromJson(const FString& DataType, const TSharedPtr<FJsonValue>& Value);

	/** Create a success response JSON object */
	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);

	/** Create an error response JSON object */
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);

	/** Convert EMetaSoundBuilderResult to string for error messages */
	static FString BuilderResultToString(EMetaSoundBuilderResult Result);

	/** Populate node and input caches from an existing builder (after opening an asset) */
	void PopulateCachesFromBuilder(const FString& SoundName, UMetaSoundSourceBuilder* Builder);

	//-------------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------------

	/** Cache of active builders (sound_name -> builder) */
	TMap<FString, TWeakObjectPtr<UMetaSoundSourceBuilder>> ActiveBuilders;

	/** Cache of node handles for reference (sound_name -> (node_name -> handle)) */
	TMap<FString, TMap<FString, FMetaSoundNodeHandle>> NodeHandleCache;

	/** Cache of graph input output handles (sound_name -> (input_name -> output_handle)) */
	TMap<FString, TMap<FString, FMetaSoundBuilderNodeOutputHandle>> GraphInputOutputCache;
};
