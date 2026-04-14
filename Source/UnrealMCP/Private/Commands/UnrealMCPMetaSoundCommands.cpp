// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPMetaSoundCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "MetasoundBuilderSubsystem.h"
#include "MetasoundSource.h"
#include "MetasoundFrontendDocument.h"
#include "MetasoundFactory.h"
#include "MetasoundEditorSubsystem.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// Asset Creation Commands
	if (CommandType == TEXT("create_metasound_source"))
	{
		return HandleCreateMetaSoundSource(Params);
	}
	else if (CommandType == TEXT("create_metasound_preset"))
	{
		return HandleCreateMetaSoundPreset(Params);
	}
	else if (CommandType == TEXT("delete_metasound"))
	{
		return HandleDeleteMetaSound(Params);
	}
	else if (CommandType == TEXT("get_metasound_info"))
	{
		return HandleGetMetaSoundInfo(Params);
	}
	else if (CommandType == TEXT("list_metasound_assets"))
	{
		return HandleListMetaSoundAssets(Params);
	}
	else if (CommandType == TEXT("open_metasound"))
	{
		return HandleOpenMetaSound(Params);
	}
	// Graph I/O Commands
	else if (CommandType == TEXT("add_metasound_input"))
	{
		return HandleAddMetaSoundInput(Params);
	}
	else if (CommandType == TEXT("add_metasound_output"))
	{
		return HandleAddMetaSoundOutput(Params);
	}
	else if (CommandType == TEXT("remove_metasound_input"))
	{
		return HandleRemoveMetaSoundInput(Params);
	}
	else if (CommandType == TEXT("remove_metasound_output"))
	{
		return HandleRemoveMetaSoundOutput(Params);
	}
	else if (CommandType == TEXT("get_metasound_inputs"))
	{
		return HandleGetMetaSoundInputs(Params);
	}
	else if (CommandType == TEXT("get_metasound_outputs"))
	{
		return HandleGetMetaSoundOutputs(Params);
	}
	else if (CommandType == TEXT("set_metasound_input_default"))
	{
		return HandleSetInputDefault(Params);
	}
	// Node Management Commands
	else if (CommandType == TEXT("add_metasound_node"))
	{
		return HandleAddMetaSoundNode(Params);
	}
	else if (CommandType == TEXT("delete_metasound_node"))
	{
		return HandleDeleteMetaSoundNode(Params);
	}
	else if (CommandType == TEXT("get_metasound_nodes"))
	{
		return HandleGetMetaSoundNodes(Params);
	}
	else if (CommandType == TEXT("find_metasound_node"))
	{
		return HandleFindMetaSoundNode(Params);
	}
	else if (CommandType == TEXT("get_metasound_node_inputs"))
	{
		return HandleGetNodeInputs(Params);
	}
	else if (CommandType == TEXT("get_metasound_node_outputs"))
	{
		return HandleGetNodeOutputs(Params);
	}
	else if (CommandType == TEXT("set_metasound_node_input_default"))
	{
		return HandleSetNodeInputDefault(Params);
	}
	// Connection Commands
	else if (CommandType == TEXT("connect_metasound_nodes"))
	{
		return HandleConnectMetaSoundNodes(Params);
	}
	else if (CommandType == TEXT("disconnect_metasound_nodes"))
	{
		return HandleDisconnectMetaSoundNodes(Params);
	}
	else if (CommandType == TEXT("connect_to_metasound_output"))
	{
		return HandleConnectToGraphOutput(Params);
	}
	else if (CommandType == TEXT("get_metasound_node_connections"))
	{
		return HandleGetNodeConnections(Params);
	}
	// Variable Commands
	else if (CommandType == TEXT("add_metasound_variable"))
	{
		return HandleAddMetaSoundVariable(Params);
	}
	else if (CommandType == TEXT("add_metasound_variable_getter"))
	{
		return HandleAddVariableGetter(Params);
	}
	else if (CommandType == TEXT("add_metasound_variable_setter"))
	{
		return HandleAddVariableSetter(Params);
	}
	// Build Commands
	else if (CommandType == TEXT("build_metasound"))
	{
		return HandleBuildMetaSound(Params);
	}
	else if (CommandType == TEXT("build_overwrite_metasound"))
	{
		return HandleBuildAndOverwriteMetaSound(Params);
	}
	else if (CommandType == TEXT("validate_metasound"))
	{
		return HandleValidateMetaSound(Params);
	}
	// Utility Commands
	else if (CommandType == TEXT("list_metasound_node_types"))
	{
		return HandleListMetaSoundNodeTypes(Params);
	}
	else if (CommandType == TEXT("get_metasound_data_types"))
	{
		return HandleGetMetaSoundDataTypes(Params);
	}
	else if (CommandType == TEXT("auto_layout_metasound"))
	{
		return HandleAutoLayoutMetaSound(Params);
	}

	return CreateErrorResponse(FString::Printf(TEXT("Unknown MetaSound command: %s"), *CommandType));
}

