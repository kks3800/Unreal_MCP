// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPNiagaraCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"

// Niagara includes
#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraScript.h"
#include "NiagaraRendererProperties.h"
#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"
#include "NiagaraLightRendererProperties.h"
#include "NiagaraUserRedirectionParameterStore.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeOutput.h"
#include "NiagaraSystemFactoryNew.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// Sprint 1: Asset Management
	if (CommandType == TEXT("create_niagara_system"))
	{
		return HandleCreateNiagaraSystem(Params);
	}
	else if (CommandType == TEXT("delete_niagara_system"))
	{
		return HandleDeleteNiagaraSystem(Params);
	}
	else if (CommandType == TEXT("get_niagara_system_info"))
	{
		return HandleGetNiagaraSystemInfo(Params);
	}
	else if (CommandType == TEXT("list_niagara_systems"))
	{
		return HandleListNiagaraSystems(Params);
	}
	else if (CommandType == TEXT("open_niagara_system"))
	{
		return HandleOpenNiagaraSystem(Params);
	}
	else if (CommandType == TEXT("compile_niagara_system"))
	{
		return HandleCompileNiagaraSystem(Params);
	}
	else if (CommandType == TEXT("save_niagara_system"))
	{
		return HandleSaveNiagaraSystem(Params);
	}
	else if (CommandType == TEXT("set_niagara_system_properties"))
	{
		return HandleSetNiagaraSystemProperties(Params);
	}
	// Sprint 2: Emitter Management
	else if (CommandType == TEXT("add_emitter_to_system"))
	{
		return HandleAddEmitterToSystem(Params);
	}
	else if (CommandType == TEXT("remove_emitter_from_system"))
	{
		return HandleRemoveEmitterFromSystem(Params);
	}
	else if (CommandType == TEXT("duplicate_emitter"))
	{
		return HandleDuplicateEmitter(Params);
	}
	else if (CommandType == TEXT("enable_emitter"))
	{
		return HandleEnableEmitter(Params);
	}
	else if (CommandType == TEXT("disable_emitter"))
	{
		return HandleDisableEmitter(Params);
	}
	else if (CommandType == TEXT("rename_emitter"))
	{
		return HandleRenameEmitter(Params);
	}
	else if (CommandType == TEXT("get_emitter_info"))
	{
		return HandleGetEmitterInfo(Params);
	}
	else if (CommandType == TEXT("list_emitters"))
	{
		return HandleListEmitters(Params);
	}
	else if (CommandType == TEXT("set_emitter_mode"))
	{
		return HandleSetEmitterMode(Params);
	}
	else if (CommandType == TEXT("isolate_emitter"))
	{
		return HandleIsolateEmitter(Params);
	}
	// Sprint 3: Renderer Configuration
	else if (CommandType == TEXT("add_sprite_renderer"))
	{
		return HandleAddSpriteRenderer(Params);
	}
	else if (CommandType == TEXT("configure_sprite_renderer"))
	{
		return HandleConfigureSpriteRenderer(Params);
	}
	else if (CommandType == TEXT("add_mesh_renderer"))
	{
		return HandleAddMeshRenderer(Params);
	}
	else if (CommandType == TEXT("configure_mesh_renderer"))
	{
		return HandleConfigureMeshRenderer(Params);
	}
	else if (CommandType == TEXT("add_ribbon_renderer"))
	{
		return HandleAddRibbonRenderer(Params);
	}
	else if (CommandType == TEXT("configure_ribbon_renderer"))
	{
		return HandleConfigureRibbonRenderer(Params);
	}
	else if (CommandType == TEXT("add_light_renderer"))
	{
		return HandleAddLightRenderer(Params);
	}
	else if (CommandType == TEXT("configure_light_renderer"))
	{
		return HandleConfigureLightRenderer(Params);
	}
	else if (CommandType == TEXT("remove_renderer"))
	{
		return HandleRemoveRenderer(Params);
	}
	else if (CommandType == TEXT("get_renderers"))
	{
		return HandleGetRenderers(Params);
	}
	else if (CommandType == TEXT("set_renderer_material"))
	{
		return HandleSetRendererMaterial(Params);
	}
	else if (CommandType == TEXT("set_renderer_visibility"))
	{
		return HandleSetRendererVisibility(Params);
	}
	else if (CommandType == TEXT("set_renderer_sort_mode"))
	{
		return HandleSetRendererSortMode(Params);
	}
	else if (CommandType == TEXT("set_renderer_bindings"))
	{
		return HandleSetRendererBindings(Params);
	}
	// Sprint 4: Module Operations
	else if (CommandType == TEXT("add_spawn_module"))
	{
		return HandleAddSpawnModule(Params);
	}
	else if (CommandType == TEXT("add_update_module"))
	{
		return HandleAddUpdateModule(Params);
	}
	else if (CommandType == TEXT("add_particle_spawn_module"))
	{
		return HandleAddParticleSpawnModule(Params);
	}
	else if (CommandType == TEXT("add_particle_update_module"))
	{
		return HandleAddParticleUpdateModule(Params);
	}
	else if (CommandType == TEXT("remove_module"))
	{
		return HandleRemoveModule(Params);
	}
	else if (CommandType == TEXT("get_modules"))
	{
		return HandleGetModules(Params);
	}
	else if (CommandType == TEXT("set_module_enabled"))
	{
		return HandleSetModuleEnabled(Params);
	}
	else if (CommandType == TEXT("list_available_modules"))
	{
		return HandleListAvailableModules(Params);
	}
	else if (CommandType == TEXT("configure_module_input"))
	{
		return HandleConfigureModuleInput(Params);
	}
	else if (CommandType == TEXT("get_module_inputs"))
	{
		return HandleGetModuleInputs(Params);
	}
	else if (CommandType == TEXT("reorder_modules"))
	{
		return HandleReorderModules(Params);
	}
	else if (CommandType == TEXT("add_event_handler"))
	{
		return HandleAddEventHandler(Params);
	}
	// Sprint 5: Parameter System
	else if (CommandType == TEXT("expose_user_parameter"))
	{
		return HandleExposeUserParameter(Params);
	}
	else if (CommandType == TEXT("set_user_parameter_default"))
	{
		return HandleSetUserParameterDefault(Params);
	}
	else if (CommandType == TEXT("get_user_parameters"))
	{
		return HandleGetUserParameters(Params);
	}
	else if (CommandType == TEXT("remove_user_parameter"))
	{
		return HandleRemoveUserParameter(Params);
	}
	else if (CommandType == TEXT("bind_parameter"))
	{
		return HandleBindParameter(Params);
	}
	else if (CommandType == TEXT("set_emitter_parameter"))
	{
		return HandleSetEmitterParameter(Params);
	}
	else if (CommandType == TEXT("set_particle_parameter"))
	{
		return HandleSetParticleParameter(Params);
	}
	else if (CommandType == TEXT("get_parameter_bindings"))
	{
		return HandleGetParameterBindings(Params);
	}
	else if (CommandType == TEXT("create_parameter_collection"))
	{
		return HandleCreateParameterCollection(Params);
	}
	else if (CommandType == TEXT("override_parameter_collection"))
	{
		return HandleOverrideParameterCollection(Params);
	}
	// Sprint 6: Material Integration
	else if (CommandType == TEXT("create_particle_material"))
	{
		return HandleCreateParticleMaterial(Params);
	}
	else if (CommandType == TEXT("assign_material_to_renderer"))
	{
		return HandleAssignMaterialToRenderer(Params);
	}
	else if (CommandType == TEXT("set_dynamic_material_binding"))
	{
		return HandleSetDynamicMaterialBinding(Params);
	}
	else if (CommandType == TEXT("create_particle_material_instance"))
	{
		return HandleCreateParticleMaterialInstance(Params);
	}
	else if (CommandType == TEXT("configure_material_parameters"))
	{
		return HandleConfigureMaterialParameters(Params);
	}
	else if (CommandType == TEXT("add_particle_color_node"))
	{
		return HandleAddParticleColorNode(Params);
	}

	return CreateErrorResponse(FString::Printf(TEXT("Unknown Niagara command: %s"), *CommandType));
}

//=============================================================================
// Helper Methods
//=============================================================================

UNiagaraSystem* FUnrealMCPNiagaraCommands::LoadNiagaraSystem(const FString& SystemName, FString& OutPath)
{
	// Check cache first
	if (TWeakObjectPtr<UNiagaraSystem>* CachedSystem = ActiveSystems.Find(SystemName))
	{
		if (CachedSystem->IsValid())
		{
			UNiagaraSystem* System = CachedSystem->Get();
			OutPath = System->GetPathName();
			return System;
		}
	}

	// Try direct path
	FString AssetPath = SystemName;
	if (!AssetPath.StartsWith(TEXT("/")))
	{
		AssetPath = FString::Printf(TEXT("/Game/FX/%s.%s"), *SystemName, *SystemName);
	}

	UNiagaraSystem* System = Cast<UNiagaraSystem>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!System)
	{
		// Try searching in the asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssetsByClass(UNiagaraSystem::StaticClass()->GetClassPathName(), AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			if (Asset.AssetName.ToString() == SystemName)
			{
				System = Cast<UNiagaraSystem>(Asset.GetAsset());
				if (System)
				{
					OutPath = Asset.GetObjectPathString();
					break;
				}
			}
		}
	}

	if (System)
	{
		OutPath = System->GetPathName();
		ActiveSystems.Add(SystemName, System);
	}

	return System;
}

