// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

// Forward declarations
class FMCPCommandRegistry;
class UMaterialFunction;
class UMaterialInstanceConstant;
enum EMaterialDomain : int;
enum EFunctionInputType : int;
enum ETranslucencyLightingMode : int;
enum ERefractionMode : int;
enum EDecalBlendMode : int;
enum EBlendableLocation : int;
enum EMaterialStencilCompare : int;
enum EMaterialTranslucencyPass : int;

/**
 * Material creation and editing command handlers for Unreal MCP.
 * Provides full material node graph creation, material property control,
 * and material function support via MCP commands.
 *
 * EDITOR ONLY - These commands require the MaterialEditor module.
 */
class UNREALMCP_API FUnrealMCPMaterialCommands
{
public:
	FUnrealMCPMaterialCommands() = default;

	/**
	 * Handle material-related commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all material commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//-------------------------------------------------------------------------
	// Material Commands
	//-------------------------------------------------------------------------

	/** Create a new Material asset. */
	static TSharedPtr<FJsonObject> HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params);

	/** Set material properties (domain, blend mode, shading model, translucency, usage flags, etc.). */
	static TSharedPtr<FJsonObject> HandleSetMaterialProperties(const TSharedPtr<FJsonObject>& Params);

	/** Add a material expression node to a material. */
	static TSharedPtr<FJsonObject> HandleAddMaterialNode(const TSharedPtr<FJsonObject>& Params);

	/** Set properties on an existing material expression node. */
	static TSharedPtr<FJsonObject> HandleSetMaterialNodeProperty(const TSharedPtr<FJsonObject>& Params);

	/** Connect two material expression nodes. */
	static TSharedPtr<FJsonObject> HandleConnectMaterialNodes(const TSharedPtr<FJsonObject>& Params);

	/** Connect a material expression to a material output property. */
	static TSharedPtr<FJsonObject> HandleConnectToMaterialOutput(const TSharedPtr<FJsonObject>& Params);

	/** Delete a material expression node. */
	static TSharedPtr<FJsonObject> HandleDeleteMaterialNode(const TSharedPtr<FJsonObject>& Params);

	/** Recompile a material after making changes. */
	static TSharedPtr<FJsonObject> HandleRecompileMaterial(const TSharedPtr<FJsonObject>& Params);

	/** Create a Material Instance Constant from a parent material. */
	static TSharedPtr<FJsonObject> HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params);

	/** Set a parameter value on a Material Instance. */
	static TSharedPtr<FJsonObject> HandleSetMaterialInstanceParameter(const TSharedPtr<FJsonObject>& Params);

	/** Get all overridden parameter values from a Material Instance. */
	static TSharedPtr<FJsonObject> HandleGetMaterialInstanceParameters(const TSharedPtr<FJsonObject>& Params);

	/** Get all expression nodes in a material. */
	static TSharedPtr<FJsonObject> HandleGetMaterialNodes(const TSharedPtr<FJsonObject>& Params);

	/** Auto-layout material expression nodes. */
	static TSharedPtr<FJsonObject> HandleLayoutMaterialNodes(const TSharedPtr<FJsonObject>& Params);

	/** Set the position of one or more material expression nodes. */
	static TSharedPtr<FJsonObject> HandleSetMaterialNodePosition(const TSharedPtr<FJsonObject>& Params);

	/** Link a NamedRerouteUsage node to a NamedRerouteDeclaration node. */
	static TSharedPtr<FJsonObject> HandleLinkNamedRerouteUsage(const TSharedPtr<FJsonObject>& Params);

	/** Get complete material hierarchy with all node connections. */
	static TSharedPtr<FJsonObject> HandleGetMaterialHierarchy(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Material Function Commands
	//-------------------------------------------------------------------------

	/** Create a new UMaterialFunction asset. */
	static TSharedPtr<FJsonObject> HandleCreateMaterialFunction(const TSharedPtr<FJsonObject>& Params);

	/** Add a FunctionInput expression to a material function. */
	static TSharedPtr<FJsonObject> HandleAddFunctionInput(const TSharedPtr<FJsonObject>& Params);

	/** Add a FunctionOutput expression to a material function. */
	static TSharedPtr<FJsonObject> HandleAddFunctionOutput(const TSharedPtr<FJsonObject>& Params);

	/** Add an expression node inside a material function. */
	static TSharedPtr<FJsonObject> HandleAddFunctionNode(const TSharedPtr<FJsonObject>& Params);

	/** Connect nodes inside a material function. */
	static TSharedPtr<FJsonObject> HandleConnectFunctionNodes(const TSharedPtr<FJsonObject>& Params);

	/** Add a MaterialFunctionCall node to a material referencing an existing function. */
	static TSharedPtr<FJsonObject> HandleAddMaterialFunctionCall(const TSharedPtr<FJsonObject>& Params);

	/** Inspect a material function's inputs, outputs, and internal nodes. */
	static TSharedPtr<FJsonObject> HandleGetMaterialFunctionInfo(const TSharedPtr<FJsonObject>& Params);

	/** Execute multiple material operations in a single batch call. */
	static TSharedPtr<FJsonObject> HandleExecuteMaterialBatch(const TSharedPtr<FJsonObject>& Params);

	/** Execute multiple material function operations in a single batch call. */
	static TSharedPtr<FJsonObject> HandleExecuteFunctionBatch(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Shared Helpers
	//-------------------------------------------------------------------------

	/** Helper to load a material by name or path. */
	static UMaterial* LoadMaterial(const FString& MaterialName, FString& OutPath);

	/** Helper to load a material instance by name or path. */
	static UMaterialInstanceConstant* LoadMaterialInstance(const FString& InstanceName, FString& OutPath);

	/** Helper to load a material function by name or path. */
	static UMaterialFunction* LoadMaterialFunction(const FString& FunctionName, FString& OutPath);

	/** Helper to find an expression by name in a material. */
	static UMaterialExpression* FindExpressionByName(UMaterial* Material, const FString& NodeName);

	/** Helper to find an expression by name in a material function. */
	static UMaterialExpression* FindExpressionInFunction(UMaterialFunction* Function, const FString& NodeName);

	//-------------------------------------------------------------------------
	// String-to-Enum Converters
	//-------------------------------------------------------------------------

	static EMaterialDomain StringToMaterialDomain(const FString& DomainStr);
	static EBlendMode StringToBlendMode(const FString& BlendModeStr);
	static EMaterialShadingModel StringToShadingModel(const FString& ShadingModelStr);
	static EMaterialProperty StringToMaterialProperty(const FString& PropertyStr);
	static ETranslucencyLightingMode StringToTranslucencyLightingMode(const FString& ModeStr);
	static ERefractionMode StringToRefractionMode(const FString& ModeStr);
	static EDecalBlendMode StringToDecalBlendMode(const FString& ModeStr);
	static EBlendableLocation StringToBlendableLocation(const FString& LocationStr);
	static EMaterialStencilCompare StringToStencilCompare(const FString& CompareStr);
	static EMaterialTranslucencyPass StringToTranslucencyPass(const FString& PassStr);
	static EFunctionInputType StringToFunctionInputType(const FString& TypeStr);

	/** Helper to get expression class from type string. */
	static UClass* GetExpressionClassFromType(const FString& NodeType);

	/** Apply extended properties from JSON to a material (shared by create and set_properties). */
	static void ApplyExtendedProperties(UMaterial* Material, const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Composite Commands (non-static: capture this for lambdas)
	//-------------------------------------------------------------------------

	/**
	 * Build a complete material in one call: create (or clear) the asset, add all nodes,
	 * wire connections, connect outputs, and recompile.
	 */
	TSharedPtr<FJsonObject> HandleBuildMaterial(const TSharedPtr<FJsonObject>& Params);

	/** Render and return a base64-encoded PNG thumbnail of a material. */
	TSharedPtr<FJsonObject> HandleGetMaterialPreview(const TSharedPtr<FJsonObject>& Params);

	/** Full material introspection — settings, nodes, connections, custom code, function calls. */
	TSharedPtr<FJsonObject> HandleGetMaterialInfo(const TSharedPtr<FJsonObject>& Params);
};