//=============================================================================
// Asset Creation Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleCreateMetaSoundSource(
	const TSharedPtr<FJsonObject>& Params)
{
	// Parse parameters
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName) || SoundName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game/MetaSounds");

	FString OutputFormatStr = Params->HasField(TEXT("output_format"))
		? Params->GetStringField(TEXT("output_format"))
		: TEXT("Mono");

	bool bIsOneShot = Params->HasField(TEXT("is_one_shot"))
		? Params->GetBoolField(TEXT("is_one_shot"))
		: true;

	// Get builder subsystem
	UMetaSoundBuilderSubsystem* BuilderSubsystem = GEngine->GetEngineSubsystem<UMetaSoundBuilderSubsystem>();
	if (!BuilderSubsystem)
	{
		return CreateErrorResponse(TEXT("Failed to get MetaSoundBuilderSubsystem"));
	}

	// Convert output format
	EMetaSoundOutputAudioFormat OutputFormat = StringToOutputFormat(OutputFormatStr);

	// Create the builder
	EMetaSoundBuilderResult Result;
	FMetaSoundBuilderNodeOutputHandle OnPlayOutput;
	FMetaSoundBuilderNodeInputHandle OnFinishedInput;
	TArray<FMetaSoundBuilderNodeInputHandle> AudioOutInputs;

	UMetaSoundSourceBuilder* Builder = BuilderSubsystem->CreateSourceBuilder(
		FName(*SoundName),
		OnPlayOutput,
		OnFinishedInput,
		AudioOutInputs,
		Result,
		OutputFormat,
		bIsOneShot
	);

	if (Result != EMetaSoundBuilderResult::Succeeded || !Builder)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to create MetaSound builder: %s"),
			*BuilderResultToString(Result)
		));
	}

	// Cache the builder
	ActiveBuilders.Add(SoundName, Builder);

	// Prepare response
	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("sound_name"), SoundName);
	Data->SetStringField(TEXT("path"), Path);
	Data->SetStringField(TEXT("output_format"), OutputFormatStr);
	Data->SetBoolField(TEXT("is_one_shot"), bIsOneShot);
	Data->SetStringField(TEXT("message"), TEXT("MetaSound builder created. Use build_metasound to save the asset."));

	// Include audio output count
	Data->SetNumberField(TEXT("audio_output_count"), AudioOutInputs.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleCreateMetaSoundPreset(
	const TSharedPtr<FJsonObject>& Params)
{
	FString PresetName;
	if (!Params->TryGetStringField(TEXT("preset_name"), PresetName) || PresetName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: preset_name"));
	}

	FString SourceName;
	if (!Params->TryGetStringField(TEXT("source_name"), SourceName) || SourceName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: source_name"));
	}

	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game/MetaSounds");

	// Load the source MetaSound
	FString SourcePath;
	UMetaSoundSource* SourceAsset = LoadMetaSoundSource(SourceName, SourcePath);
	if (!SourceAsset)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to load source MetaSound: %s"), *SourceName));
	}

	// Get builder subsystem
	UMetaSoundBuilderSubsystem* BuilderSubsystem = GEngine->GetEngineSubsystem<UMetaSoundBuilderSubsystem>();
	if (!BuilderSubsystem)
	{
		return CreateErrorResponse(TEXT("Failed to get MetaSoundBuilderSubsystem"));
	}

	// Create preset builder
	EMetaSoundBuilderResult Result;
	TScriptInterface<IMetaSoundDocumentInterface> SourceInterface = SourceAsset;

	UMetaSoundSourceBuilder* PresetBuilder = BuilderSubsystem->CreateSourcePresetBuilder(
		FName(*PresetName),
		SourceInterface,
		Result
	);

	if (Result != EMetaSoundBuilderResult::Succeeded || !PresetBuilder)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to create preset builder: %s"),
			*BuilderResultToString(Result)
		));
	}

	// Cache the builder
	ActiveBuilders.Add(PresetName, PresetBuilder);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("preset_name"), PresetName);
	Data->SetStringField(TEXT("source_name"), SourceName);
	Data->SetStringField(TEXT("path"), Path);
	Data->SetStringField(TEXT("message"), TEXT("Preset builder created. Use build_metasound to save."));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleDeleteMetaSound(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName) || SoundName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	// Try to find and delete the asset
	FString AssetPath;
	UMetaSoundSource* Source = LoadMetaSoundSource(SoundName, AssetPath);
	if (!Source)
	{
		return CreateErrorResponse(FString::Printf(TEXT("MetaSound not found: %s"), *SoundName));
	}

	// Delete the asset
	bool bDeleted = UEditorAssetLibrary::DeleteAsset(AssetPath);
	if (!bDeleted)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to delete MetaSound: %s"), *SoundName));
	}

	// Remove from cache if present
	ActiveBuilders.Remove(SoundName);
	NodeHandleCache.Remove(SoundName);
	GraphInputOutputCache.Remove(SoundName);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("deleted"), SoundName);
	Data->SetStringField(TEXT("path"), AssetPath);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetMetaSoundInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName) || SoundName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString AssetPath;
	UMetaSoundSource* Source = LoadMetaSoundSource(SoundName, AssetPath);
	if (!Source)
	{
		return CreateErrorResponse(FString::Printf(TEXT("MetaSound not found: %s"), *SoundName));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("name"), Source->GetName());
	Data->SetStringField(TEXT("path"), AssetPath);
	Data->SetStringField(TEXT("class"), Source->GetClass()->GetName());

	// Get duration if available
	Data->SetNumberField(TEXT("duration"), Source->GetDuration());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleListMetaSoundAssets(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game");

	bool bRecursive = Params->HasField(TEXT("recursive"))
		? Params->GetBoolField(TEXT("recursive"))
		: true;

	// Query asset registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(UMetaSoundSource::StaticClass()->GetClassPathName(), AssetDataList, bRecursive);

	// Filter by path
	TArray<TSharedPtr<FJsonValue>> AssetsArray;
	for (const FAssetData& AssetData : AssetDataList)
	{
		FString AssetPath = AssetData.GetSoftObjectPath().ToString();
		if (AssetPath.StartsWith(Path))
		{
			TSharedPtr<FJsonObject> AssetInfo = MakeShareable(new FJsonObject);
			AssetInfo->SetStringField(TEXT("name"), AssetData.AssetName.ToString());
			AssetInfo->SetStringField(TEXT("path"), AssetPath);
			AssetsArray.Add(MakeShareable(new FJsonValueObject(AssetInfo)));
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("assets"), AssetsArray);
	Data->SetNumberField(TEXT("count"), AssetsArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleOpenMetaSound(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName) || SoundName.IsEmpty())
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString BuilderName = Params->HasField(TEXT("builder_name"))
		? Params->GetStringField(TEXT("builder_name"))
		: SoundName;

	// Load the existing MetaSound asset
	FString AssetPath;
	UMetaSoundSource* SourceAsset = LoadMetaSoundSource(SoundName, AssetPath);
	if (!SourceAsset)
	{
		return CreateErrorResponse(FString::Printf(TEXT("MetaSound not found: %s"), *SoundName));
	}

	// Get MetaSound Editor Subsystem to create builder from existing asset
	UMetaSoundEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaSoundEditorSubsystem>();
	if (!EditorSubsystem)
	{
		return CreateErrorResponse(TEXT("Failed to get MetaSoundEditorSubsystem"));
	}

	// Create a builder from the existing document
	EMetaSoundBuilderResult Result;
	TScriptInterface<IMetaSoundDocumentInterface> SourceInterface = SourceAsset;

#if ENGINE_MINOR_VERSION >= 4
	UMetaSoundBuilderBase* BuilderBase = EditorSubsystem->FindOrBeginBuilding(SourceInterface, Result);
	if (Result != EMetaSoundBuilderResult::Succeeded || !BuilderBase)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to create builder from MetaSound: %s"),
			*SoundName
		));
	}
#else
	(void)Result;
	return CreateErrorResponse(TEXT("MetaSound editing via builder requires UE 5.4+"));
	UMetaSoundBuilderBase* BuilderBase = nullptr;
#endif

	// Cast to source builder
	UMetaSoundSourceBuilder* Builder = Cast<UMetaSoundSourceBuilder>(BuilderBase);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("MetaSound is not a source: %s"),
			*SoundName
		));
	}

	// Cache the builder
	ActiveBuilders.Add(BuilderName, Builder);

	// Populate node and input caches from the existing document
	PopulateCachesFromBuilder(BuilderName, Builder);

	// Get node count for reporting
	int32 NodeCount = NodeHandleCache.Contains(BuilderName) ? NodeHandleCache[BuilderName].Num() : 0;
	int32 InputCount = GraphInputOutputCache.Contains(BuilderName) ? GraphInputOutputCache[BuilderName].Num() : 0;

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("builder_name"), BuilderName);
	Data->SetStringField(TEXT("asset_path"), AssetPath);
	Data->SetNumberField(TEXT("node_count"), NodeCount);
	Data->SetNumberField(TEXT("input_count"), InputCount);
	Data->SetStringField(TEXT("message"), TEXT("MetaSound opened for editing. Use add/connect commands, then build_metasound to save."));

	return CreateSuccessResponse(Data);
}

