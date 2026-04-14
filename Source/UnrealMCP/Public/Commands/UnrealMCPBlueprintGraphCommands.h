#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class FMCPCommandRegistry;

/**
 * Handler class for Blueprint Graph management MCP commands.
 * Supports creating/deleting function graphs, macro graphs, and auto-layout.
 */
class UNREALMCP_API FUnrealMCPBlueprintGraphCommands
{
public:
	FUnrealMCPBlueprintGraphCommands() = default;

	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all blueprint graph commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

private:
	TSharedPtr<FJsonObject> HandleCreateFunctionGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteFunctionGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleCreateMacroGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteMacroGraph(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAutoLayoutGraph(const TSharedPtr<FJsonObject>& Params);

	// Sprint 6: Function-level operations
	TSharedPtr<FJsonObject> HandleSetFunctionAccessSpecifier(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetFunctionFlags(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleAddLocalVariable(const TSharedPtr<FJsonObject>& Params);
};
