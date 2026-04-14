#pragma once

#include "CoreMinimal.h"
#include "Json.h"

class FMCPCommandRegistry;

/**
 * Handler class for Asset management MCP commands.
 *
 * Provides 17 commands across 4 groups:
 *   A) Discovery  - list_assets, get_asset_info, find_asset_references,
 *                   get_folder_structure, get_asset_class
 *   B) Organization - rename_asset, move_asset, move_directory, make_directory,
 *                     duplicate_asset, save_asset, save_directory
 *   C) Safety     - validate_asset_move, fix_redirectors, is_protected_path
 *   D) Analysis   - analyze_folder_organization, batch_move_assets
 *
 * Protected paths are passed per-command via a "protected_paths" JSON array
 * so callers decide which marketplace / third-party folders to guard.
 */
class UNREALMCP_API FUnrealMCPAssetCommands
{
public:
	FUnrealMCPAssetCommands();

	/** Route an incoming command string to the correct handler. */
	TSharedPtr<FJsonObject> HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params);

	/** Register all asset commands with the command registry. */
	void RegisterCommands(FMCPCommandRegistry& Registry);

private:
	//-------------------------------------------------------------------------
	// Group A: Discovery (read-only)
	//-------------------------------------------------------------------------

	TSharedPtr<FJsonObject> HandleListAssets(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetAssetInfo(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleFindAssetReferences(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetFolderStructure(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleGetAssetClass(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Group B: Organization (write)
	//-------------------------------------------------------------------------

	TSharedPtr<FJsonObject> HandleRenameAsset(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleMoveAsset(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleMoveDirectory(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleMakeDirectory(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleDuplicateAsset(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSaveAsset(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleSaveDirectory(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Group C: Safety
	//-------------------------------------------------------------------------

	TSharedPtr<FJsonObject> HandleValidateAssetMove(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleFixRedirectors(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleIsProtectedPath(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Group D: Analysis / Batch
	//-------------------------------------------------------------------------

	TSharedPtr<FJsonObject> HandleAnalyzeFolderOrganization(const TSharedPtr<FJsonObject>& Params);
	TSharedPtr<FJsonObject> HandleBatchMoveAssets(const TSharedPtr<FJsonObject>& Params);

	//-------------------------------------------------------------------------
	// Helpers
	//-------------------------------------------------------------------------

	/** Check if a path starts with any entry in the protected list. */
	bool IsProtectedPath(const FString& Path, const TArray<FString>& ProtectedPaths) const;

	/** Extract the protected_paths JSON array into a TArray. */
	TArray<FString> GetProtectedPaths(const TSharedPtr<FJsonObject>& Params) const;

	/** Build a recursive folder tree as nested JSON. Cycle-safe via VisitedPaths; hard-capped at MAX_SAFE_FOLDER_DEPTH. */
	TSharedPtr<FJsonObject> BuildFolderTree(const FString& DirectoryPath, int32 CurrentDepth, int32 MaxDepth, bool bIncludeAssetCounts, TSet<FString>& VisitedPaths) const;

	/** Check if an asset name follows the expected prefix convention. */
	bool CheckNamingConvention(const FString& AssetName, const FString& ClassName, FString& OutExpectedPrefix) const;

	/** Suggest the canonical subfolder for a given asset class. */
	FString GetSuggestedFolderForClass(const FString& ClassName) const;
};