void FUnrealMCPMetaSoundCommands::PopulateCachesFromBuilder(
	const FString& SoundName,
	UMetaSoundSourceBuilder* Builder)
{
	if (!Builder)
	{
		return;
	}

	EMetaSoundBuilderResult Result;
	(void)Result; // Suppress C4101 when graph input caching is disabled for UE < 5.4

	// Clear existing caches for this sound
	NodeHandleCache.Remove(SoundName);
	GraphInputOutputCache.Remove(SoundName);

	// Initialize cache maps
	NodeHandleCache.Add(SoundName, TMap<FString, FMetaSoundNodeHandle>());
	GraphInputOutputCache.Add(SoundName, TMap<FString, FMetaSoundBuilderNodeOutputHandle>());

	// Cache graph inputs (UE 5.4+ only - GetGraphInputNames not available in 5.3)
#if ENGINE_MINOR_VERSION >= 4
	TArray<FName> InputNames = Builder->GetGraphInputNames(Result);
	for (const FName& InputName : InputNames)
	{
		// Skip the built-in OnPlay input
		if (InputName == TEXT("OnPlay"))
		{
			continue;
		}

		// Get the output handle for this graph input (so we can connect it to nodes)
		FName DataType;
		FMetaSoundBuilderNodeOutputHandle OutputHandle;
		Builder->FindGraphInputNode(InputName, DataType, OutputHandle, Result);
		if (Result == EMetaSoundBuilderResult::Succeeded)
		{
			GraphInputOutputCache[SoundName].Add(InputName.ToString(), OutputHandle);
		}
	}
#endif

	// Note: We cannot easily enumerate existing nodes from a builder,
	// as the Builder API doesn't expose a "GetAllNodes" method.
	// New nodes added via MCP will be cached, but existing nodes from the
	// document won't be accessible by name. Users can still add new nodes
	// and connect them to graph inputs/outputs.
}

