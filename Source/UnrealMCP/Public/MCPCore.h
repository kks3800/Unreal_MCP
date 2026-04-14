// Copyright UnrealMCP Contributors. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

class UPCGGraph;
class UPCGNode;

/**
 * Command Registry for MCP UMG Tools
 * Replaces giant if-else chain with clean registration pattern
 */
class UNREALMCP_API FMCPCommandRegistry
{
public:
	using FCommandHandler = TFunction<TSharedPtr<FJsonObject>(const TSharedPtr<FJsonObject>&)>;

	/** Register a command handler */
	void RegisterCommand(const FString& CommandName, FCommandHandler Handler);

	/** Execute a command by name */
	TSharedPtr<FJsonObject> ExecuteCommand(const FString& CommandName, const TSharedPtr<FJsonObject>& Params);

	/** Check if a command exists */
	bool HasCommand(const FString& CommandName) const;

	/** Get list of all registered commands */
	TArray<FString> GetRegisteredCommands() const;

	/** Singleton access */
	static FMCPCommandRegistry& Get();

private:
	TMap<FString, FCommandHandler> CommandHandlers;
	static TUniquePtr<FMCPCommandRegistry> Instance;
};

/**
 * Widget Edit Context for deferred compilation
 * Allows batch operations without compiling after each one
 */
class UNREALMCP_API FMCPWidgetContext
{
public:
	/** Begin editing a widget - defers compilation */
	bool BeginEdit(const FString& WidgetName);

	/** End editing - compiles and saves */
	bool EndEdit();

	/** Check if we're in batch edit mode */
	bool IsEditing() const { return ActiveBlueprint != nullptr; }

	/** Get the active widget blueprint */
	class UWidgetBlueprint* GetActiveBlueprint() const { return ActiveBlueprint; }

	/** Get the blueprint path for saving */
	const FString& GetBlueprintPath() const { return BlueprintPath; }

	/** Singleton access */
	static FMCPWidgetContext& Get();

	/** Mark that changes were made (for deferred compile) */
	void MarkDirty() { bIsDirty = true; }

private:
	class UWidgetBlueprint* ActiveBlueprint = nullptr;
	FString BlueprintPath;
	bool bIsDirty = false;
	static TUniquePtr<FMCPWidgetContext> Instance;
};

/**
 * Blueprint Edit Context for deferred compilation.
 * Allows batch Blueprint operations without compiling after each one.
 * Mirrors FMCPWidgetContext but for standard Blueprints.
 */
class UNREALMCP_API FMCPBlueprintContext
{
public:
	/** Begin editing a Blueprint - defers compilation until EndEdit. */
	bool BeginEdit(const FString& BlueprintName);

	/**
	 * End editing - single MarkBlueprintAsStructurallyModified + optional compile/save/layout.
	 * @param bAutoCompile If true, compile the Blueprint after all edits.
	 * @param bAutoSave If true, save the Blueprint asset.
	 * @param AutoLayoutGraph If non-empty, auto-layout this graph name.
	 */
	bool EndEdit(bool bAutoCompile = true, bool bAutoSave = false, const FString& AutoLayoutGraph = FString());

	/** Check if we're currently in batch edit mode. */
	bool IsEditing() const { return ActiveBlueprint != nullptr; }

	/** Get the active Blueprint being edited. */
	class UBlueprint* GetBlueprint() const { return ActiveBlueprint; }

	/** Get the asset path for saving. */
	const FString& GetBlueprintPath() const { return BlueprintPath; }

	/** Mark that changes were made (for deferred compile). */
	void MarkDirty() { bIsDirty = true; }

	/** Log an operation result for batch reporting. */
	void LogOperation(const FString& OpName, bool bSuccess);

	/** Get operation log. */
	const TArray<FString>& GetOperationLog() const { return OperationLog; }

	/** Singleton access. */
	static FMCPBlueprintContext& Get();

private:
	class UBlueprint* ActiveBlueprint = nullptr;
	FString BlueprintPath;
	bool bIsDirty = false;
	TArray<FString> OperationLog;
	static TUniquePtr<FMCPBlueprintContext> Instance;
};

/**
 * PCG Graph Edit Context for batched graph mutations.
 * Allows batch PCG operations without per-edit editor notifications.
 * Mirrors FMCPBlueprintContext / FMCPWidgetContext but for PCG graphs.
 *
 * BeginEdit calls DisableNotificationsForEditor so downstream node/edge
 * mutations do not spam the editor. EndEdit re-enables notifications and
 * forces a single structural notification, optionally saving the asset.
 *
 * Also owns a stable node-id <-> UPCGNode* map for the active session so
 * MCP clients can address nodes by a string id instead of a pointer or the
 * node's internal name (which can change on rename).
 */
