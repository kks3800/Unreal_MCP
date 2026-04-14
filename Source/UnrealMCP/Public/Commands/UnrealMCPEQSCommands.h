// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class FMCPCommandRegistry;
class UEnvQuery;
class UEnvQueryOption;

/**
 * Environment Query System (EQS) command handlers for Unreal MCP.
 * Provides full EQS asset management: create, delete, list, inspect,
 * add/remove generators, add/remove tests, configure properties.
 *
 * EDITOR ONLY - Requires AIModule.
 *
 * Command Categories:
 * - Asset Management: create, delete, list, info, save, open
 * - Option/Generator: add generator (creates option), remove option, list options
 * - Tests: add test to option, remove test, list tests
 * - Configuration: set generator/test properties
 */
class UNREALMCP_API FUnrealMCPEQSCommands
{
public:
	FUnrealMCPEQSCommands() = default;

	/** Handle EQS-related commands. */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all EQS commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

	//=========================================================================
	// Asset Management
	//=========================================================================

	TSharedPtr<FJsonObject> HandleCreateEQSQuery(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDeleteEQSQuery(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleListEQSQueries(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetEQSQueryInfo(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSaveEQSQuery(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleOpenEQSQuery(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Option / Generator Management
	//=========================================================================

	TSharedPtr<FJsonObject> HandleAddEQSGenerator(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleRemoveEQSOption(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetEQSOptions(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetEQSGeneratorProperty(const TSharedPtr<FJsonObject>& Params);

	//=========================================================================
	// Test Management
	//=========================================================================

	TSharedPtr<FJsonObject> HandleAddEQSTest(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleRemoveEQSTest(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetEQSTests(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSetEQSTestProperty(const TSharedPtr<FJsonObject>& Params);

private:
	//-------------------------------------------------------------------------
	// Helper Methods
	//-------------------------------------------------------------------------

	/** Load an EQS query asset by name or path. */
	UEnvQuery* LoadEQSQuery(const FString& QueryName, FString& OutPath);

	/** Resolve a generator class from a string name. */
	UClass* ResolveGeneratorClass(const FString& GeneratorType);

	/** Resolve a test class from a string name. */
	UClass* ResolveTestClass(const FString& TestType);

	/** Resolve a context class from a string name. */
	UClass* ResolveContextClass(const FString& ContextType);

	/** Serialize an option to JSON. */
	TSharedPtr<FJsonObject> OptionToJson(UEnvQueryOption* Option, int32 Index);

	static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);
	static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& ErrorMessage);

	//-------------------------------------------------------------------------
	// State
	//-------------------------------------------------------------------------

	TMap<FString, TWeakObjectPtr<UEnvQuery>> ActiveQueries;
};