//=============================================================================
// Graph I/O Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleAddMetaSoundInput(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString InputName;
	if (!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: input_name"));
	}

	FString DataType;
	if (!Params->TryGetStringField(TEXT("data_type"), DataType))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: data_type"));
	}

	bool bIsConstructor = Params->HasField(TEXT("is_constructor"))
		? Params->GetBoolField(TEXT("is_constructor"))
		: false;

	// Get or find builder
	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	// Create default value literal
	FMetasoundFrontendLiteral DefaultLiteral;
	if (Params->HasField(TEXT("default_value")))
	{
		DefaultLiteral = CreateLiteralFromJson(DataType, Params->TryGetField(TEXT("default_value")));
	}

	// Add graph input
	FMetaSoundBuilderNodeOutputHandle OutputHandle = Builder->AddGraphInputNode(
		FName(*InputName),
		FName(*DataType),
		DefaultLiteral,
		Result,
		bIsConstructor
	);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to add input: %s"),
			*BuilderResultToString(Result)
		));
	}

	// Cache the output handle so we can connect it to nodes later
	if (!GraphInputOutputCache.Contains(SoundName))
	{
		GraphInputOutputCache.Add(SoundName, TMap<FString, FMetaSoundBuilderNodeOutputHandle>());
	}
	GraphInputOutputCache[SoundName].Add(InputName, OutputHandle);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("input_name"), InputName);
	Data->SetStringField(TEXT("data_type"), DataType);
	Data->SetBoolField(TEXT("is_constructor"), bIsConstructor);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleAddMetaSoundOutput(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString OutputName;
	if (!Params->TryGetStringField(TEXT("output_name"), OutputName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: output_name"));
	}

	FString DataType;
	if (!Params->TryGetStringField(TEXT("data_type"), DataType))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: data_type"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	FMetasoundFrontendLiteral DefaultLiteral;
	FMetaSoundBuilderNodeInputHandle InputHandle = Builder->AddGraphOutputNode(
		FName(*OutputName),
		FName(*DataType),
		DefaultLiteral,
		Result,
		false // bIsConstructorOutput
	);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to add output: %s"),
			*BuilderResultToString(Result)
		));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("output_name"), OutputName);
	Data->SetStringField(TEXT("data_type"), DataType);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleRemoveMetaSoundInput(
	const TSharedPtr<FJsonObject>& Params)
{
	// TODO: Implement input removal
	return CreateErrorResponse(TEXT("remove_metasound_input not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleRemoveMetaSoundOutput(
	const TSharedPtr<FJsonObject>& Params)
{
	// TODO: Implement output removal
	return CreateErrorResponse(TEXT("remove_metasound_output not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetMetaSoundInputs(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

#if ENGINE_MINOR_VERSION >= 4
	TArray<FName> InputNames = Builder->GetGraphInputNames(Result);
#else
	return CreateErrorResponse(TEXT("Listing MetaSound inputs requires UE 5.4+"));
	TArray<FName> InputNames;
#endif

	TArray<TSharedPtr<FJsonValue>> InputsArray;
	for (const FName& Name : InputNames)
	{
		TSharedPtr<FJsonObject> InputInfo = MakeShareable(new FJsonObject);
		InputInfo->SetStringField(TEXT("name"), Name.ToString());
		InputsArray.Add(MakeShareable(new FJsonValueObject(InputInfo)));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("inputs"), InputsArray);
	Data->SetNumberField(TEXT("count"), InputsArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetMetaSoundOutputs(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

#if ENGINE_MINOR_VERSION >= 4
	TArray<FName> OutputNames = Builder->GetGraphOutputNames(Result);
#else
	return CreateErrorResponse(TEXT("Listing MetaSound outputs requires UE 5.4+"));
	TArray<FName> OutputNames;
#endif

	TArray<TSharedPtr<FJsonValue>> OutputsArray;
	for (const FName& Name : OutputNames)
	{
		TSharedPtr<FJsonObject> OutputInfo = MakeShareable(new FJsonObject);
		OutputInfo->SetStringField(TEXT("name"), Name.ToString());
		OutputsArray.Add(MakeShareable(new FJsonValueObject(OutputInfo)));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("outputs"), OutputsArray);
	Data->SetNumberField(TEXT("count"), OutputsArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleSetInputDefault(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString InputName;
	if (!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: input_name"));
	}

	if (!Params->HasField(TEXT("value")))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: value"));
	}

	FString DataType = Params->HasField(TEXT("data_type"))
		? Params->GetStringField(TEXT("data_type"))
		: TEXT("Float");

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	FMetasoundFrontendLiteral Literal = CreateLiteralFromJson(DataType, Params->TryGetField(TEXT("value")));
	Builder->SetGraphInputDefault(FName(*InputName), Literal, Result);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to set input default: %s"),
			*BuilderResultToString(Result)
		));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("input_name"), InputName);
	Data->SetStringField(TEXT("message"), TEXT("Default value set"));

	return CreateSuccessResponse(Data);
}

//=============================================================================
// Node Management Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleAddMetaSoundNode(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString NodeType;
	if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: node_type"));
	}

	FString NodeName = Params->HasField(TEXT("node_name"))
		? Params->GetStringField(TEXT("node_name"))
		: NodeType;

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	// Parse node class name
	FMetasoundFrontendClassName ClassName = ParseClassName(NodeType);

	// Add the node
#if ENGINE_MINOR_VERSION >= 4
	FMetaSoundNodeHandle NodeHandle = Builder->AddNodeByClassName(ClassName, Result);
#else
	FMetaSoundNodeHandle NodeHandle = Builder->AddNodeByClassName(ClassName, 1, Result);
#endif

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to add node '%s': %s"),
			*NodeType,
			*BuilderResultToString(Result)
		));
	}

	// Cache the node handle
	if (!NodeHandleCache.Contains(SoundName))
	{
		NodeHandleCache.Add(SoundName, TMap<FString, FMetaSoundNodeHandle>());
	}
	NodeHandleCache[SoundName].Add(NodeName, NodeHandle);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("node_name"), NodeName);
	Data->SetStringField(TEXT("node_type"), NodeType);
	Data->SetStringField(TEXT("node_id"), NodeName); // Use name as ID for now

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleDeleteMetaSoundNode(
	const TSharedPtr<FJsonObject>& Params)
{
	// TODO: Implement node deletion
	return CreateErrorResponse(TEXT("delete_metasound_node not yet implemented"));
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetMetaSoundNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	// Return cached nodes for this sound
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	if (NodeHandleCache.Contains(SoundName))
	{
		for (const auto& Pair : NodeHandleCache[SoundName])
		{
			TSharedPtr<FJsonObject> NodeInfo = MakeShareable(new FJsonObject);
			NodeInfo->SetStringField(TEXT("name"), Pair.Key);
			NodesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("nodes"), NodesArray);
	Data->SetNumberField(TEXT("count"), NodesArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleFindMetaSoundNode(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString NodeName;
	Params->TryGetStringField(TEXT("node_name"), NodeName);

	if (!NodeHandleCache.Contains(SoundName))
	{
		return CreateErrorResponse(FString::Printf(TEXT("No nodes cached for: %s"), *SoundName));
	}

	if (NodeHandleCache[SoundName].Contains(NodeName))
	{
		TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
		Data->SetStringField(TEXT("node_name"), NodeName);
		Data->SetBoolField(TEXT("found"), true);
		return CreateSuccessResponse(Data);
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetBoolField(TEXT("found"), false);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetNodeInputs(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: node_id"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	if (!NodeHandleCache.Contains(SoundName) || !NodeHandleCache[SoundName].Contains(NodeId))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	FMetaSoundNodeHandle NodeHandle = NodeHandleCache[SoundName][NodeId];
	TArray<FMetaSoundBuilderNodeInputHandle> InputHandles = Builder->FindNodeInputs(NodeHandle, Result);

	TArray<TSharedPtr<FJsonValue>> InputsArray;
	for (const FMetaSoundBuilderNodeInputHandle& InputHandle : InputHandles)
	{
		FName InputName;
		FName DataType;
		Builder->GetNodeInputData(InputHandle, InputName, DataType, Result);

		TSharedPtr<FJsonObject> InputInfo = MakeShareable(new FJsonObject);
		InputInfo->SetStringField(TEXT("name"), InputName.ToString());
		InputInfo->SetStringField(TEXT("data_type"), DataType.ToString());
		InputsArray.Add(MakeShareable(new FJsonValueObject(InputInfo)));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("inputs"), InputsArray);
	Data->SetNumberField(TEXT("count"), InputsArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetNodeOutputs(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: node_id"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	if (!NodeHandleCache.Contains(SoundName) || !NodeHandleCache[SoundName].Contains(NodeId))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	FMetaSoundNodeHandle NodeHandle = NodeHandleCache[SoundName][NodeId];
	TArray<FMetaSoundBuilderNodeOutputHandle> OutputHandles = Builder->FindNodeOutputs(NodeHandle, Result);

	TArray<TSharedPtr<FJsonValue>> OutputsArray;
	for (const FMetaSoundBuilderNodeOutputHandle& OutputHandle : OutputHandles)
	{
		FName OutputName;
		FName DataType;
		Builder->GetNodeOutputData(OutputHandle, OutputName, DataType, Result);

		TSharedPtr<FJsonObject> OutputInfo = MakeShareable(new FJsonObject);
		OutputInfo->SetStringField(TEXT("name"), OutputName.ToString());
		OutputInfo->SetStringField(TEXT("data_type"), DataType.ToString());
		OutputsArray.Add(MakeShareable(new FJsonValueObject(OutputInfo)));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("outputs"), OutputsArray);
	Data->SetNumberField(TEXT("count"), OutputsArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleSetNodeInputDefault(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString NodeId;
	if (!Params->TryGetStringField(TEXT("node_id"), NodeId))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: node_id"));
	}

	FString InputName;
	if (!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: input_name"));
	}

	if (!Params->HasField(TEXT("value")))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: value"));
	}

	FString DataType = Params->HasField(TEXT("data_type"))
		? Params->GetStringField(TEXT("data_type"))
		: TEXT("Float");

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	if (!NodeHandleCache.Contains(SoundName) || !NodeHandleCache[SoundName].Contains(NodeId))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeId));
	}

	FMetaSoundNodeHandle NodeHandle = NodeHandleCache[SoundName][NodeId];
	FMetaSoundBuilderNodeInputHandle InputHandle = Builder->FindNodeInputByName(NodeHandle, FName(*InputName), Result);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Input not found: %s"), *InputName));
	}

	FMetasoundFrontendLiteral Literal = CreateLiteralFromJson(DataType, Params->TryGetField(TEXT("value")));
	Builder->SetNodeInputDefault(InputHandle, Literal, Result);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to set node input default: %s"),
			*BuilderResultToString(Result)
		));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("node_id"), NodeId);
	Data->SetStringField(TEXT("input_name"), InputName);

	return CreateSuccessResponse(Data);
}

