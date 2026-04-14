// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class UnrealMCP : ModuleRules
{
	public UnrealMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		// Use IWYUSupport instead of the deprecated bEnforceIWYU in UE5.5
		IWYUSupport = IWYUSupport.Full;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
		);
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"Networking",
				"Sockets",
				"HTTP",
				"Json",
				"JsonUtilities",
				"DeveloperSettings"
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"EditorScriptingUtilities",
				"EditorSubsystem",
				"Slate",
				"SlateCore",
				"UMG",
				"Kismet",
				"KismetCompiler",
				"BlueprintGraph",
				"Projects",
				"AssetRegistry",
				"CommonUI",
				// MovieScene modules for widget animations
				"MovieScene",
				"MovieSceneTracks"
			}
		);
		
		if (Target.bBuildEditor == true)
		{
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"PropertyEditor",      // For widget property editing
					"ToolMenus",           // For editor UI
					"BlueprintEditorLibrary", // For Blueprint utilities
					"UMGEditor",          // For WidgetBlueprint.h and other UMG editor functionality
					"MaterialEditor",      // For UMaterialEditingLibrary and material graph editing
					// MetaSound modules for audio graph creation
					"MetasoundEngine",     // Core MetaSound runtime and builder subsystem
					"MetasoundFrontend",   // Document/graph structures and node registry
					"MetasoundEditor",     // Editor utilities and factory classes
					"MetasoundGraphCore", // Graph manipulation and node types
					// Niagara modules for particle system creation
					"Niagara",             // Core Niagara runtime (systems, emitters, renderers)
					"NiagaraCore",         // Core types and utilities
					"NiagaraEditor",       // Editor utilities and factory classes
					// Asset management
					"AssetTools",          // IAssetTools for redirector fixup
				// AI / Behavior Tree modules
				"AIModule",            // Core BT/BB runtime (UBehaviorTree, UBlackboardData)
				"GameplayTasks",       // Required by AIModule task nodes
				"GameplayTags",        // FGameplayTag for TagCooldown decorator
				"BehaviorTreeEditor",  // BT graph nodes, factories
				"AIGraph",             // AIGraphNode, AIGraphSchema
				// Screenshot and image encoding
				"ImageWrapper",        // IImageWrapper for editor screenshots
				// ThumbnailTools is part of UnrealEd (already included above)
				// Procedural Content Generation modules
				"PCG",                 // PCG runtime (graphs, nodes, settings)
				"PCGEditor",           // PCG editor (factory, asset utilities)
				}
			);
		}
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
} 