FNiagaraEmitterHandle* FUnrealMCPNiagaraCommands::FindEmitterHandle(UNiagaraSystem* System, const FString& EmitterName)
{
	if (!System)
	{
		return nullptr;
	}

	TArray<FNiagaraEmitterHandle>& Handles = System->GetEmitterHandles();
	for (FNiagaraEmitterHandle& Handle : Handles)
	{
		if (Handle.GetName().ToString() == EmitterName)
		{
			return &Handle;
		}
	}

	return nullptr;
}

FGuid FUnrealMCPNiagaraCommands::GetEmitterVersion(const FNiagaraEmitterHandle& Handle)
{
	FVersionedNiagaraEmitter VersionedEmitter = Handle.GetInstance();
	return VersionedEmitter.Version;
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), true);
	if (Data.IsValid())
	{
		for (const auto& Pair : Data->Values)
		{
			Response->SetField(Pair.Key, Pair.Value);
		}
	}
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

//=============================================================================
// Sprint 1: Asset Management Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCreateNiagaraSystem(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game/FX");

	bool bCreateDefaultEmitter = Params->HasField(TEXT("create_default_emitter"))
		? Params->GetBoolField(TEXT("create_default_emitter"))
		: true;

	// Construct full path
	FString FullPath = FString::Printf(TEXT("%s/%s"), *Path, *SystemName);
	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *SystemName);

	// Create the package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package for Niagara System"));
	}

	// Create the Niagara System
	UNiagaraSystem* NewSystem = NewObject<UNiagaraSystem>(
		Package,
		UNiagaraSystem::StaticClass(),
		*SystemName,
		RF_Public | RF_Standalone | RF_Transactional
	);

	if (!NewSystem)
	{
		return CreateErrorResponse(TEXT("Failed to create Niagara System object"));
	}

	// Initialize the system using the factory helper
	UNiagaraSystemFactoryNew::InitializeSystem(NewSystem, bCreateDefaultEmitter);

	// Mark package dirty
	Package->MarkPackageDirty();

	// Save the asset
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewSystem, *PackageFileName, SaveArgs);

	// Notify asset registry
	FAssetRegistryModule::AssetCreated(NewSystem);

	// Cache the system
	ActiveSystems.Add(SystemName, NewSystem);

	// Prepare response
	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("system_name"), SystemName);
	Data->SetStringField(TEXT("path"), FullPath);
	Data->SetNumberField(TEXT("emitter_count"), NewSystem->GetNumEmitters());
	Data->SetStringField(TEXT("message"), TEXT("Niagara System created successfully"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleDeleteNiagaraSystem(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Remove from cache
	ActiveSystems.Remove(SystemName);

	// Delete the asset
	if (!UEditorAssetLibrary::DeleteAsset(SystemPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to delete Niagara System: %s"), *SystemName));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("deleted"), SystemName);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetNiagaraSystemInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("name"), System->GetName());
	Data->SetStringField(TEXT("path"), SystemPath);
	Data->SetNumberField(TEXT("emitter_count"), System->GetNumEmitters());

	// Get emitter names
	TArray<TSharedPtr<FJsonValue>> EmitterArray;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		TSharedPtr<FJsonObject> EmitterInfo = MakeShareable(new FJsonObject);
		EmitterInfo->SetStringField(TEXT("name"), Handle.GetName().ToString());
		EmitterInfo->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());
		EmitterInfo->SetStringField(TEXT("id"), Handle.GetId().ToString());
		EmitterArray.Add(MakeShareable(new FJsonValueObject(EmitterInfo)));
	}
	Data->SetArrayField(TEXT("emitters"), EmitterArray);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleListNiagaraSystems(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game");

	bool bRecursive = Params->HasField(TEXT("recursive"))
		? Params->GetBoolField(TEXT("recursive"))
		: true;

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UNiagaraSystem::StaticClass()->GetClassPathName(), AssetList);

	TArray<TSharedPtr<FJsonValue>> SystemArray;
	for (const FAssetData& Asset : AssetList)
	{
		FString AssetPath = Asset.GetObjectPathString();
		if (AssetPath.StartsWith(Path) || Path == TEXT("/Game"))
		{
			TSharedPtr<FJsonObject> SystemInfo = MakeShareable(new FJsonObject);
			SystemInfo->SetStringField(TEXT("name"), Asset.AssetName.ToString());
			SystemInfo->SetStringField(TEXT("path"), AssetPath);
			SystemArray.Add(MakeShareable(new FJsonValueObject(SystemInfo)));
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("systems"), SystemArray);
	Data->SetNumberField(TEXT("count"), SystemArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleOpenNiagaraSystem(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Cache the system for editing
	ActiveSystems.Add(SystemName, System);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("system_name"), SystemName);
	Data->SetStringField(TEXT("path"), SystemPath);
	Data->SetNumberField(TEXT("emitter_count"), System->GetNumEmitters());
	Data->SetStringField(TEXT("message"), TEXT("Niagara System opened for editing"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCompileNiagaraSystem(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Request compile
	System->RequestCompile(false);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("system_name"), SystemName);
	Data->SetStringField(TEXT("message"), TEXT("Compilation requested"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSaveNiagaraSystem(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Mark dirty and save
	UPackage* Package = System->GetOutermost();
	if (Package)
	{
		Package->MarkPackageDirty();

		FString PackageFileName = FPackageName::LongPackageNameToFilename(
			Package->GetName(),
			FPackageName::GetAssetPackageExtension()
		);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
		bool bSaved = UPackage::SavePackage(Package, System, *PackageFileName, SaveArgs);

		if (!bSaved)
		{
			return CreateErrorResponse(TEXT("Failed to save Niagara System package"));
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("system_name"), SystemName);
	Data->SetStringField(TEXT("path"), SystemPath);
	Data->SetStringField(TEXT("message"), TEXT("Niagara System saved"));

	return CreateSuccessResponse(Data);
}

// Placeholder - implementation moved to unified version at line ~2490

//=============================================================================
// Sprint 2: Emitter Management Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddEmitterToSystem(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Check if a template was specified
	FString TemplateName;
	UNiagaraEmitter* TemplateEmitter = nullptr;

	if (Params->TryGetStringField(TEXT("template_name"), TemplateName) && !TemplateName.IsEmpty())
	{
		// Try to load the template emitter
		FString TemplatePath = TemplateName;
		if (!TemplatePath.StartsWith(TEXT("/")))
		{
			TemplatePath = FString::Printf(TEXT("/Game/FX/%s.%s"), *TemplateName, *TemplateName);
		}

		TemplateEmitter = Cast<UNiagaraEmitter>(UEditorAssetLibrary::LoadAsset(TemplatePath));
	}

	// If no template specified, try to load the engine's simple sprite burst emitter
	if (!TemplateEmitter)
	{
		// Try several known engine emitter templates (UE 5.7 paths)
		static const TCHAR* DefaultEmitterPaths[] = {
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/SimpleSpriteBurst.SimpleSpriteBurst"),
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/OmnidirectionalBurst.OmnidirectionalBurst"),
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/Fountain.Fountain"),
			TEXT("/Niagara/DefaultAssets/Templates/Emitters/Minimal.Minimal")
		};

		for (const TCHAR* Path : DefaultEmitterPaths)
		{
			TemplateEmitter = Cast<UNiagaraEmitter>(StaticLoadObject(UNiagaraEmitter::StaticClass(), nullptr, Path));
			if (TemplateEmitter)
			{
				break;
			}
		}
	}

	if (!TemplateEmitter)
	{
		return CreateErrorResponse(TEXT("No template emitter specified and could not find engine default emitter. Please provide a template_name parameter."));
	}

	// Get the template's version for proper copying
	FGuid TemplateVersion = TemplateEmitter->GetExposedVersion().VersionGuid;

	// Use AddEmitterHandle which properly creates a copy with parent relationship
	System->Modify();
	FNiagaraEmitterHandle NewHandle = System->AddEmitterHandle(*TemplateEmitter, FName(*EmitterName), TemplateVersion);

	// Request recompile
	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("system_name"), SystemName);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("emitter_id"), NewHandle.GetId().ToString());
	Data->SetStringField(TEXT("message"), TEXT("Emitter added to system"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveEmitterFromSystem(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	// Remove the emitter
	System->RemoveEmitterHandle(*Handle);

	// Request recompile
	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("system_name"), SystemName);
	Data->SetStringField(TEXT("removed_emitter"), EmitterName);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleDuplicateEmitter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SourceEmitter;
	if (!Params->TryGetStringField(TEXT("source_emitter"), SourceEmitter) || SourceEmitter.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: source_emitter"));
	}

	FString NewName;
	if (!Params->TryGetStringField(TEXT("new_name"), NewName) || NewName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: new_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, SourceEmitter);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *SourceEmitter));
	}

	// Duplicate the emitter
	FNiagaraEmitterHandle DuplicatedHandle = System->DuplicateEmitterHandle(*Handle, FName(*NewName));

	// Request recompile
	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("system_name"), SystemName);
	Data->SetStringField(TEXT("new_emitter"), NewName);
	Data->SetStringField(TEXT("emitter_id"), DuplicatedHandle.GetId().ToString());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleEnableEmitter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	Handle->SetIsEnabled(true, *System, true);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetBoolField(TEXT("enabled"), true);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleDisableEmitter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	Handle->SetIsEnabled(false, *System, true);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetBoolField(TEXT("enabled"), false);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRenameEmitter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString NewName;
	if (!Params->TryGetStringField(TEXT("new_name"), NewName) || NewName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: new_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	Handle->SetName(FName(*NewName), *System);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("old_name"), EmitterName);
	Data->SetStringField(TEXT("new_name"), NewName);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetEmitterInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("name"), Handle->GetName().ToString());
	Data->SetStringField(TEXT("id"), Handle->GetId().ToString());
	Data->SetBoolField(TEXT("enabled"), Handle->GetIsEnabled());

	// Get emitter instance details
	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	if (UNiagaraEmitter* Emitter = VersionedEmitter.Emitter)
	{
		Data->SetStringField(TEXT("emitter_path"), Emitter->GetPathName());

		// Renderer count - accessed through versioned emitter data
		if (FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData())
		{
			const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
			Data->SetNumberField(TEXT("renderer_count"), Renderers.Num());
		}
	}

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleListEmitters(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	TArray<TSharedPtr<FJsonValue>> EmitterArray;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		TSharedPtr<FJsonObject> EmitterInfo = MakeShareable(new FJsonObject);
		EmitterInfo->SetStringField(TEXT("name"), Handle.GetName().ToString());
		EmitterInfo->SetStringField(TEXT("id"), Handle.GetId().ToString());
		EmitterInfo->SetBoolField(TEXT("enabled"), Handle.GetIsEnabled());
		EmitterArray.Add(MakeShareable(new FJsonValueObject(EmitterInfo)));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("emitters"), EmitterArray);
	Data->SetNumberField(TEXT("count"), EmitterArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetEmitterMode(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString Mode;
	if (!Params->TryGetStringField(TEXT("mode"), Mode) || Mode.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: mode"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	// Note: SimTarget is stored in FVersionedNiagaraEmitterData and requires editor-level APIs to modify
	// For now, this is a placeholder - full implementation requires NiagaraEditor subsystem access
	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("mode"), Mode);
	Data->SetStringField(TEXT("message"), TEXT("SetEmitterMode not yet fully implemented - requires Niagara Editor APIs"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleIsolateEmitter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	bool bIsolated = Params->HasField(TEXT("isolated"))
		? Params->GetBoolField(TEXT("isolated"))
		: true;

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Find the target emitter
	FNiagaraEmitterHandle* TargetHandle = FindEmitterHandle(System, EmitterName);
	if (!TargetHandle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	// Toggle isolation by enabling/disabling other emitters
	for (FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (bIsolated)
		{
			bool bIsTarget = (Handle.GetId() == TargetHandle->GetId());
			Handle.SetIsEnabled(bIsTarget, *System, true);
		}
		else
		{
			Handle.SetIsEnabled(true, *System, true);
		}
	}

	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetBoolField(TEXT("isolated"), bIsolated);

	return CreateSuccessResponse(Data);
}

//=============================================================================
// Sprint 3: Renderer Configuration Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddSpriteRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	// Create sprite renderer
	UNiagaraSpriteRendererProperties* SpriteRenderer = NewObject<UNiagaraSpriteRendererProperties>(
		Emitter,
		UNiagaraSpriteRendererProperties::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	if (!SpriteRenderer)
	{
		return CreateErrorResponse(TEXT("Failed to create sprite renderer"));
	}

	// Configure optional properties
	FString MaterialPath;
	if (Params->TryGetStringField(TEXT("material"), MaterialPath) && !MaterialPath.IsEmpty())
	{
		UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
		if (Material)
		{
			SpriteRenderer->Material = Material;
		}
	}

	FString Alignment;
	if (Params->TryGetStringField(TEXT("alignment"), Alignment))
	{
		if (Alignment.Equals(TEXT("Unaligned"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->Alignment = ENiagaraSpriteAlignment::Unaligned;
		}
		else if (Alignment.Equals(TEXT("VelocityAligned"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->Alignment = ENiagaraSpriteAlignment::VelocityAligned;
		}
		else if (Alignment.Equals(TEXT("CustomAlignment"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->Alignment = ENiagaraSpriteAlignment::CustomAlignment;
		}
	}

	FString FacingMode;
	if (Params->TryGetStringField(TEXT("facing_mode"), FacingMode))
	{
		if (FacingMode.Equals(TEXT("FaceCamera"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->FacingMode = ENiagaraSpriteFacingMode::FaceCamera;
		}
		else if (FacingMode.Equals(TEXT("FaceCameraPlane"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->FacingMode = ENiagaraSpriteFacingMode::FaceCameraPlane;
		}
		else if (FacingMode.Equals(TEXT("CustomFacingVector"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->FacingMode = ENiagaraSpriteFacingMode::CustomFacingVector;
		}
		else if (FacingMode.Equals(TEXT("FaceCameraPosition"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->FacingMode = ENiagaraSpriteFacingMode::FaceCameraPosition;
		}
		else if (FacingMode.Equals(TEXT("FaceCameraDistanceBlend"), ESearchCase::IgnoreCase))
		{
			SpriteRenderer->FacingMode = ENiagaraSpriteFacingMode::FaceCameraDistanceBlend;
		}
	}

	// Add renderer to emitter
	Emitter->AddRenderer(SpriteRenderer, VersionedEmitter.Version);

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("renderer_type"), TEXT("Sprite"));

	// Get renderer count through versioned emitter data
	if (FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData())
	{
		Data->SetNumberField(TEXT("renderer_index"), EmitterData->GetRenderers().Num() - 1);
	}

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleConfigureSpriteRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	int32 RendererIndex = Params->HasField(TEXT("renderer_index"))
		? static_cast<int32>(Params->GetNumberField(TEXT("renderer_index")))
		: 0;

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return CreateErrorResponse(TEXT("Invalid renderer index"));
	}

	UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderers[RendererIndex]);
	if (!SpriteRenderer)
	{
		return CreateErrorResponse(TEXT("Renderer at index is not a sprite renderer"));
	}

	// Configure properties based on provided params
	FString MaterialPath;
	if (Params->TryGetStringField(TEXT("material"), MaterialPath) && !MaterialPath.IsEmpty())
	{
		UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
		if (Material)
		{
			SpriteRenderer->Material = Material;
		}
	}

	if (Params->HasField(TEXT("sub_image_size")))
	{
		const TArray<TSharedPtr<FJsonValue>>* SizeArray;
		if (Params->TryGetArrayField(TEXT("sub_image_size"), SizeArray) && SizeArray->Num() >= 2)
		{
			SpriteRenderer->SubImageSize = FVector2D(
				(*SizeArray)[0]->AsNumber(),
				(*SizeArray)[1]->AsNumber()
			);
		}
	}

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetNumberField(TEXT("renderer_index"), RendererIndex);
	Data->SetStringField(TEXT("message"), TEXT("Sprite renderer configured"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddMeshRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	// Create mesh renderer
	UNiagaraMeshRendererProperties* MeshRenderer = NewObject<UNiagaraMeshRendererProperties>(
		Emitter,
		UNiagaraMeshRendererProperties::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	if (!MeshRenderer)
	{
		return CreateErrorResponse(TEXT("Failed to create mesh renderer"));
	}

	// Configure mesh if provided
	FString MeshPath;
	if (Params->TryGetStringField(TEXT("mesh"), MeshPath) && !MeshPath.IsEmpty())
	{
		UStaticMesh* Mesh = Cast<UStaticMesh>(UEditorAssetLibrary::LoadAsset(MeshPath));
		if (Mesh)
		{
			FNiagaraMeshRendererMeshProperties MeshProps;
			MeshProps.Mesh = Mesh;
			MeshRenderer->Meshes.Add(MeshProps);
		}
	}

	// Configure material if provided
	FString MaterialPath;
	if (Params->TryGetStringField(TEXT("material"), MaterialPath) && !MaterialPath.IsEmpty())
	{
		UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
		if (Material)
		{
			MeshRenderer->OverrideMaterials.Add(FNiagaraMeshMaterialOverride());
			MeshRenderer->OverrideMaterials[0].ExplicitMat = Material;
		}
	}

	// Add renderer to emitter
	Emitter->AddRenderer(MeshRenderer, VersionedEmitter.Version);

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("renderer_type"), TEXT("Mesh"));

	// Get renderer count through versioned emitter data
	if (FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData())
	{
		Data->SetNumberField(TEXT("renderer_index"), EmitterData->GetRenderers().Num() - 1);
	}

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleConfigureMeshRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	int32 RendererIndex = Params->HasField(TEXT("renderer_index"))
		? static_cast<int32>(Params->GetNumberField(TEXT("renderer_index")))
		: 0;

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return CreateErrorResponse(TEXT("Invalid renderer index"));
	}

	UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderers[RendererIndex]);
	if (!MeshRenderer)
	{
		return CreateErrorResponse(TEXT("Renderer at index is not a mesh renderer"));
	}

	// Configure properties
	FString FacingMode;
	if (Params->TryGetStringField(TEXT("facing_mode"), FacingMode))
	{
		if (FacingMode.Equals(TEXT("Default"), ESearchCase::IgnoreCase))
		{
			MeshRenderer->FacingMode = ENiagaraMeshFacingMode::Default;
		}
		else if (FacingMode.Equals(TEXT("Velocity"), ESearchCase::IgnoreCase))
		{
			MeshRenderer->FacingMode = ENiagaraMeshFacingMode::Velocity;
		}
		else if (FacingMode.Equals(TEXT("CameraPosition"), ESearchCase::IgnoreCase))
		{
			MeshRenderer->FacingMode = ENiagaraMeshFacingMode::CameraPosition;
		}
		else if (FacingMode.Equals(TEXT("CameraPlane"), ESearchCase::IgnoreCase))
		{
			MeshRenderer->FacingMode = ENiagaraMeshFacingMode::CameraPlane;
		}
	}

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetNumberField(TEXT("renderer_index"), RendererIndex);
	Data->SetStringField(TEXT("message"), TEXT("Mesh renderer configured"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddRibbonRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	// Create ribbon renderer
	UNiagaraRibbonRendererProperties* RibbonRenderer = NewObject<UNiagaraRibbonRendererProperties>(
		Emitter,
		UNiagaraRibbonRendererProperties::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	if (!RibbonRenderer)
	{
		return CreateErrorResponse(TEXT("Failed to create ribbon renderer"));
	}

	// Configure material if provided
	FString MaterialPath;
	if (Params->TryGetStringField(TEXT("material"), MaterialPath) && !MaterialPath.IsEmpty())
	{
		UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
		if (Material)
		{
			RibbonRenderer->Material = Material;
		}
	}

	// Add renderer to emitter
	Emitter->AddRenderer(RibbonRenderer, VersionedEmitter.Version);

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("renderer_type"), TEXT("Ribbon"));
	if (FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData())
	{
		Data->SetNumberField(TEXT("renderer_index"), EmitterData->GetRenderers().Num() - 1);
	}

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleConfigureRibbonRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	int32 RendererIndex = Params->HasField(TEXT("renderer_index"))
		? static_cast<int32>(Params->GetNumberField(TEXT("renderer_index")))
		: 0;

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return CreateErrorResponse(TEXT("Invalid renderer index"));
	}

	UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderers[RendererIndex]);
	if (!RibbonRenderer)
	{
		return CreateErrorResponse(TEXT("Renderer at index is not a ribbon renderer"));
	}

	// Configure tessellation
	if (Params->HasField(TEXT("tessellation_factor")))
	{
		int32 TessellationFactor = static_cast<int32>(Params->GetNumberField(TEXT("tessellation_factor")));
		RibbonRenderer->TessellationFactor = TessellationFactor;
	}

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetNumberField(TEXT("renderer_index"), RendererIndex);
	Data->SetStringField(TEXT("message"), TEXT("Ribbon renderer configured"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddLightRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	// Create light renderer
	UNiagaraLightRendererProperties* LightRenderer = NewObject<UNiagaraLightRendererProperties>(
		Emitter,
		UNiagaraLightRendererProperties::StaticClass(),
		NAME_None,
		RF_Transactional
	);

	if (!LightRenderer)
	{
		return CreateErrorResponse(TEXT("Failed to create light renderer"));
	}

	// Add renderer to emitter
	Emitter->AddRenderer(LightRenderer, VersionedEmitter.Version);

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetStringField(TEXT("renderer_type"), TEXT("Light"));
	if (FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData())
	{
		Data->SetNumberField(TEXT("renderer_index"), EmitterData->GetRenderers().Num() - 1);
	}

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleConfigureLightRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	int32 RendererIndex = Params->HasField(TEXT("renderer_index"))
		? static_cast<int32>(Params->GetNumberField(TEXT("renderer_index")))
		: 0;

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return CreateErrorResponse(TEXT("Invalid renderer index"));
	}

	UNiagaraLightRendererProperties* LightRenderer = Cast<UNiagaraLightRendererProperties>(Renderers[RendererIndex]);
	if (!LightRenderer)
	{
		return CreateErrorResponse(TEXT("Renderer at index is not a light renderer"));
	}

	// Configure properties
	if (Params->HasField(TEXT("radius_scale")))
	{
		LightRenderer->RadiusScale = Params->GetNumberField(TEXT("radius_scale"));
	}

	if (Params->HasField(TEXT("intensity_scale")))
	{
		LightRenderer->DefaultExponent = Params->GetNumberField(TEXT("intensity_scale"));
	}

	if (Params->HasField(TEXT("use_inverse_squared_falloff")))
	{
		LightRenderer->bUseInverseSquaredFalloff = Params->GetBoolField(TEXT("use_inverse_squared_falloff"));
	}

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetNumberField(TEXT("renderer_index"), RendererIndex);
	Data->SetStringField(TEXT("message"), TEXT("Light renderer configured"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	if (!Params->HasField(TEXT("renderer_index")))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: renderer_index"));
	}
	int32 RendererIndex = static_cast<int32>(Params->GetNumberField(TEXT("renderer_index")));

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return CreateErrorResponse(TEXT("Invalid renderer index"));
	}

	UNiagaraRendererProperties* RendererToRemove = Renderers[RendererIndex];
	Emitter->RemoveRenderer(RendererToRemove, VersionedEmitter.Version);

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetNumberField(TEXT("removed_index"), RendererIndex);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetRenderers(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	TArray<TSharedPtr<FJsonValue>> RendererArray;
	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();

	for (int32 i = 0; i < Renderers.Num(); ++i)
	{
		TSharedPtr<FJsonObject> RendererInfo = MakeShareable(new FJsonObject);
		RendererInfo->SetNumberField(TEXT("index"), i);

		UNiagaraRendererProperties* Renderer = Renderers[i];
		if (Cast<UNiagaraSpriteRendererProperties>(Renderer))
		{
			RendererInfo->SetStringField(TEXT("type"), TEXT("Sprite"));
		}
		else if (Cast<UNiagaraMeshRendererProperties>(Renderer))
		{
			RendererInfo->SetStringField(TEXT("type"), TEXT("Mesh"));
		}
		else if (Cast<UNiagaraRibbonRendererProperties>(Renderer))
		{
			RendererInfo->SetStringField(TEXT("type"), TEXT("Ribbon"));
		}
		else if (Cast<UNiagaraLightRendererProperties>(Renderer))
		{
			RendererInfo->SetStringField(TEXT("type"), TEXT("Light"));
		}
		else
		{
			RendererInfo->SetStringField(TEXT("type"), TEXT("Unknown"));
		}

		RendererInfo->SetBoolField(TEXT("enabled"), Renderer->GetIsEnabled());
		RendererArray.Add(MakeShareable(new FJsonValueObject(RendererInfo)));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("renderers"), RendererArray);
	Data->SetNumberField(TEXT("count"), RendererArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetRendererMaterial(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	if (!Params->HasField(TEXT("renderer_index")))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: renderer_index"));
	}
	int32 RendererIndex = static_cast<int32>(Params->GetNumberField(TEXT("renderer_index")));

	FString MaterialPath;
	if (!Params->TryGetStringField(TEXT("material"), MaterialPath) || MaterialPath.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: material"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraEmitterHandle* Handle = FindEmitterHandle(System, EmitterName);
	if (!Handle)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Handle->GetInstance();
	UNiagaraEmitter* Emitter = VersionedEmitter.Emitter;
	if (!Emitter)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter instance"));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	const TArray<UNiagaraRendererProperties*>& Renderers = EmitterData->GetRenderers();
	if (RendererIndex < 0 || RendererIndex >= Renderers.Num())
	{
		return CreateErrorResponse(TEXT("Invalid renderer index"));
	}

	UMaterialInterface* Material = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(MaterialPath));
	if (!Material)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
	}

	UNiagaraRendererProperties* Renderer = Renderers[RendererIndex];

	// Set material based on renderer type
	if (UNiagaraSpriteRendererProperties* SpriteRenderer = Cast<UNiagaraSpriteRendererProperties>(Renderer))
	{
		SpriteRenderer->Material = Material;
	}
	else if (UNiagaraRibbonRendererProperties* RibbonRenderer = Cast<UNiagaraRibbonRendererProperties>(Renderer))
	{
		RibbonRenderer->Material = Material;
	}
	else if (UNiagaraMeshRendererProperties* MeshRenderer = Cast<UNiagaraMeshRendererProperties>(Renderer))
	{
		if (MeshRenderer->OverrideMaterials.Num() == 0)
		{
			MeshRenderer->OverrideMaterials.Add(FNiagaraMeshMaterialOverride());
		}
		MeshRenderer->OverrideMaterials[0].ExplicitMat = Material;
	}
	else
	{
		return CreateErrorResponse(TEXT("Renderer type does not support materials"));
	}

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("emitter_name"), EmitterName);
	Data->SetNumberField(TEXT("renderer_index"), RendererIndex);
	Data->SetStringField(TEXT("material"), MaterialPath);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetRendererVisibility(
	const TSharedPtr<FJsonObject>& Params)
{
	// Implementation for visibility tags
	return CreateErrorResponse(TEXT("set_renderer_visibility not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetRendererSortMode(
	const TSharedPtr<FJsonObject>& Params)
{
	// Implementation for sort modes
	return CreateErrorResponse(TEXT("set_renderer_sort_mode not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetRendererBindings(
	const TSharedPtr<FJsonObject>& Params)
{
	// Implementation for attribute bindings
	return CreateErrorResponse(TEXT("set_renderer_bindings not yet implemented"));
}

//=============================================================================
// Sprint 4: Module Operations Commands (Stubs)
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddSpawnModule(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("add_spawn_module not yet implemented - requires NiagaraStackGraphUtilities"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddUpdateModule(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("add_update_module not yet implemented - requires NiagaraStackGraphUtilities"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddParticleSpawnModule(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("add_particle_spawn_module not yet implemented - requires NiagaraStackGraphUtilities"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddParticleUpdateModule(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("add_particle_update_module not yet implemented - requires NiagaraStackGraphUtilities"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveModule(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("remove_module not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetModules(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("get_modules not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetModuleEnabled(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("set_module_enabled not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleListAvailableModules(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("list_available_modules not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleConfigureModuleInput(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("configure_module_input not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetModuleInputs(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("get_module_inputs not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleReorderModules(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("reorder_modules not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddEventHandler(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("add_event_handler not yet implemented"));
}

//=============================================================================
// Sprint 5: Parameter System Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleExposeUserParameter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString ParamName;
	if (!Params->TryGetStringField(TEXT("param_name"), ParamName) || ParamName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: param_name"));
	}

	FString ParamType;
	if (!Params->TryGetStringField(TEXT("param_type"), ParamType) || ParamType.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: param_type"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Create the parameter variable
	FNiagaraVariable NewParam;
	NewParam.SetName(FName(*FString::Printf(TEXT("User.%s"), *ParamName)));

	// Set the type based on param_type string
	if (ParamType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
	{
		NewParam.SetType(FNiagaraTypeDefinition::GetFloatDef());
	}
	else if (ParamType.Equals(TEXT("Int32"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("Int"), ESearchCase::IgnoreCase))
	{
		NewParam.SetType(FNiagaraTypeDefinition::GetIntDef());
	}
	else if (ParamType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase))
	{
		NewParam.SetType(FNiagaraTypeDefinition::GetBoolDef());
	}
	else if (ParamType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("Vector3"), ESearchCase::IgnoreCase))
	{
		NewParam.SetType(FNiagaraTypeDefinition::GetVec3Def());
	}
	else if (ParamType.Equals(TEXT("Vector4"), ESearchCase::IgnoreCase))
	{
		NewParam.SetType(FNiagaraTypeDefinition::GetVec4Def());
	}
	else if (ParamType.Equals(TEXT("Color"), ESearchCase::IgnoreCase) || ParamType.Equals(TEXT("LinearColor"), ESearchCase::IgnoreCase))
	{
		NewParam.SetType(FNiagaraTypeDefinition::GetColorDef());
	}
	else
	{
		return CreateErrorResponse(FString::Printf(TEXT("Unknown parameter type: %s"), *ParamType));
	}

	// Add to exposed parameters
	FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
	ExposedParams.AddParameter(NewParam, true, true);

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("param_name"), ParamName);
	Data->SetStringField(TEXT("param_type"), ParamType);
	Data->SetStringField(TEXT("full_name"), NewParam.GetName().ToString());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetUserParameterDefault(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString ParamName;
	if (!Params->TryGetStringField(TEXT("param_name"), ParamName) || ParamName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: param_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Find the parameter
	FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
	FString FullParamName = ParamName.StartsWith(TEXT("User.")) ? ParamName : FString::Printf(TEXT("User.%s"), *ParamName);

	// Set value based on type and provided value
	if (Params->HasField(TEXT("value")))
	{
		TSharedPtr<FJsonValue> Value = Params->TryGetField(TEXT("value"));
		if (Value->Type == EJson::Number)
		{
			float FloatValue = Value->AsNumber();
			FNiagaraVariable FloatVar(FNiagaraTypeDefinition::GetFloatDef(), FName(*FullParamName));
			FloatVar.SetValue(FloatValue);
			ExposedParams.SetParameterData(FloatVar.GetData(), FloatVar, true);
		}
		else if (Value->Type == EJson::Boolean)
		{
			bool BoolValue = Value->AsBool();
			FNiagaraVariable BoolVar(FNiagaraTypeDefinition::GetBoolDef(), FName(*FullParamName));
			FNiagaraBool NiagaraBool(BoolValue);
			BoolVar.SetValue(NiagaraBool);
			ExposedParams.SetParameterData(BoolVar.GetData(), BoolVar, true);
		}
		else if (Value->Type == EJson::Array)
		{
			const TArray<TSharedPtr<FJsonValue>>& ArrayValue = Value->AsArray();
			if (ArrayValue.Num() == 3)
			{
				FVector VectorValue(
					ArrayValue[0]->AsNumber(),
					ArrayValue[1]->AsNumber(),
					ArrayValue[2]->AsNumber()
				);
				FNiagaraVariable VecVar(FNiagaraTypeDefinition::GetVec3Def(), FName(*FullParamName));
#if ENGINE_MINOR_VERSION >= 4
				VecVar.SetValue(VectorValue);
#else
				VecVar.SetValue(FVector3f(VectorValue));
#endif
				ExposedParams.SetParameterData(VecVar.GetData(), VecVar, true);
			}
			else if (ArrayValue.Num() == 4)
			{
				FLinearColor ColorValue(
					ArrayValue[0]->AsNumber(),
					ArrayValue[1]->AsNumber(),
					ArrayValue[2]->AsNumber(),
					ArrayValue[3]->AsNumber()
				);
				FNiagaraVariable ColorVar(FNiagaraTypeDefinition::GetColorDef(), FName(*FullParamName));
				ColorVar.SetValue(ColorValue);
				ExposedParams.SetParameterData(ColorVar.GetData(), ColorVar, true);
			}
		}
	}

	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("param_name"), FullParamName);
	Data->SetStringField(TEXT("message"), TEXT("Default value set"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetUserParameters(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	const FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
	TArray<FNiagaraVariable> UserParams;
	ExposedParams.GetUserParameters(UserParams);

	TArray<TSharedPtr<FJsonValue>> ParamArray;
	for (const FNiagaraVariable& Param : UserParams)
	{
		TSharedPtr<FJsonObject> ParamInfo = MakeShareable(new FJsonObject);
		ParamInfo->SetStringField(TEXT("name"), Param.GetName().ToString());
		ParamInfo->SetStringField(TEXT("type"), Param.GetType().GetName());
		ParamArray.Add(MakeShareable(new FJsonValueObject(ParamInfo)));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("parameters"), ParamArray);
	Data->SetNumberField(TEXT("count"), ParamArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleRemoveUserParameter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString ParamName;
	if (!Params->TryGetStringField(TEXT("param_name"), ParamName) || ParamName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: param_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	FNiagaraUserRedirectionParameterStore& ExposedParams = System->GetExposedParameters();
	FString FullParamName = ParamName.StartsWith(TEXT("User.")) ? ParamName : FString::Printf(TEXT("User.%s"), *ParamName);

	FNiagaraVariableBase VarToRemove(FNiagaraTypeDefinition::GetFloatDef(), FName(*FullParamName));
	ExposedParams.RemoveParameter(VarToRemove);

	System->RequestCompile(false);
	System->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("removed"), FullParamName);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleBindParameter(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("bind_parameter not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetNiagaraSystemProperties(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITORONLY_DATA
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	bool bModified = false;

	// System-level properties (when no emitter_name specified)
	FString EmitterName;
	bool bHasEmitterName = Params->TryGetStringField(TEXT("emitter_name"), EmitterName) && !EmitterName.IsEmpty();

	if (!bHasEmitterName)
	{
		// Set fixed bounds
		if (Params->HasField(TEXT("fixed_bounds")))
		{
			const TArray<TSharedPtr<FJsonValue>>* BoundsArray;
			if (Params->TryGetArrayField(TEXT("fixed_bounds"), BoundsArray) && BoundsArray->Num() >= 6)
			{
				FBox Bounds(
					FVector(
						(*BoundsArray)[0]->AsNumber(),
						(*BoundsArray)[1]->AsNumber(),
						(*BoundsArray)[2]->AsNumber()
					),
					FVector(
						(*BoundsArray)[3]->AsNumber(),
						(*BoundsArray)[4]->AsNumber(),
						(*BoundsArray)[5]->AsNumber()
					)
				);
				System->SetFixedBounds(Bounds);
				bModified = true;
			}
		}

		// Set warmup time
		if (Params->HasField(TEXT("warmup_time")))
		{
			float WarmupTime = Params->GetNumberField(TEXT("warmup_time"));
			System->SetWarmupTime(WarmupTime);
			bModified = true;
		}

		if (bModified)
		{
			System->MarkPackageDirty();
		}

		TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetBoolField(TEXT("success"), bModified);
		Data->SetStringField(TEXT("system"), SystemPath);
		Data->SetStringField(TEXT("message"), TEXT("System properties updated"));
		return CreateSuccessResponse(Data);
	}

	// Emitter-level properties (when emitter_name IS specified)

	// Find the emitter
	UNiagaraEmitter* Emitter = nullptr;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase))
		{
			FVersionedNiagaraEmitterData* EmitterData = Handle.GetEmitterData();
			if (EmitterData)
			{
				Emitter = const_cast<UNiagaraEmitter*>(Handle.GetInstance().Emitter.Get());
				break;
			}
		}
	}

	if (!Emitter)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitter VersionedEmitter = Emitter->GetLatestEmitterData()->GetParent();
	if (!VersionedEmitter.Emitter)
	{
		VersionedEmitter = FVersionedNiagaraEmitter(Emitter, Emitter->GetExposedVersion().VersionGuid);
	}

	FVersionedNiagaraEmitterData* EmitterData = VersionedEmitter.GetEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	// Access spawn script rapid iteration parameters (module inputs)
	UNiagaraScript* SpawnScript = EmitterData->SpawnScriptProps.Script;
	if (!SpawnScript)
	{
		return CreateErrorResponse(TEXT("Spawn script not found"));
	}

	FNiagaraParameterStore& RapidIterParams = SpawnScript->RapidIterationParameters;
	FString EmitterNameStr = EmitterName;

	// Set spawn rate (emitter-level parameter)
	double SpawnRate;
	if (Params->TryGetNumberField(TEXT("spawn_rate"), SpawnRate))
	{
		// Try common spawn rate parameter names
		TArray<FString> SpawnRateNames = {
			FString::Printf(TEXT("Constants.%s.SpawnRate"), *EmitterNameStr),
			TEXT("Constants.Emitter.SpawnRate"),
			FString::Printf(TEXT("Constants.%s.Emitter Spawn Rate.Spawn Rate"), *EmitterNameStr)
		};

		for (const FString& ParamName : SpawnRateNames)
		{
			FNiagaraVariable SpawnRateVar(FNiagaraTypeDefinition::GetFloatDef(), FName(*ParamName));
			if (RapidIterParams.SetParameterValue((float)SpawnRate, SpawnRateVar, true))
			{
				bModified = true;
				break;
			}
		}
	}

	// Set particle lifetime
	double Lifetime;
	if (Params->TryGetNumberField(TEXT("lifetime"), Lifetime))
	{
		// Common lifetime parameter names
		TArray<FString> LifetimeNames = {
			FString::Printf(TEXT("Constants.%s.Initialize Particle.Lifetime"), *EmitterNameStr),
			FString::Printf(TEXT("Constants.%s.InitializeParticle.Lifetime"), *EmitterNameStr),
			TEXT("Constants.Emitter.Initialize Particle.Lifetime")
		};

		for (const FString& ParamName : LifetimeNames)
		{
			FNiagaraVariable LifetimeVar(FNiagaraTypeDefinition::GetFloatDef(), FName(*ParamName));
			if (RapidIterParams.SetParameterValue((float)Lifetime, LifetimeVar, true))
			{
				bModified = true;
				break;
			}
		}
	}

	// Set particle size (sprite size is typically a Vector2)
	double Size;
	if (Params->TryGetNumberField(TEXT("size"), Size))
	{
		FVector2f SpriteSize((float)Size, (float)Size);

		TArray<FString> SizeNames = {
			FString::Printf(TEXT("Constants.%s.Initialize Particle.Sprite Size"), *EmitterNameStr),
			FString::Printf(TEXT("Constants.%s.InitializeParticle.SpriteSize"), *EmitterNameStr),
			TEXT("Constants.Emitter.Initialize Particle.Sprite Size")
		};

		for (const FString& ParamName : SizeNames)
		{
			FNiagaraVariable SizeVar(FNiagaraTypeDefinition::GetVec2Def(), FName(*ParamName));
			if (RapidIterParams.SetParameterValue(SpriteSize, SizeVar, true))
			{
				bModified = true;
				break;
			}
		}
	}

	// Set initial velocity
	const TArray<TSharedPtr<FJsonValue>>* VelocityArray;
	if (Params->TryGetArrayField(TEXT("initial_velocity"), VelocityArray) && VelocityArray->Num() >= 3)
	{
		FVector Velocity(
			(*VelocityArray)[0]->AsNumber(),
			(*VelocityArray)[1]->AsNumber(),
			(*VelocityArray)[2]->AsNumber()
		);

		TArray<FString> VelocityNames = {
			FString::Printf(TEXT("Constants.%s.Initialize Particle.Initial Velocity"), *EmitterNameStr),
			FString::Printf(TEXT("Constants.%s.InitializeParticle.InitialVelocity"), *EmitterNameStr),
			TEXT("Constants.Emitter.Initialize Particle.Initial Velocity")
		};

		for (const FString& ParamName : VelocityNames)
		{
			FNiagaraVariable VelocityVar(FNiagaraTypeDefinition::GetVec3Def(), FName(*ParamName));
			if (RapidIterParams.SetParameterValue(Velocity, VelocityVar, true))
			{
				bModified = true;
				break;
			}
		}
	}

	// Set particle color
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
	{
		FLinearColor Color(
			(*ColorArray)[0]->AsNumber(),
			(*ColorArray)[1]->AsNumber(),
			(*ColorArray)[2]->AsNumber(),
			ColorArray->Num() > 3 ? (*ColorArray)[3]->AsNumber() : 1.0f
		);

		TArray<FString> ColorNames = {
			FString::Printf(TEXT("Constants.%s.Initialize Particle.Color"), *EmitterNameStr),
			FString::Printf(TEXT("Constants.%s.InitializeParticle.Color"), *EmitterNameStr),
			TEXT("Constants.Emitter.Initialize Particle.Color")
		};

		for (const FString& ParamName : ColorNames)
		{
			FNiagaraVariable ColorVar(FNiagaraTypeDefinition::GetColorDef(), FName(*ParamName));
			if (RapidIterParams.SetParameterValue(Color, ColorVar, true))
			{
				bModified = true;
				break;
			}
		}
	}

	if (bModified)
	{
		System->RequestCompile(false);
		System->MarkPackageDirty();
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetBoolField(TEXT("success"), bModified);
	Data->SetStringField(TEXT("system"), SystemPath);
	Data->SetStringField(TEXT("emitter"), EmitterName);

	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Niagara editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleGetParameterBindings(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Find emitter
	UNiagaraEmitter* Emitter = nullptr;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase))
		{
			Emitter = const_cast<UNiagaraEmitter*>(Handle.GetInstance().Emitter.Get());
			break;
		}
	}

	if (!Emitter)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter data"));
	}

	// Get rapid iteration parameters from spawn and update scripts
	TArray<TSharedPtr<FJsonValue>> ParameterArray;

	auto CollectParameters = [&](UNiagaraScript* Script, const FString& ScriptType)
	{
		if (!Script) return;

		const FNiagaraParameterStore& RapidIterParams = Script->RapidIterationParameters;
		TArray<FNiagaraVariable> Variables;
		RapidIterParams.GetParameters(Variables);

		for (const FNiagaraVariable& Var : Variables)
		{
			TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
			ParamObj->SetStringField(TEXT("name"), Var.GetName().ToString());
			ParamObj->SetStringField(TEXT("type"), Var.GetType().GetName());
			ParamObj->SetStringField(TEXT("script"), ScriptType);

			// Try to get current value as string
			const uint8* Data = RapidIterParams.GetParameterData(Var);
			if (Data)
			{
				FString ValueStr;
				if (Var.GetType() == FNiagaraTypeDefinition::GetFloatDef())
				{
					float Value = *reinterpret_cast<const float*>(Data);
					ValueStr = FString::Printf(TEXT("%.3f"), Value);
				}
				else if (Var.GetType() == FNiagaraTypeDefinition::GetIntDef())
				{
					int32 Value = *reinterpret_cast<const int32*>(Data);
					ValueStr = FString::Printf(TEXT("%d"), Value);
				}
				else if (Var.GetType() == FNiagaraTypeDefinition::GetBoolDef())
				{
					bool Value = *reinterpret_cast<const FNiagaraBool*>(Data);
					ValueStr = Value ? TEXT("true") : TEXT("false");
				}
				else if (Var.GetType() == FNiagaraTypeDefinition::GetVec2Def())
				{
					FVector2f Value = *reinterpret_cast<const FVector2f*>(Data);
					ValueStr = FString::Printf(TEXT("[%.3f, %.3f]"), Value.X, Value.Y);
				}
				else if (Var.GetType() == FNiagaraTypeDefinition::GetVec3Def())
				{
					FVector Value = *reinterpret_cast<const FVector*>(Data);
					ValueStr = FString::Printf(TEXT("[%.3f, %.3f, %.3f]"), Value.X, Value.Y, Value.Z);
				}
				else if (Var.GetType() == FNiagaraTypeDefinition::GetColorDef())
				{
					FLinearColor Value = *reinterpret_cast<const FLinearColor*>(Data);
					ValueStr = FString::Printf(TEXT("[%.3f, %.3f, %.3f, %.3f]"), Value.R, Value.G, Value.B, Value.A);
				}
				else
				{
					ValueStr = TEXT("<complex type>");
				}
				ParamObj->SetStringField(TEXT("value"), ValueStr);
			}

			ParameterArray.Add(MakeShared<FJsonValueObject>(ParamObj));
		}
	};

	CollectParameters(EmitterData->SpawnScriptProps.Script, TEXT("Spawn"));
	CollectParameters(EmitterData->UpdateScriptProps.Script, TEXT("Update"));

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("system"), SystemPath);
	Data->SetStringField(TEXT("emitter"), EmitterName);
	Data->SetNumberField(TEXT("parameter_count"), ParameterArray.Num());
	Data->SetArrayField(TEXT("parameters"), ParameterArray);

	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Niagara editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetEmitterParameter(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString SystemName;
	if (!Params->TryGetStringField(TEXT("system_name"), SystemName) || SystemName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: system_name"));
	}

	FString EmitterName;
	if (!Params->TryGetStringField(TEXT("emitter_name"), EmitterName) || EmitterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: emitter_name"));
	}

	FString ParameterName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName) || ParameterName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: parameter_name"));
	}

	FString SystemPath;
	UNiagaraSystem* System = LoadNiagaraSystem(SystemName, SystemPath);
	if (!System)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Niagara System not found: %s"), *SystemName));
	}

	// Find emitter
	UNiagaraEmitter* Emitter = nullptr;
	for (const FNiagaraEmitterHandle& Handle : System->GetEmitterHandles())
	{
		if (Handle.GetName().ToString().Equals(EmitterName, ESearchCase::IgnoreCase))
		{
			Emitter = const_cast<UNiagaraEmitter*>(Handle.GetInstance().Emitter.Get());
			break;
		}
	}

	if (!Emitter)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Emitter not found: %s"), *EmitterName));
	}

	FVersionedNiagaraEmitterData* EmitterData = Emitter->GetLatestEmitterData();
	if (!EmitterData || !EmitterData->SpawnScriptProps.Script)
	{
		return CreateErrorResponse(TEXT("Failed to get emitter spawn script"));
	}

	FNiagaraParameterStore& RapidIterParams = EmitterData->SpawnScriptProps.Script->RapidIterationParameters;

	// Try to find existing parameter to determine type
	FNiagaraVariable ExistingVar(FNiagaraTypeDefinition::GetFloatDef(), FName(*ParameterName));
	TArray<FNiagaraVariable> Variables;
	RapidIterParams.GetParameters(Variables);

	for (const FNiagaraVariable& Var : Variables)
	{
		if (Var.GetName().ToString().Equals(ParameterName, ESearchCase::IgnoreCase))
		{
			ExistingVar = Var;
			break;
		}
	}

	bool bSuccess = false;

	// Set value based on type
	if (ExistingVar.GetType() == FNiagaraTypeDefinition::GetFloatDef())
	{
		double Value = Params->GetNumberField(TEXT("value"));
		bSuccess = RapidIterParams.SetParameterValue((float)Value, ExistingVar, true);
	}
	else if (ExistingVar.GetType() == FNiagaraTypeDefinition::GetIntDef())
	{
		int32 Value = (int32)Params->GetNumberField(TEXT("value"));
		bSuccess = RapidIterParams.SetParameterValue(Value, ExistingVar, true);
	}
	else if (ExistingVar.GetType() == FNiagaraTypeDefinition::GetBoolDef())
	{
		bool Value = Params->GetBoolField(TEXT("value"));
		FNiagaraBool NiagaraBool(Value);
		bSuccess = RapidIterParams.SetParameterValue(NiagaraBool, ExistingVar, true);
	}
	else if (ExistingVar.GetType() == FNiagaraTypeDefinition::GetVec2Def())
	{
		const TArray<TSharedPtr<FJsonValue>>* ValueArray;
		if (Params->TryGetArrayField(TEXT("value"), ValueArray) && ValueArray->Num() >= 2)
		{
			FVector2f Value((*ValueArray)[0]->AsNumber(), (*ValueArray)[1]->AsNumber());
			bSuccess = RapidIterParams.SetParameterValue(Value, ExistingVar, true);
		}
	}
	else if (ExistingVar.GetType() == FNiagaraTypeDefinition::GetVec3Def())
	{
		const TArray<TSharedPtr<FJsonValue>>* ValueArray;
		if (Params->TryGetArrayField(TEXT("value"), ValueArray) && ValueArray->Num() >= 3)
		{
			FVector Value((*ValueArray)[0]->AsNumber(), (*ValueArray)[1]->AsNumber(), (*ValueArray)[2]->AsNumber());
			bSuccess = RapidIterParams.SetParameterValue(Value, ExistingVar, true);
		}
	}
	else if (ExistingVar.GetType() == FNiagaraTypeDefinition::GetColorDef())
	{
		const TArray<TSharedPtr<FJsonValue>>* ValueArray;
		if (Params->TryGetArrayField(TEXT("value"), ValueArray) && ValueArray->Num() >= 3)
		{
			FLinearColor Value(
				(*ValueArray)[0]->AsNumber(),
				(*ValueArray)[1]->AsNumber(),
				(*ValueArray)[2]->AsNumber(),
				ValueArray->Num() > 3 ? (*ValueArray)[3]->AsNumber() : 1.0f
			);
			bSuccess = RapidIterParams.SetParameterValue(Value, ExistingVar, true);
		}
	}

	if (bSuccess)
	{
		System->RequestCompile(false);
		System->MarkPackageDirty();
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetBoolField(TEXT("success"), bSuccess);
	Data->SetStringField(TEXT("parameter"), ParameterName);
	return CreateSuccessResponse(Data);
#else
	return CreateErrorResponse(TEXT("Niagara editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetParticleParameter(const TSharedPtr<FJsonObject>& Params)
{
	// Alias to HandleSetEmitterParameter - particle parameters are also rapid iteration parameters
	return HandleSetEmitterParameter(Params);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCreateParameterCollection(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("create_parameter_collection not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleOverrideParameterCollection(const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("override_parameter_collection not yet implemented"));
}

//=============================================================================
// Sprint 6: Material Integration Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCreateParticleMaterial(
	const TSharedPtr<FJsonObject>& Params)
{
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName) || MaterialName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game/Materials");

	FString BlendMode = Params->HasField(TEXT("blend_mode"))
		? Params->GetStringField(TEXT("blend_mode"))
		: TEXT("Translucent");

	bool bForSprites = Params->HasField(TEXT("for_sprites"))
		? Params->GetBoolField(TEXT("for_sprites"))
		: true;

	bool bForMeshes = Params->HasField(TEXT("for_meshes"))
		? Params->GetBoolField(TEXT("for_meshes"))
		: false;

	bool bForRibbons = Params->HasField(TEXT("for_ribbons"))
		? Params->GetBoolField(TEXT("for_ribbons"))
		: false;

	// Create package
	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *MaterialName);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create material
	UMaterial* NewMaterial = NewObject<UMaterial>(
		Package,
		*MaterialName,
		RF_Public | RF_Standalone | RF_Transactional
	);

	if (!NewMaterial)
	{
		return CreateErrorResponse(TEXT("Failed to create material"));
	}

	// Set blend mode
	if (BlendMode.Equals(TEXT("Opaque"), ESearchCase::IgnoreCase))
	{
		NewMaterial->BlendMode = BLEND_Opaque;
	}
	else if (BlendMode.Equals(TEXT("Translucent"), ESearchCase::IgnoreCase))
	{
		NewMaterial->BlendMode = BLEND_Translucent;
	}
	else if (BlendMode.Equals(TEXT("Additive"), ESearchCase::IgnoreCase))
	{
		NewMaterial->BlendMode = BLEND_Additive;
	}
	else if (BlendMode.Equals(TEXT("Modulate"), ESearchCase::IgnoreCase))
	{
		NewMaterial->BlendMode = BLEND_Modulate;
	}

	// Enable for particle systems
	NewMaterial->bUsedWithNiagaraSprites = bForSprites;
	NewMaterial->bUsedWithNiagaraMeshParticles = bForMeshes;
	NewMaterial->bUsedWithNiagaraRibbons = bForRibbons;

	// Mark dirty and save
	Package->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewMaterial, *PackageFileName, SaveArgs);

	FAssetRegistryModule::AssetCreated(NewMaterial);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("material_name"), MaterialName);
	Data->SetStringField(TEXT("path"), PackagePath);
	Data->SetStringField(TEXT("blend_mode"), BlendMode);
	Data->SetBoolField(TEXT("for_sprites"), bForSprites);
	Data->SetBoolField(TEXT("for_meshes"), bForMeshes);
	Data->SetBoolField(TEXT("for_ribbons"), bForRibbons);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAssignMaterialToRenderer(
	const TSharedPtr<FJsonObject>& Params)
{
	// Delegates to set_renderer_material
	return HandleSetRendererMaterial(Params);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleSetDynamicMaterialBinding(
	const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("set_dynamic_material_binding not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleCreateParticleMaterialInstance(
	const TSharedPtr<FJsonObject>& Params)
{
	FString InstanceName;
	if (!Params->TryGetStringField(TEXT("instance_name"), InstanceName) || InstanceName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: instance_name"));
	}

	FString ParentPath;
	if (!Params->TryGetStringField(TEXT("parent_material"), ParentPath) || ParentPath.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: parent_material"));
	}

	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game/Materials");

	// Load parent material
	UMaterialInterface* ParentMaterial = Cast<UMaterialInterface>(UEditorAssetLibrary::LoadAsset(ParentPath));
	if (!ParentMaterial)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Parent material not found: %s"), *ParentPath));
	}

	// Create package
	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *InstanceName);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(TEXT("Failed to create package"));
	}

	// Create material instance
	UMaterialInstanceConstant* NewInstance = NewObject<UMaterialInstanceConstant>(
		Package,
		*InstanceName,
		RF_Public | RF_Standalone | RF_Transactional
	);

	if (!NewInstance)
	{
		return CreateErrorResponse(TEXT("Failed to create material instance"));
	}

	NewInstance->SetParentEditorOnly(ParentMaterial);

	// Mark dirty and save
	Package->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewInstance, *PackageFileName, SaveArgs);

	FAssetRegistryModule::AssetCreated(NewInstance);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("instance_name"), InstanceName);
	Data->SetStringField(TEXT("path"), PackagePath);
	Data->SetStringField(TEXT("parent"), ParentPath);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleConfigureMaterialParameters(
	const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("configure_material_parameters not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPNiagaraCommands::HandleAddParticleColorNode(
	const TSharedPtr<FJsonObject>& Params)
{
	return CreateErrorResponse(TEXT("add_particle_color_node not yet implemented - use create_material with existing material_tools"));
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPNiagaraCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// Asset Management
	Registry.RegisterCommand(TEXT("create_niagara_system"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_niagara_system"), P); });
	Registry.RegisterCommand(TEXT("delete_niagara_system"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_niagara_system"), P); });
	Registry.RegisterCommand(TEXT("get_niagara_system_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_niagara_system_info"), P); });
	Registry.RegisterCommand(TEXT("list_niagara_systems"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_niagara_systems"), P); });
	Registry.RegisterCommand(TEXT("open_niagara_system"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("open_niagara_system"), P); });
	Registry.RegisterCommand(TEXT("compile_niagara_system"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("compile_niagara_system"), P); });
	Registry.RegisterCommand(TEXT("save_niagara_system"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("save_niagara_system"), P); });
	Registry.RegisterCommand(TEXT("set_niagara_system_properties"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_niagara_system_properties"), P); });
	// Emitter Management
	Registry.RegisterCommand(TEXT("add_emitter_to_system"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_emitter_to_system"), P); });
	Registry.RegisterCommand(TEXT("remove_emitter_from_system"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_emitter_from_system"), P); });
	Registry.RegisterCommand(TEXT("duplicate_emitter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("duplicate_emitter"), P); });
	Registry.RegisterCommand(TEXT("enable_emitter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("enable_emitter"), P); });
	Registry.RegisterCommand(TEXT("disable_emitter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("disable_emitter"), P); });
	Registry.RegisterCommand(TEXT("rename_emitter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("rename_emitter"), P); });
	Registry.RegisterCommand(TEXT("get_emitter_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_emitter_info"), P); });
	Registry.RegisterCommand(TEXT("list_emitters"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_emitters"), P); });
	Registry.RegisterCommand(TEXT("set_emitter_mode"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_emitter_mode"), P); });
	Registry.RegisterCommand(TEXT("isolate_emitter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("isolate_emitter"), P); });
	// Renderer Configuration
	Registry.RegisterCommand(TEXT("add_sprite_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_sprite_renderer"), P); });
	Registry.RegisterCommand(TEXT("configure_sprite_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("configure_sprite_renderer"), P); });
	Registry.RegisterCommand(TEXT("add_mesh_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_mesh_renderer"), P); });
	Registry.RegisterCommand(TEXT("configure_mesh_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("configure_mesh_renderer"), P); });
	Registry.RegisterCommand(TEXT("add_ribbon_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_ribbon_renderer"), P); });
	Registry.RegisterCommand(TEXT("configure_ribbon_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("configure_ribbon_renderer"), P); });
	Registry.RegisterCommand(TEXT("add_light_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_light_renderer"), P); });
	Registry.RegisterCommand(TEXT("configure_light_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("configure_light_renderer"), P); });
	Registry.RegisterCommand(TEXT("remove_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_renderer"), P); });
	Registry.RegisterCommand(TEXT("get_renderers"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_renderers"), P); });
	Registry.RegisterCommand(TEXT("set_renderer_material"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_renderer_material"), P); });
	Registry.RegisterCommand(TEXT("set_renderer_visibility"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_renderer_visibility"), P); });
	Registry.RegisterCommand(TEXT("set_renderer_sort_mode"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_renderer_sort_mode"), P); });
	Registry.RegisterCommand(TEXT("set_renderer_bindings"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_renderer_bindings"), P); });
	// Module Operations
	Registry.RegisterCommand(TEXT("add_spawn_module"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_spawn_module"), P); });
	Registry.RegisterCommand(TEXT("add_update_module"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_update_module"), P); });
	Registry.RegisterCommand(TEXT("add_particle_spawn_module"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_particle_spawn_module"), P); });
	Registry.RegisterCommand(TEXT("add_particle_update_module"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_particle_update_module"), P); });
	Registry.RegisterCommand(TEXT("remove_module"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_module"), P); });
	Registry.RegisterCommand(TEXT("get_modules"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_modules"), P); });
	Registry.RegisterCommand(TEXT("set_module_enabled"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_module_enabled"), P); });
	Registry.RegisterCommand(TEXT("list_available_modules"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_available_modules"), P); });
	Registry.RegisterCommand(TEXT("configure_module_input"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("configure_module_input"), P); });
	Registry.RegisterCommand(TEXT("get_module_inputs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_module_inputs"), P); });
	Registry.RegisterCommand(TEXT("reorder_modules"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("reorder_modules"), P); });
	Registry.RegisterCommand(TEXT("add_event_handler"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_event_handler"), P); });
	// Parameter System
	Registry.RegisterCommand(TEXT("expose_user_parameter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("expose_user_parameter"), P); });
	Registry.RegisterCommand(TEXT("set_user_parameter_default"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_user_parameter_default"), P); });
	Registry.RegisterCommand(TEXT("get_user_parameters"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_user_parameters"), P); });
	Registry.RegisterCommand(TEXT("remove_user_parameter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_user_parameter"), P); });
	Registry.RegisterCommand(TEXT("bind_parameter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("bind_parameter"), P); });
	Registry.RegisterCommand(TEXT("set_emitter_parameter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_emitter_parameter"), P); });
	Registry.RegisterCommand(TEXT("set_particle_parameter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_particle_parameter"), P); });
	Registry.RegisterCommand(TEXT("get_parameter_bindings"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_parameter_bindings"), P); });
	Registry.RegisterCommand(TEXT("create_parameter_collection"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_parameter_collection"), P); });
	Registry.RegisterCommand(TEXT("override_parameter_collection"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("override_parameter_collection"), P); });
	// Material Integration
	Registry.RegisterCommand(TEXT("create_particle_material"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_particle_material"), P); });
	Registry.RegisterCommand(TEXT("assign_material_to_renderer"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("assign_material_to_renderer"), P); });
	Registry.RegisterCommand(TEXT("set_dynamic_material_binding"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_dynamic_material_binding"), P); });
	Registry.RegisterCommand(TEXT("create_particle_material_instance"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_particle_material_instance"), P); });
	Registry.RegisterCommand(TEXT("configure_material_parameters"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("configure_material_parameters"), P); });
	Registry.RegisterCommand(TEXT("add_particle_color_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_particle_color_node"), P); });
}