//=============================================================================
// Connection Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleConnectMetaSoundNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString FromNode;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNode))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: from_node"));
	}

	FString FromOutput;
	if (!Params->TryGetStringField(TEXT("from_output"), FromOutput))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: from_output"));
	}

	FString ToNode;
	if (!Params->TryGetStringField(TEXT("to_node"), ToNode))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: to_node"));
	}

	FString ToInput;
	if (!Params->TryGetStringField(TEXT("to_input"), ToInput))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: to_input"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	// Check if FromNode is a graph input (cached separately)
	bool bFromIsGraphInput = GraphInputOutputCache.Contains(SoundName) &&
	                         GraphInputOutputCache[SoundName].Contains(FromNode);

	FMetaSoundBuilderNodeOutputHandle OutputHandle;

	if (bFromIsGraphInput)
	{
		// FromNode is a graph input - use cached output handle directly
		// For graph inputs, FromOutput is ignored since they have a single output
		OutputHandle = GraphInputOutputCache[SoundName][FromNode];
	}
	else
	{
		// FromNode is a regular node - look up in node cache
		if (!NodeHandleCache.Contains(SoundName) || !NodeHandleCache[SoundName].Contains(FromNode))
		{
			return CreateErrorResponse(FString::Printf(TEXT("Source node not found: %s"), *FromNode));
		}

		FMetaSoundNodeHandle FromHandle = NodeHandleCache[SoundName][FromNode];
		OutputHandle = Builder->FindNodeOutputByName(FromHandle, FName(*FromOutput), Result);
		if (Result != EMetaSoundBuilderResult::Succeeded)
		{
			return CreateErrorResponse(FString::Printf(TEXT("Output pin not found: %s.%s"), *FromNode, *FromOutput));
		}
	}

	// Get destination node handle
	if (!NodeHandleCache.Contains(SoundName) || !NodeHandleCache[SoundName].Contains(ToNode))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Destination node not found: %s"), *ToNode));
	}

	FMetaSoundNodeHandle ToHandle = NodeHandleCache[SoundName][ToNode];
	FMetaSoundBuilderNodeInputHandle InputHandle = Builder->FindNodeInputByName(ToHandle, FName(*ToInput), Result);
	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Input pin not found: %s.%s"), *ToNode, *ToInput));
	}

	// Connect
	Builder->ConnectNodes(OutputHandle, InputHandle, Result);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to connect nodes: %s"),
			*BuilderResultToString(Result)
		));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("from"), FString::Printf(TEXT("%s.%s"), *FromNode, *FromOutput));
	Data->SetStringField(TEXT("to"), FString::Printf(TEXT("%s.%s"), *ToNode, *ToInput));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleDisconnectMetaSoundNodes(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString FromNode;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNode))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: from_node"));
	}

	FString FromOutput;
	if (!Params->TryGetStringField(TEXT("from_output"), FromOutput))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: from_output"));
	}

	FString ToNode;
	if (!Params->TryGetStringField(TEXT("to_node"), ToNode))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: to_node"));
	}

	FString ToInput;
	if (!Params->TryGetStringField(TEXT("to_input"), ToInput))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: to_input"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	if (!NodeHandleCache.Contains(SoundName) ||
		!NodeHandleCache[SoundName].Contains(FromNode) ||
		!NodeHandleCache[SoundName].Contains(ToNode))
	{
		return CreateErrorResponse(TEXT("Node not found in cache"));
	}

	FMetaSoundNodeHandle FromHandle = NodeHandleCache[SoundName][FromNode];
	FMetaSoundNodeHandle ToHandle = NodeHandleCache[SoundName][ToNode];

	FMetaSoundBuilderNodeOutputHandle OutputHandle = Builder->FindNodeOutputByName(FromHandle, FName(*FromOutput), Result);
	FMetaSoundBuilderNodeInputHandle InputHandle = Builder->FindNodeInputByName(ToHandle, FName(*ToInput), Result);

	Builder->DisconnectNodes(OutputHandle, InputHandle, Result);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to disconnect nodes: %s"),
			*BuilderResultToString(Result)
		));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("disconnected"), FString::Printf(TEXT("%s.%s -> %s.%s"), *FromNode, *FromOutput, *ToNode, *ToInput));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleConnectToGraphOutput(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString FromNode;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNode))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: from_node"));
	}

	FString FromOutput;
	if (!Params->TryGetStringField(TEXT("from_output"), FromOutput))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: from_output"));
	}

	FString GraphOutput;
	if (!Params->TryGetStringField(TEXT("graph_output"), GraphOutput))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: graph_output"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	if (!NodeHandleCache.Contains(SoundName) || !NodeHandleCache[SoundName].Contains(FromNode))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *FromNode));
	}

	FMetaSoundNodeHandle FromHandle = NodeHandleCache[SoundName][FromNode];
	FMetaSoundBuilderNodeOutputHandle OutputHandle = Builder->FindNodeOutputByName(FromHandle, FName(*FromOutput), Result);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Output pin not found: %s.%s"), *FromNode, *FromOutput));
	}

	// Connect to graph output
	Builder->ConnectNodeOutputToGraphOutput(FName(*GraphOutput), OutputHandle, Result);

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to connect to graph output: %s"),
			*BuilderResultToString(Result)
		));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("from"), FString::Printf(TEXT("%s.%s"), *FromNode, *FromOutput));
	Data->SetStringField(TEXT("to_graph_output"), GraphOutput);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetNodeConnections(
	const TSharedPtr<FJsonObject>& Params)
{
	// TODO: Implement connection query
	return CreateErrorResponse(TEXT("get_metasound_node_connections not yet implemented"));
}

//=============================================================================
// Variable Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleAddMetaSoundVariable(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: variable_name"));
	}

	FString DataType;
	if (!Params->TryGetStringField(TEXT("data_type"), DataType))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: data_type"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	FMetasoundFrontendLiteral DefaultLiteral;
	if (Params->HasField(TEXT("default_value")))
	{
		DefaultLiteral = CreateLiteralFromJson(DataType, Params->TryGetField(TEXT("default_value")));
	}

#if ENGINE_MINOR_VERSION >= 4
	Builder->AddGraphVariable(FName(*VariableName), FName(*DataType), DefaultLiteral, Result);
#else
	return CreateErrorResponse(TEXT("Graph variables require UE 5.4+"));
#endif

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to add variable: %s"),
			*BuilderResultToString(Result)
		));
	}

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("variable_name"), VariableName);
	Data->SetStringField(TEXT("data_type"), DataType);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleAddVariableGetter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: variable_name"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

#if ENGINE_MINOR_VERSION >= 4
	FMetaSoundNodeHandle GetterHandle = Builder->AddGraphVariableGetNode(FName(*VariableName), Result);
#else
	return CreateErrorResponse(TEXT("Graph variable getters require UE 5.4+"));
	FMetaSoundNodeHandle GetterHandle;
