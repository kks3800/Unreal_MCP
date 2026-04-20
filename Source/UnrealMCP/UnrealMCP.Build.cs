// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;
using System.Text.RegularExpressions;

public class UnrealMCP : ModuleRules
{
	// Detect engine minor version from Build.version JSON
	private int GetEngineMinorVersion()
	{
		string VersionFile = Path.Combine(EngineDirectory, "Build", "Build.version");
		if (File.Exists(VersionFile))
		{
			string Content = File.ReadAllText(VersionFile);
			Match M = Regex.Match(Content, "\"MinorVersion\"\\s*:\\s*(\\d+)");
			if (M.Success) return int.Parse(M.Groups[1].Value);
		}
		return 0;
	}

	public UnrealMCP(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		int MinorVer = GetEngineMinorVersion();

		// IWYU: bEnforceIWYU (5.3) vs IWYUSupport (5.5+) — skip to avoid compat issues

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
				"MovieScene",
				"MovieSceneTracks"
			}
		);

		if (Target.bBuildEditor)
		{
			// Always-available editor modules
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"PropertyEditor",
					"ToolMenus",
					"BlueprintEditorLibrary",
					"UMGEditor",
					"MaterialEditor",
					"AssetTools",
					"ImageWrapper",
					// AI runtime (available in all UE5 versions)
					"AIModule",
					"GameplayTasks",
					"GameplayTags",
				}
			);

			// MetaSound modules
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"MetasoundEngine",
					"MetasoundFrontend",
					"MetasoundEditor",
					"MetasoundGraphCore",
				}
			);

			// Niagara modules
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Niagara",
					"NiagaraCore",
					"NiagaraEditor",
				}
			);

			// PCG modules (available in 5.3+ but experimental)
			PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"PCG",
					"PCGEditor",
				}
			);

			// BT editor graph nodes — symbols not exported before 5.4
			if (MinorVer >= 4)
			{
				PrivateDependencyModuleNames.AddRange(
					new string[]
					{
						"BehaviorTreeEditor",
						"AIGraph",
					}
				);
			}
		}
	}
}
