#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class FMCPCommandRegistry;

/**
 * Handler class for Blueprint search and discovery MCP commands.
 * Provides action search, class function listing, node type info,
 * category browsing, and placing searched actions into graphs.
 */
class UNREALMCP_API FUnrealMCPBlueprintSearchCommands
{
public:
	FUnrealMCPBlueprintSearchCommands() = default;

	/**
	 * Handle blueprint search commands.
	 * @param CommandType - The type of command to handle
	 * @param Params - JSON parameters for the command
	 * @return JSON response with results or error
	 */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all blueprint search commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

private:
	/** Search the Blueprint action database by keyword with pagination. */
	TSharedPtr<FJsonObject> HandleSearchBlueprintActions(const TSharedPtr<FJsonObject>& Params);

	/** List all BlueprintCallable/BlueprintPure functions on a UClass. */
	TSharedPtr<FJsonObject> HandleGetClassFunctions(const TSharedPtr<FJsonObject>& Params);

	/** Get information about a specific node type including its default pins. */
	TSharedPtr<FJsonObject> HandleGetNodeTypeInfo(const TSharedPtr<FJsonObject>& Params);

	/** Browse or filter actions by category. */
	TSharedPtr<FJsonObject> HandleSearchByCategory(const TSharedPtr<FJsonObject>& Params);

	/** Place a node from search results into a Blueprint graph. */
	TSharedPtr<FJsonObject> HandlePlaceSearchedAction(const TSharedPtr<FJsonObject>& Params);

	/** Search for an action by keyword and place the best match in one step. */
	TSharedPtr<FJsonObject> HandleSearchAndPlaceAction(const TSharedPtr<FJsonObject>& Params);
};
