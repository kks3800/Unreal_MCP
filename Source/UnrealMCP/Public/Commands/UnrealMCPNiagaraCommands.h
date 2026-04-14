// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;
class UNiagaraSystem;
struct FNiagaraEmitterHandle;

/**
 * Niagara particle system creation and editing command handlers for Unreal MCP.
 * Provides comprehensive Niagara system management via MCP commands.
 *
 * EDITOR ONLY - These commands require the NiagaraEditor module.
 *
 * Command Categories:
 * - Sprint 1: Asset Management (create, delete, list, compile, save)
 * - Sprint 2: Emitter Management (add, remove, configure emitters)
 * - Sprint 3: Renderer Configuration (sprite, mesh, ribbon, light)
 * - Sprint 4: Module Operations (spawn, update, particle stacks)
 * - Sprint 5: Parameter System (user params, bindings)
 * - Sprint 6: Material Integration (particle materials, bindings)
 */
class UNREALMCP_API FUnrealMCPNiagaraCommands
{
public:
	FUnrealMCPNiagaraCommands() = default;

	/**
	 * Handle Niagara-related commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all Niagara commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//=========================================================================
	// Sprint 1: Asset Management Commands
	//=========================================================================

	/**
	 * Create a new Niagara System asset.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Name for the new system
	 *   - "path" (string, optional): Content path (default: /Game/FX)
	 *   - "create_default_emitter" (bool, optional): Create a default emitter (default: true)
	 */
	TSharedPtr<FJsonObject> HandleCreateNiagaraSystem(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Delete a Niagara System asset.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Name of system to delete
	 */
	TSharedPtr<FJsonObject> HandleDeleteNiagaraSystem(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get information about a Niagara System.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Name of system to query
	 */
	TSharedPtr<FJsonObject> HandleGetNiagaraSystemInfo(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List Niagara Systems in a path.
	 *
	 * @param Params JSON with:
	 *   - "path" (string, optional): Content path to search (default: /Game)
	 *   - "recursive" (bool, optional): Search recursively (default: true)
	 */
	TSharedPtr<FJsonObject> HandleListNiagaraSystems(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Open a Niagara System for editing.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Name or path of system to open
	 */
	TSharedPtr<FJsonObject> HandleOpenNiagaraSystem(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Request compilation of a Niagara System.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Name of system to compile
	 */
	TSharedPtr<FJsonObject> HandleCompileNiagaraSystem(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Save a Niagara System asset.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Name of system to save
	 */
	TSharedPtr<FJsonObject> HandleSaveNiagaraSystem(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set Niagara System properties.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "fixed_bounds" (array, optional): [MinX,MinY,MinZ,MaxX,MaxY,MaxZ]
	 *   - "warmup_time" (float, optional): System warmup time
	 *   - "determinism" (bool, optional): Enable deterministic simulation
	 */
	TSharedPtr<FJsonObject> HandleSetNiagaraSystemProperties(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Sprint 2: Emitter Management Commands
	//=========================================================================

	/**
	 * Add an emitter to a Niagara System.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Name for the new emitter
	 *   - "template_name" (string, optional): Template emitter to copy from
	 */
	TSharedPtr<FJsonObject> HandleAddEmitterToSystem(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove an emitter from a Niagara System.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Name of emitter to remove
	 */
	TSharedPtr<FJsonObject> HandleRemoveEmitterFromSystem(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Duplicate an emitter within a system.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "source_emitter" (string): Emitter to duplicate
	 *   - "new_name" (string): Name for the duplicate
	 */
	TSharedPtr<FJsonObject> HandleDuplicateEmitter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Enable an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Emitter to enable
	 */
	TSharedPtr<FJsonObject> HandleEnableEmitter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Disable an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Emitter to disable
	 */
	TSharedPtr<FJsonObject> HandleDisableEmitter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Rename an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Current emitter name
	 *   - "new_name" (string): New name for the emitter
	 */
	TSharedPtr<FJsonObject> HandleRenameEmitter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get emitter information.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Emitter to query
	 */
	TSharedPtr<FJsonObject> HandleGetEmitterInfo(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List all emitters in a system.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 */
	TSharedPtr<FJsonObject> HandleListEmitters(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set emitter simulation mode.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "mode" (string): "Standard" or "Stateless"
	 */
	TSharedPtr<FJsonObject> HandleSetEmitterMode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Toggle emitter isolation for debugging.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Emitter to isolate
	 *   - "isolated" (bool): Whether to isolate
	 */
	TSharedPtr<FJsonObject> HandleIsolateEmitter(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Sprint 3: Renderer Configuration Commands
	//=========================================================================

	/**
	 * Add a sprite renderer to an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "material" (string, optional): Material path
	 *   - "alignment" (string, optional): Alignment mode
	 *   - "facing_mode" (string, optional): Facing mode
	 */
	TSharedPtr<FJsonObject> HandleAddSpriteRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure sprite renderer properties.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int, optional): Renderer index (default: 0)
	 *   - Various sprite renderer properties
	 */
	TSharedPtr<FJsonObject> HandleConfigureSpriteRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a mesh renderer to an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "mesh" (string, optional): Static mesh path
	 *   - "material" (string, optional): Material path
	 */
	TSharedPtr<FJsonObject> HandleAddMeshRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure mesh renderer properties.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int, optional): Renderer index
	 *   - Various mesh renderer properties
	 */
	TSharedPtr<FJsonObject> HandleConfigureMeshRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a ribbon renderer to an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "material" (string, optional): Material path
	 */
	TSharedPtr<FJsonObject> HandleAddRibbonRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure ribbon renderer properties.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int, optional): Renderer index
	 *   - Various ribbon renderer properties
	 */
	TSharedPtr<FJsonObject> HandleConfigureRibbonRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a light renderer to an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 */
	TSharedPtr<FJsonObject> HandleAddLightRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure light renderer properties.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int, optional): Renderer index
	 *   - "radius_scale" (float, optional): Light radius scale
	 *   - "intensity_scale" (float, optional): Light intensity scale
	 */
	TSharedPtr<FJsonObject> HandleConfigureLightRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove a renderer from an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index to remove
	 */
	TSharedPtr<FJsonObject> HandleRemoveRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all renderers for an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 */
	TSharedPtr<FJsonObject> HandleGetRenderers(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set renderer material.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index
	 *   - "material" (string): Material path
	 */
	TSharedPtr<FJsonObject> HandleSetRendererMaterial(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set renderer visibility tag.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index
	 *   - "visibility_tag" (int): Visibility tag value
	 */
	TSharedPtr<FJsonObject> HandleSetRendererVisibility(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set renderer sort mode.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index
	 *   - "sort_mode" (string): Sort mode name
	 */
	TSharedPtr<FJsonObject> HandleSetRendererSortMode(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set renderer attribute bindings.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index
	 *   - "bindings" (object): Binding name-value pairs
	 */
	TSharedPtr<FJsonObject> HandleSetRendererBindings(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Sprint 4: Module Operations Commands
	//=========================================================================

	/**
	 * Add a module to the Emitter Spawn stack.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "module_path" (string): Module script path
	 *   - "index" (int, optional): Insert position
	 */
	TSharedPtr<FJsonObject> HandleAddSpawnModule(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a module to the Emitter Update stack.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "module_path" (string): Module script path
	 *   - "index" (int, optional): Insert position
	 */
	TSharedPtr<FJsonObject> HandleAddUpdateModule(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a module to the Particle Spawn stack.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "module_path" (string): Module script path
	 *   - "index" (int, optional): Insert position
	 */
	TSharedPtr<FJsonObject> HandleAddParticleSpawnModule(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add a module to the Particle Update stack.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "module_path" (string): Module script path
	 *   - "index" (int, optional): Insert position
	 */
	TSharedPtr<FJsonObject> HandleAddParticleUpdateModule(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove a module from a stack.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "stack" (string): Stack type (EmitterSpawn, EmitterUpdate, ParticleSpawn, ParticleUpdate)
	 *   - "module_index" (int): Module index to remove
	 */
	TSharedPtr<FJsonObject> HandleRemoveModule(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all modules in an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 */
	TSharedPtr<FJsonObject> HandleGetModules(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set module enabled state.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "stack" (string): Stack type
	 *   - "module_index" (int): Module index
	 *   - "enabled" (bool): Enabled state
	 */
	TSharedPtr<FJsonObject> HandleSetModuleEnabled(const TSharedPtr<FJsonObject>& Params);

	/**
	 * List available module types.
	 *
	 * @param Params JSON with:
	 *   - "category" (string, optional): Filter by category
	 */
	TSharedPtr<FJsonObject> HandleListAvailableModules(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure a module input parameter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "stack" (string): Stack type
	 *   - "module_index" (int): Module index
	 *   - "input_name" (string): Input parameter name
	 *   - "value" (varies): Parameter value
	 */
	TSharedPtr<FJsonObject> HandleConfigureModuleInput(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get module input parameters.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "stack" (string): Stack type
	 *   - "module_index" (int): Module index
	 */
	TSharedPtr<FJsonObject> HandleGetModuleInputs(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Reorder modules in a stack.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "stack" (string): Stack type
	 *   - "from_index" (int): Source index
	 *   - "to_index" (int): Target index
	 */
	TSharedPtr<FJsonObject> HandleReorderModules(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add an event handler to an emitter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "event_name" (string): Event to handle
	 *   - "handler_script" (string): Handler script path
	 */
	TSharedPtr<FJsonObject> HandleAddEventHandler(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Sprint 5: Parameter System Commands
	//=========================================================================

	/**
	 * Expose a user parameter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "param_name" (string): Parameter name
	 *   - "param_type" (string): Parameter type (Float, Int32, Bool, Vector, Color, etc.)
	 *   - "default_value" (varies, optional): Default value
	 */
	TSharedPtr<FJsonObject> HandleExposeUserParameter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set user parameter default value.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "param_name" (string): Parameter name
	 *   - "value" (varies): New default value
	 */
	TSharedPtr<FJsonObject> HandleSetUserParameterDefault(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all user parameters.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 */
	TSharedPtr<FJsonObject> HandleGetUserParameters(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Remove a user parameter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "param_name" (string): Parameter to remove
	 */
	TSharedPtr<FJsonObject> HandleRemoveUserParameter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Create a parameter binding.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string, optional): Emitter name (for emitter params)
	 *   - "source_param" (string): Source parameter
	 *   - "target_param" (string): Target parameter
	 */
	TSharedPtr<FJsonObject> HandleBindParameter(const TSharedPtr<FJsonObject>& Params);
	/**
	 * Set emitter-level parameter.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "param_name" (string): Parameter name
	 *   - "value" (varies): Parameter value
	 */
	TSharedPtr<FJsonObject> HandleSetEmitterParameter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set particle attribute.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "attribute_name" (string): Attribute name
	 *   - "value" (varies): Attribute value
	 */
	TSharedPtr<FJsonObject> HandleSetParticleParameter(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Get all parameter bindings.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 */
	TSharedPtr<FJsonObject> HandleGetParameterBindings(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Reference a parameter collection.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "collection_path" (string): Parameter collection asset path
	 */
	TSharedPtr<FJsonObject> HandleCreateParameterCollection(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Override parameter collection values.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "collection_path" (string): Parameter collection path
	 *   - "overrides" (object): Parameter override values
	 */
	TSharedPtr<FJsonObject> HandleOverrideParameterCollection(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Sprint 6: Material Integration Commands
	//=========================================================================

	/**
	 * Create a particle-compatible material.
	 *
	 * @param Params JSON with:
	 *   - "material_name" (string): Name for the new material
	 *   - "path" (string, optional): Content path
	 *   - "blend_mode" (string, optional): Opaque, Translucent, Additive, Modulate
	 *   - "for_sprites" (bool, optional): Enable for sprite particles
	 *   - "for_meshes" (bool, optional): Enable for mesh particles
	 *   - "for_ribbons" (bool, optional): Enable for ribbon particles
	 */
	TSharedPtr<FJsonObject> HandleCreateParticleMaterial(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Assign material to renderer.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index
	 *   - "material" (string): Material path
	 */
	TSharedPtr<FJsonObject> HandleAssignMaterialToRenderer(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Set up dynamic material parameter binding.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index
	 *   - "niagara_param" (string): Niagara parameter name
	 *   - "material_param" (string): Material parameter name
	 */
	TSharedPtr<FJsonObject> HandleSetDynamicMaterialBinding(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Create a material instance for particles.
	 *
	 * @param Params JSON with:
	 *   - "instance_name" (string): Name for the instance
	 *   - "parent_material" (string): Parent material path
	 *   - "path" (string, optional): Content path
	 */
	TSharedPtr<FJsonObject> HandleCreateParticleMaterialInstance(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Configure material parameter bindings.
	 *
	 * @param Params JSON with:
	 *   - "system_name" (string): Target system
	 *   - "emitter_name" (string): Target emitter
	 *   - "renderer_index" (int): Renderer index
	 *   - "bindings" (object): Parameter binding configuration
	 */
	TSharedPtr<FJsonObject> HandleConfigureMaterialParameters(const TSharedPtr<FJsonObject>& Params);

	/**
	 * Add ParticleColor expression to material.
	 *
	 * @param Params JSON with:
	 *   - "material_name" (string): Target material
	 *   - "connect_to" (string, optional): Output to connect to (BaseColor, EmissiveColor)
	 */
	TSharedPtr<FJsonObject> HandleAddParticleColorNode(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------

	/** Load a Niagara System asset by name or path */
	UNiagaraSystem* LoadNiagaraSystem(const FString& SystemName, FString& OutPath);

	/** Find an emitter handle by name within a system */
	FNiagaraEmitterHandle* FindEmitterHandle(UNiagaraSystem* System, const FString& EmitterName);

	/** Get emitter version GUID from handle */
	FGuid GetEmitterVersion(const FNiagaraEmitterHandle& Handle);

	/** Create a success response JSON object */
	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);

	/** Create an error response JSON object */
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);

	//-------------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------------

	/** Cache of active systems (system_name -> system) */
	TMap<FString, TWeakObjectPtr<UNiagaraSystem>> ActiveSystems;
};