#endif

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to add variable getter: %s"),
			*BuilderResultToString(Result)
		));
	}

	// Cache the getter node
	FString NodeName = FString::Printf(TEXT("Get_%s"), *VariableName);
	if (!NodeHandleCache.Contains(SoundName))
	{
		NodeHandleCache.Add(SoundName, TMap<FString, FMetaSoundNodeHandle>());
	}
	NodeHandleCache[SoundName].Add(NodeName, GetterHandle);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("node_name"), NodeName);
	Data->SetStringField(TEXT("variable_name"), VariableName);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleAddVariableSetter(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString VariableName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VariableName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: variable_name"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

#if ENGINE_MINOR_VERSION >= 4
	FMetaSoundNodeHandle SetterHandle = Builder->AddGraphVariableSetNode(FName(*VariableName), Result);
#else
	return CreateErrorResponse(TEXT("Graph variable setters require UE 5.4+"));
	FMetaSoundNodeHandle SetterHandle;
#endif

	if (Result != EMetaSoundBuilderResult::Succeeded)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to add variable setter: %s"),
			*BuilderResultToString(Result)
		));
	}

	// Cache the setter node
	FString NodeName = FString::Printf(TEXT("Set_%s"), *VariableName);
	if (!NodeHandleCache.Contains(SoundName))
	{
		NodeHandleCache.Add(SoundName, TMap<FString, FMetaSoundNodeHandle>());
	}
	NodeHandleCache[SoundName].Add(NodeName, SetterHandle);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("node_name"), NodeName);
	Data->SetStringField(TEXT("variable_name"), VariableName);

	return CreateSuccessResponse(Data);
}

//=============================================================================
// Build Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleBuildMetaSound(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString AssetName = Params->HasField(TEXT("asset_name"))
		? Params->GetStringField(TEXT("asset_name"))
		: SoundName;

	FString Path = Params->HasField(TEXT("path"))
		? Params->GetStringField(TEXT("path"))
		: TEXT("/Game/MetaSounds");

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	// Get the MetaSound Editor Subsystem - this is the correct API for building to assets
	UMetaSoundEditorSubsystem* EditorSubsystem = GEditor->GetEditorSubsystem<UMetaSoundEditorSubsystem>();
	if (!EditorSubsystem)
	{
		return CreateErrorResponse(TEXT("MetaSoundEditorSubsystem not available"));
	}

	// Use BuildToAsset - the correct API for saving MetaSounds with all nodes and connections
	TScriptInterface<IMetaSoundDocumentInterface> BuiltAsset = EditorSubsystem->BuildToAsset(
		Builder,
		TEXT("MCP"),        // Author
		AssetName,          // Asset name
		Path,               // Package path
		Result,
		nullptr             // TemplateSoundWave (optional)
	);

	if (Result != EMetaSoundBuilderResult::Succeeded || !BuiltAsset.GetObject())
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Failed to build MetaSound to asset: %s"),
			*BuilderResultToString(Result)
		));
	}

	// Get the full asset path for reporting
	FString FullAssetPath = FString::Printf(TEXT("%s/%s"), *Path, *AssetName);

	// Save the asset
	bool bSaved = UEditorAssetLibrary::SaveAsset(FullAssetPath, false);
	if (!bSaved)
	{
		// Asset was built but save might have issues - still report success since asset exists
		UE_LOG(LogTemp, Warning, TEXT("MetaSound built but SaveAsset returned false for: %s"), *FullAssetPath);
	}

	// Clear builder cache
	ActiveBuilders.Remove(SoundName);
	NodeHandleCache.Remove(SoundName);
	GraphInputOutputCache.Remove(SoundName);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("asset_name"), AssetName);
	Data->SetStringField(TEXT("path"), FullAssetPath);
	Data->SetBoolField(TEXT("saved"), bSaved);
	Data->SetStringField(TEXT("message"), TEXT("MetaSound built and saved successfully"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleBuildAndOverwriteMetaSound(
	const TSharedPtr<FJsonObject>& Params)
{
	FString SoundName;
	if (!Params->TryGetStringField(TEXT("sound_name"), SoundName))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: sound_name"));
	}

	FString TargetAsset;
	if (!Params->TryGetStringField(TEXT("target_asset"), TargetAsset))
	{
		return CreateErrorResponse(TEXT("Missing required parameter: target_asset"));
	}

	EMetaSoundBuilderResult Result;
	UMetaSoundSourceBuilder* Builder = GetOrCreateBuilder(SoundName, Result);
	if (!Builder)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Builder not found for: %s"), *SoundName));
	}

	// Load target asset
	FString TargetPath;
	UMetaSoundSource* Target = LoadMetaSoundSource(TargetAsset, TargetPath);
	if (!Target)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Target asset not found: %s"), *TargetAsset));
	}

	// Build and overwrite
	TScriptInterface<IMetaSoundDocumentInterface> TargetInterface = Target;
#if ENGINE_MINOR_VERSION >= 4
	Builder->BuildAndOverwriteMetaSound(TargetInterface, false);
#else
	return CreateErrorResponse(TEXT("BuildAndOverwriteMetaSound requires UE 5.4+"));
#endif

	// Save the modified asset
	UEditorAssetLibrary::SaveAsset(TargetPath, false);

	// Clear builder cache
	ActiveBuilders.Remove(SoundName);
	NodeHandleCache.Remove(SoundName);
	GraphInputOutputCache.Remove(SoundName);

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetStringField(TEXT("target_asset"), TargetAsset);
	Data->SetStringField(TEXT("path"), TargetPath);
	Data->SetStringField(TEXT("message"), TEXT("MetaSound overwritten successfully"));

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleValidateMetaSound(
	const TSharedPtr<FJsonObject>& Params)
{
	// TODO: Implement validation
	return CreateErrorResponse(TEXT("validate_metasound not yet implemented"));
}