class UNREALMCP_API FMCPPCGContext
{
public:
	/** Begin editing a PCG graph - defers editor notifications until EndEdit. */
	bool BeginEdit(const FString& GraphPath);

	/**
	 * End editing - re-enables editor notifications, fires a single structural
	 * change notification, and optionally saves the asset.
	 * @param bAutoLayout If true, apply auto-layout to the graph (no-op until Task 11).
	 * @param bSave If true, save the graph asset to its package.
	 */
	bool EndEdit(bool bAutoLayout, bool bSave);

	/** Check if we're currently in a batch edit session. */
	bool IsEditing() const { return ActiveGraph.IsValid(); }

	/**
	 * Returns true if an edit session is active AND the given graph path refers
	 * to the active graph. Tolerant of both canonical (/Game/Foo/Bar.Bar) and
	 * package-only (/Game/Foo/Bar) path forms by normalizing the input before
	 * comparison against the stored (package-form) ActiveGraphPath.
	 */
	bool IsEditingGraph(const FString& GraphPath) const;

	/** Get the active PCG graph being edited. */
	UPCGGraph* GetActiveGraph() const { return ActiveGraph.Get(); }

	/** Get the asset path of the graph currently being edited. */
	const FString& GetActiveGraphPath() const { return ActiveGraphPath; }

	/** Register a node in the session-local id map. The caller owns id generation. */
	void RegisterNode(const FString& Id, UPCGNode* Node);

	/** Remove a node registration (used after deleting the node from the graph). */
	void UnregisterNode(const FString& Id);

	/**
	 * Look up a node by its session-local id. Returns nullptr if not found.
	 * Reserved ids "$input" and "$output" resolve to Graph->GetInputNode() /
	 * GetOutputNode() so downstream edge commands can reference the auto-created
	 * IO nodes uniformly without special-casing.
	 */
	UPCGNode* FindNode(const FString& Id) const;

	/** Look up the session-local id for a node. Returns empty string if not registered. */
	FString GetNodeId(UPCGNode* Node) const;

	/**
	 * Generate a stable, session-local id for a freshly-created node of the given
	 * short type (e.g. "SurfaceSampler" -> "SurfaceSampler_0", "SurfaceSampler_1").
	 * Counter is per-type and resets when the session ends.
	 */
	FString GenerateNodeId(const FString& TypeShortName);

	/** Singleton access. */
	static FMCPPCGContext& Get();

private:
	// TWeakObjectPtr so a GC of the graph during a session does not dangle the context.
	TWeakObjectPtr<UPCGGraph> ActiveGraph;
	FString ActiveGraphPath;
	TMap<FString, TWeakObjectPtr<UPCGNode>> IdToNode;
	TMap<TWeakObjectPtr<UPCGNode>, FString> NodeToId;
	// Per-type monotonic counters for GenerateNodeId; reset each BeginEdit/EndEdit cycle.
	TMap<FString, int32> NodeTypeCounters;
	static TUniquePtr<FMCPPCGContext> Instance;
};

/**
 * Common UI Configuration
 * Maps widget type names to Widget Blueprint paths
 */
class UNREALMCP_API FMCPCommonUIConfig
{
public:
	/** Set the path for a Common UI widget type */
	void SetWidgetPath(const FString& TypeName, const FString& BlueprintPath);

	/** Get the blueprint path for a widget type */
	FString GetWidgetPath(const FString& TypeName) const;

	/** Check if a type has a mapping */
	bool HasWidgetType(const FString& TypeName) const;

	/** Get all configured types */
	TArray<FString> GetConfiguredTypes() const;

	/** Initialize with default Common UI paths */
	void InitializeDefaults();

	/** Singleton access */
	static FMCPCommonUIConfig& Get();

private:
	TMap<FString, FString> WidgetPaths;
	static TUniquePtr<FMCPCommonUIConfig> Instance;
};

/**
 * Style Presets Manager
 * Stores reusable style definitions for quick application
 */
class UNREALMCP_API FMCPStylePresets
{
public:
	/** Create a style preset */
	void CreatePreset(const FString& PresetName, const TSharedPtr<FJsonObject>& StyleData);

	/** Get a style preset */
	TSharedPtr<FJsonObject> GetPreset(const FString& PresetName) const;

	/** Check if preset exists */
	bool HasPreset(const FString& PresetName) const;

	/** Get all preset names */
	TArray<FString> GetPresetNames() const;

	/** Delete a preset */
	void DeletePreset(const FString& PresetName);

	/** Singleton access */
	static FMCPStylePresets& Get();

private:
	TMap<FString, TSharedPtr<FJsonObject>> Presets;
	static TUniquePtr<FMCPStylePresets> Instance;
};
