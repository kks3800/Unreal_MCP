#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class FMCPCommandRegistry;

/**
 * Handler class for Phase 8 Blueprint multi-graph authoring commands.
 *
 * Adds full-surface Blueprint authoring beyond the EventGraph:
 *  - Full-spec function/macro creation (flags, const, replication, macro tunnels)
 *  - Event dispatcher authoring (spec-named)
 *  - Interface implementation
 *  - Full-type local variables (objects, classes, interfaces, structs)
 *  - Bind/Call/Remove delegate node spawning
 *
 * These commands use an extended type-resolution helper that supports object,
 * class, interface, and user-struct pin types in addition to the primitives
 * already handled by the older BlueprintGraphCommands handler.
 */
class UNREALMCP_API FUnrealMCPBlueprintMultigraphCommands
{
public:
	FUnrealMCPBlueprintMultigraphCommands() = default;

	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all multi-graph commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

private:
	// Function / Macro authoring
	TSharedPtr<FJsonObject> HandleCreateFunctionGraphEx(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleCreateMacroGraphEx(const TSharedPtr<FJsonObject>& Params);

	// Event dispatchers
	TSharedPtr<FJsonObject> HandleCreateEventDispatcher(const TSharedPtr<FJsonObject>& Params);

	// Interface implementation
	TSharedPtr<FJsonObject> HandleImplementInterface(const TSharedPtr<FJsonObject>& Params);

	// Local variables with full type support
	TSharedPtr<FJsonObject> HandleAddLocalVariableEx(const TSharedPtr<FJsonObject>& Params);

	// Delegate node spawning
	TSharedPtr<FJsonObject> HandleAddBindDelegateNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddCallDelegateNode(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddRemoveDelegateNode(const TSharedPtr<FJsonObject>& Params);
};