//=============================================================================
// Utility Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleListMetaSoundNodeTypes(
	const TSharedPtr<FJsonObject>& Params)
{
	// Common MetaSound node types
	TArray<TSharedPtr<FJsonValue>> NodeTypesArray;

	// Generators
	auto AddNodeType = [&NodeTypesArray](const FString& Name, const FString& Category, const FString& Description)
	{
		TSharedPtr<FJsonObject> NodeInfo = MakeShareable(new FJsonObject);
		NodeInfo->SetStringField(TEXT("name"), Name);
		NodeInfo->SetStringField(TEXT("category"), Category);
		NodeInfo->SetStringField(TEXT("description"), Description);
		NodeTypesArray.Add(MakeShareable(new FJsonValueObject(NodeInfo)));
	};

	// Oscillators (Namespace.Name.Variant format)
	AddNodeType(TEXT("UE.Sine.Audio"), TEXT("Oscillators"), TEXT("Sine wave oscillator"));
	AddNodeType(TEXT("UE.Saw.Audio"), TEXT("Oscillators"), TEXT("Sawtooth wave oscillator"));
	AddNodeType(TEXT("UE.Square.Audio"), TEXT("Oscillators"), TEXT("Square wave oscillator"));
	AddNodeType(TEXT("UE.Triangle.Audio"), TEXT("Oscillators"), TEXT("Triangle wave oscillator"));
	AddNodeType(TEXT("UE.Noise.Audio"), TEXT("Oscillators"), TEXT("White noise generator"));
	AddNodeType(TEXT("UE.LFO.Audio"), TEXT("Oscillators"), TEXT("Low frequency oscillator"));

	// Envelopes
	AddNodeType(TEXT("UE.AD Envelope.Audio"), TEXT("Envelopes"), TEXT("Attack-Decay envelope"));
	AddNodeType(TEXT("UE.ADSR Envelope.Audio"), TEXT("Envelopes"), TEXT("ADSR envelope generator"));

	// Filters
	AddNodeType(TEXT("UE.Biquad Filter.Audio"), TEXT("Filters"), TEXT("Biquad filter"));
	AddNodeType(TEXT("UE.One-Pole Low Pass Filter.Audio"), TEXT("Filters"), TEXT("One-pole lowpass filter"));
	AddNodeType(TEXT("UE.One-Pole High Pass Filter.Audio"), TEXT("Filters"), TEXT("One-pole highpass filter"));
	AddNodeType(TEXT("UE.State Variable Filter.Audio"), TEXT("Filters"), TEXT("State variable filter"));
	AddNodeType(TEXT("UE.Ladder Filter.Audio"), TEXT("Filters"), TEXT("Ladder filter"));

	// Math (Float variants)
	AddNodeType(TEXT("UE.Add.Float"), TEXT("Math"), TEXT("Add two float values"));
	AddNodeType(TEXT("UE.Subtract.Float"), TEXT("Math"), TEXT("Subtract two float values"));
	AddNodeType(TEXT("UE.Multiply.Float"), TEXT("Math"), TEXT("Multiply two float values"));
	AddNodeType(TEXT("UE.Divide.Float"), TEXT("Math"), TEXT("Divide two float values"));
	AddNodeType(TEXT("UE.Clamp.Float"), TEXT("Math"), TEXT("Clamp float to range"));
	AddNodeType(TEXT("UE.Lerp.Float"), TEXT("Math"), TEXT("Linear interpolation (float)"));
	AddNodeType(TEXT("UE.Map Range.Float"), TEXT("Math"), TEXT("Map value from one range to another"));

	// Math (Audio variants)
	AddNodeType(TEXT("UE.Add.Audio"), TEXT("Math"), TEXT("Add two audio signals"));
	AddNodeType(TEXT("UE.Multiply.Audio"), TEXT("Math"), TEXT("Multiply two audio signals"));
	AddNodeType(TEXT("UE.Multiply.Audio by Float"), TEXT("Math"), TEXT("Multiply audio by float (gain)"));

	// Audio Processing
	AddNodeType(TEXT("UE.Mono Delay.Audio"), TEXT("Audio"), TEXT("Mono delay effect"));
	AddNodeType(TEXT("UE.Stereo Delay.Audio"), TEXT("Audio"), TEXT("Stereo delay effect"));

	// Triggers
	AddNodeType(TEXT("UE.Trigger Repeat.Audio"), TEXT("Triggers"), TEXT("Repeat trigger at interval"));
	AddNodeType(TEXT("UE.Trigger Counter.Audio"), TEXT("Triggers"), TEXT("Count triggers"));

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("node_types"), NodeTypesArray);
	Data->SetNumberField(TEXT("count"), NodeTypesArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleGetMetaSoundDataTypes(
	const TSharedPtr<FJsonObject>& Params)
{
	TArray<TSharedPtr<FJsonValue>> DataTypesArray;

	auto AddDataType = [&DataTypesArray](const FString& Name, const FString& Description)
	{
		TSharedPtr<FJsonObject> TypeInfo = MakeShareable(new FJsonObject);
		TypeInfo->SetStringField(TEXT("name"), Name);
		TypeInfo->SetStringField(TEXT("description"), Description);
		DataTypesArray.Add(MakeShareable(new FJsonValueObject(TypeInfo)));
	};

	AddDataType(TEXT("Float"), TEXT("Single precision floating point"));
	AddDataType(TEXT("Int32"), TEXT("32-bit integer"));
	AddDataType(TEXT("Bool"), TEXT("Boolean true/false"));
	AddDataType(TEXT("String"), TEXT("Text string"));
	AddDataType(TEXT("Audio"), TEXT("Mono audio buffer"));
	AddDataType(TEXT("Time"), TEXT("Time value in seconds"));
	AddDataType(TEXT("Trigger"), TEXT("Trigger signal"));
	AddDataType(TEXT("WaveAsset"), TEXT("Reference to wave file"));

	TSharedPtr<FJsonObject> Data = MakeShareable(new FJsonObject);
	Data->SetArrayField(TEXT("data_types"), DataTypesArray);
	Data->SetNumberField(TEXT("count"), DataTypesArray.Num());

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::HandleAutoLayoutMetaSound(
	const TSharedPtr<FJsonObject>& Params)
{
	// TODO: Implement auto-layout
	return CreateErrorResponse(TEXT("auto_layout_metasound not yet implemented"));
}

//=============================================================================
// Helper Methods
//=============================================================================

UMetaSoundSourceBuilder* FUnrealMCPMetaSoundCommands::GetOrCreateBuilder(
	const FString& SoundName,
	EMetaSoundBuilderResult& OutResult)
{
	// Check cache first
	if (ActiveBuilders.Contains(SoundName))
	{
		UMetaSoundSourceBuilder* CachedBuilder = ActiveBuilders[SoundName].Get();
		if (CachedBuilder)
		{
			OutResult = EMetaSoundBuilderResult::Succeeded;
			return CachedBuilder;
		}
	}

	// Not found - return failure
	OutResult = EMetaSoundBuilderResult::Failed;
	return nullptr;
}

UMetaSoundBuilderBase* FUnrealMCPMetaSoundCommands::FindBuilder(const FString& SoundName)
{
	if (ActiveBuilders.Contains(SoundName))
	{
		return ActiveBuilders[SoundName].Get();
	}
	return nullptr;
}

UMetaSoundSource* FUnrealMCPMetaSoundCommands::LoadMetaSoundSource(
	const FString& SoundName,
	FString& OutPath)
{
	// Try direct path first
	if (SoundName.StartsWith(TEXT("/")))
	{
		OutPath = SoundName;
		return Cast<UMetaSoundSource>(StaticLoadObject(UMetaSoundSource::StaticClass(), nullptr, *SoundName));
	}

	// Search asset registry
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetDataList;
	AssetRegistry.GetAssetsByClass(UMetaSoundSource::StaticClass()->GetClassPathName(), AssetDataList, true);

	for (const FAssetData& AssetData : AssetDataList)
	{
		if (AssetData.AssetName.ToString() == SoundName)
		{
			OutPath = AssetData.GetSoftObjectPath().ToString();
			return Cast<UMetaSoundSource>(AssetData.GetAsset());
		}
	}

	return nullptr;
}

FMetasoundFrontendClassName FUnrealMCPMetaSoundCommands::ParseClassName(const FString& NodeType)
{
	// Parse format: "Namespace.Name.Variant" (e.g., "UE.Sine.Audio")
	// MetaSound nodes require all three parts: Namespace, Name, and Variant
	TArray<FString> Tokens;
	NodeType.ParseIntoArray(Tokens, TEXT("."));

	if (Tokens.Num() >= 3)
	{
		// Full format: Namespace.Name.Variant
		return FMetasoundFrontendClassName(FName(*Tokens[0]), FName(*Tokens[1]), FName(*Tokens[2]));
	}
	else if (Tokens.Num() == 2)
	{
		// Two parts: assume Namespace.Name with empty variant
		return FMetasoundFrontendClassName(FName(*Tokens[0]), FName(*Tokens[1]), FName());
	}
	else if (Tokens.Num() == 1)
	{
		// Single part: assume UE namespace with empty variant
		return FMetasoundFrontendClassName(FName(TEXT("UE")), FName(*Tokens[0]), FName());
	}

	// Invalid
	return FMetasoundFrontendClassName();
}

EMetaSoundOutputAudioFormat FUnrealMCPMetaSoundCommands::StringToOutputFormat(const FString& FormatStr)
{
	if (FormatStr.Equals(TEXT("Stereo"), ESearchCase::IgnoreCase))
	{
		return EMetaSoundOutputAudioFormat::Stereo;
	}
	else if (FormatStr.Equals(TEXT("Quad"), ESearchCase::IgnoreCase))
	{
		return EMetaSoundOutputAudioFormat::Quad;
	}
	else if (FormatStr.Equals(TEXT("FiveDotOne"), ESearchCase::IgnoreCase) ||
	         FormatStr.Equals(TEXT("5.1"), ESearchCase::IgnoreCase))
	{
		return EMetaSoundOutputAudioFormat::FiveDotOne;
	}

	return EMetaSoundOutputAudioFormat::Mono;
}

FMetasoundFrontendLiteral FUnrealMCPMetaSoundCommands::CreateLiteralFromJson(
	const FString& DataType,
	const TSharedPtr<FJsonValue>& Value)
{
	UMetaSoundBuilderSubsystem* Subsystem = GEngine->GetEngineSubsystem<UMetaSoundBuilderSubsystem>();
	if (!Subsystem)
	{
		return FMetasoundFrontendLiteral();
	}

	FName OutDataType;

	if (DataType.Equals(TEXT("Float"), ESearchCase::IgnoreCase))
	{
		float FloatValue = Value.IsValid() ? static_cast<float>(Value->AsNumber()) : 0.0f;
		return Subsystem->CreateFloatMetaSoundLiteral(FloatValue, OutDataType);
	}
	else if (DataType.Equals(TEXT("Int32"), ESearchCase::IgnoreCase))
	{
		int32 IntValue = Value.IsValid() ? static_cast<int32>(Value->AsNumber()) : 0;
		return Subsystem->CreateIntMetaSoundLiteral(IntValue, OutDataType);
	}
	else if (DataType.Equals(TEXT("Bool"), ESearchCase::IgnoreCase))
	{
		bool BoolValue = Value.IsValid() ? Value->AsBool() : false;
		return Subsystem->CreateBoolMetaSoundLiteral(BoolValue, OutDataType);
	}
	else if (DataType.Equals(TEXT("String"), ESearchCase::IgnoreCase))
	{
		FString StringValue = Value.IsValid() ? Value->AsString() : TEXT("");
		return Subsystem->CreateStringMetaSoundLiteral(StringValue, OutDataType);
	}

	return FMetasoundFrontendLiteral();
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), true);
	if (Data.IsValid())
	{
		for (const auto& Field : Data->Values)
		{
			Response->SetField(Field.Key, Field.Value);
		}
	}
	return Response;
}

TSharedPtr<FJsonObject> FUnrealMCPMetaSoundCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShareable(new FJsonObject);
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

FString FUnrealMCPMetaSoundCommands::BuilderResultToString(EMetaSoundBuilderResult Result)
{
	switch (Result)
	{
	case EMetaSoundBuilderResult::Succeeded:
		return TEXT("Succeeded");
	case EMetaSoundBuilderResult::Failed:
		return TEXT("Failed");
	default:
		return TEXT("Unknown result");
	}
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPMetaSoundCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// Asset Creation
	Registry.RegisterCommand(TEXT("create_metasound_source"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_metasound_source"), P); });
	Registry.RegisterCommand(TEXT("create_metasound_preset"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_metasound_preset"), P); });
	Registry.RegisterCommand(TEXT("delete_metasound"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_metasound"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_info"), P); });
	Registry.RegisterCommand(TEXT("list_metasound_assets"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_metasound_assets"), P); });
	Registry.RegisterCommand(TEXT("open_metasound"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("open_metasound"), P); });
	// Graph I/O
	Registry.RegisterCommand(TEXT("add_metasound_input"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_metasound_input"), P); });
	Registry.RegisterCommand(TEXT("add_metasound_output"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_metasound_output"), P); });
	Registry.RegisterCommand(TEXT("remove_metasound_input"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_metasound_input"), P); });
	Registry.RegisterCommand(TEXT("remove_metasound_output"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_metasound_output"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_inputs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_inputs"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_outputs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_outputs"), P); });
	Registry.RegisterCommand(TEXT("set_metasound_input_default"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_metasound_input_default"), P); });
	// Node Management
	Registry.RegisterCommand(TEXT("add_metasound_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_metasound_node"), P); });
	Registry.RegisterCommand(TEXT("delete_metasound_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_metasound_node"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_nodes"), P); });
	Registry.RegisterCommand(TEXT("find_metasound_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("find_metasound_node"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_node_inputs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_node_inputs"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_node_outputs"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_node_outputs"), P); });
	Registry.RegisterCommand(TEXT("set_metasound_node_input_default"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_metasound_node_input_default"), P); });
	// Connections
	Registry.RegisterCommand(TEXT("connect_metasound_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_metasound_nodes"), P); });
	Registry.RegisterCommand(TEXT("disconnect_metasound_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("disconnect_metasound_nodes"), P); });
	Registry.RegisterCommand(TEXT("connect_to_metasound_output"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_to_metasound_output"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_node_connections"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_node_connections"), P); });
	// Variables
	Registry.RegisterCommand(TEXT("add_metasound_variable"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_metasound_variable"), P); });
	Registry.RegisterCommand(TEXT("add_metasound_variable_getter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_metasound_variable_getter"), P); });
	Registry.RegisterCommand(TEXT("add_metasound_variable_setter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_metasound_variable_setter"), P); });
	// Build
	Registry.RegisterCommand(TEXT("build_metasound"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("build_metasound"), P); });
	Registry.RegisterCommand(TEXT("build_overwrite_metasound"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("build_overwrite_metasound"), P); });
	Registry.RegisterCommand(TEXT("validate_metasound"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("validate_metasound"), P); });
	// Utilities
	Registry.RegisterCommand(TEXT("list_metasound_node_types"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_metasound_node_types"), P); });
	Registry.RegisterCommand(TEXT("get_metasound_data_types"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_metasound_data_types"), P); });
	Registry.RegisterCommand(TEXT("auto_layout_metasound"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("auto_layout_metasound"), P); });
}
