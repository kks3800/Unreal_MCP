// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPEQSCommands.h"
#include "MCPCore.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"
#include "Editor.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "Subsystems/AssetEditorSubsystem.h"

// EQS includes
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "EnvironmentQuery/EnvQueryContext.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "DataProviders/AIDataProvider.h"

// Dynamic class resolution via GetDerivedClasses - no hardcoded subclass includes needed
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "DataProviders/AIDataProvider.h"

//=============================================================================
// Command Dispatch
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleCommand(
	const FString& CommandType,
	const TSharedPtr<FJsonObject>& Params)
{
	// Asset Management
	if (CommandType == TEXT("create_eqs_query")) { return HandleCreateEQSQuery(Params); }
	if (CommandType == TEXT("delete_eqs_query")) { return HandleDeleteEQSQuery(Params); }
	if (CommandType == TEXT("list_eqs_queries")) { return HandleListEQSQueries(Params); }
	if (CommandType == TEXT("get_eqs_query_info")) { return HandleGetEQSQueryInfo(Params); }
	if (CommandType == TEXT("save_eqs_query")) { return HandleSaveEQSQuery(Params); }
	if (CommandType == TEXT("open_eqs_query")) { return HandleOpenEQSQuery(Params); }

	// Option / Generator
	if (CommandType == TEXT("add_eqs_generator")) { return HandleAddEQSGenerator(Params); }
	if (CommandType == TEXT("remove_eqs_option")) { return HandleRemoveEQSOption(Params); }
	if (CommandType == TEXT("get_eqs_options")) { return HandleGetEQSOptions(Params); }
	if (CommandType == TEXT("set_eqs_generator_property")) { return HandleSetEQSGeneratorProperty(Params); }

	// Tests
	if (CommandType == TEXT("add_eqs_test")) { return HandleAddEQSTest(Params); }
	if (CommandType == TEXT("remove_eqs_test")) { return HandleRemoveEQSTest(Params); }
	if (CommandType == TEXT("get_eqs_tests")) { return HandleGetEQSTests(Params); }
	if (CommandType == TEXT("set_eqs_test_property")) { return HandleSetEQSTestProperty(Params); }

	return CreateErrorResponse(FString::Printf(TEXT("Unknown EQS command: %s"), *CommandType));
}

//=============================================================================
// Helper Methods
//=============================================================================

UEnvQuery* FUnrealMCPEQSCommands::LoadEQSQuery(const FString& QueryName, FString& OutPath)
{
	// Check cache first
	if (TWeakObjectPtr<UEnvQuery>* Cached = ActiveQueries.Find(QueryName))
	{
		if (Cached->IsValid())
		{
			UEnvQuery* Query = Cached->Get();
			OutPath = Query->GetPathName();
			return Query;
		}
	}

	// Try direct path
	FString AssetPath = QueryName;
	if (!AssetPath.StartsWith(TEXT("/")))
	{
		AssetPath = FString::Printf(TEXT("/Game/AI/EQS/%s.%s"), *QueryName, *QueryName);
	}

	UEnvQuery* Query = Cast<UEnvQuery>(UEditorAssetLibrary::LoadAsset(AssetPath));
	if (!Query)
	{
		// Search asset registry
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
		IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

		TArray<FAssetData> AssetList;
		AssetRegistry.GetAssetsByClass(UEnvQuery::StaticClass()->GetClassPathName(), AssetList);

		for (const FAssetData& Asset : AssetList)
		{
			if (Asset.AssetName.ToString() == QueryName)
			{
				Query = Cast<UEnvQuery>(Asset.GetAsset());
				if (Query)
				{
					OutPath = Asset.GetObjectPathString();
					break;
				}
			}
		}
	}

	if (Query)
	{
		OutPath = Query->GetPathName();
		ActiveQueries.Add(QueryName, Query);
	}

	return Query;
}

/**
 * Dynamically resolve a UClass by short name from all subclasses of a base class.
 * Matches "Distance" to "EnvQueryTest_Distance", "MyTest" to "MyEQSTest_MyTest", etc.
 * Works with any project-specific subclass without hardcoding.
 */
static UClass* FindSubclassByShortName(UClass* BaseClass, const FString& ShortName)
{
	TArray<UClass*> DerivedClasses;
	GetDerivedClasses(BaseClass, DerivedClasses);

	// Exact class name match first
	for (UClass* Class : DerivedClasses)
	{
		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}
		if (Class->GetName() == ShortName)
		{
			return Class;
		}
	}

	// Suffix match: "Distance" matches "EnvQueryTest_Distance" or "UEnvQueryTest_Distance"
	for (UClass* Class : DerivedClasses)
	{
		if (Class->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}
		FString ClassName = Class->GetName();
		if (ClassName.EndsWith(FString(TEXT("_")) + ShortName))
		{
			return Class;
		}
	}

	return nullptr;
}

UClass* FUnrealMCPEQSCommands::ResolveGeneratorClass(const FString& GeneratorType)
{
	return FindSubclassByShortName(UEnvQueryGenerator::StaticClass(), GeneratorType);
}

UClass* FUnrealMCPEQSCommands::ResolveTestClass(const FString& TestType)
{
	return FindSubclassByShortName(UEnvQueryTest::StaticClass(), TestType);
}

UClass* FUnrealMCPEQSCommands::ResolveContextClass(const FString& ContextType)
{
	return FindSubclassByShortName(UEnvQueryContext::StaticClass(), ContextType);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::OptionToJson(UEnvQueryOption* Option, int32 Index)
{
	TSharedPtr<FJsonObject> OptionJson = MakeShared<FJsonObject>();
	OptionJson->SetNumberField(TEXT("index"), Index);

	if (Option->Generator)
	{
		OptionJson->SetStringField(TEXT("generator_class"), Option->Generator->GetClass()->GetName());
		OptionJson->SetStringField(TEXT("generator_name"), Option->Generator->OptionName);

		if (Option->Generator->ItemType)
		{
			OptionJson->SetStringField(TEXT("item_type"), Option->Generator->ItemType->GetName());
		}
	}

	OptionJson->SetNumberField(TEXT("test_count"), Option->Tests.Num());

	TArray<TSharedPtr<FJsonValue>> TestsArray;
	for (int32 TestIdx = 0; TestIdx < Option->Tests.Num(); ++TestIdx)
	{
		UEnvQueryTest* Test = Option->Tests[TestIdx];
		if (!Test)
		{
			continue;
		}

		TSharedPtr<FJsonObject> TestJson = MakeShared<FJsonObject>();
		TestJson->SetNumberField(TEXT("index"), TestIdx);
		TestJson->SetStringField(TEXT("class"), Test->GetClass()->GetName());
		TestJson->SetStringField(TEXT("purpose"), StaticEnum<EEnvTestPurpose::Type>()->GetNameStringByValue(static_cast<int64>(Test->TestPurpose)));
		TestJson->SetStringField(TEXT("comment"), Test->TestComment);
		TestsArray.Add(MakeShared<FJsonValueObject>(TestJson));
	}

	OptionJson->SetArrayField(TEXT("tests"), TestsArray);
	return OptionJson;
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
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

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::CreateErrorResponse(const FString& ErrorMessage)
{
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();
	Response->SetBoolField(TEXT("success"), false);
	Response->SetStringField(TEXT("error"), ErrorMessage);
	return Response;
}

//=============================================================================
// Asset Management Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleCreateEQSQuery(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	FString Path = TEXT("/Game/AI/EQS");
	Params->TryGetStringField(TEXT("path"), Path);

	FString PackagePath = FString::Printf(TEXT("%s/%s"), *Path, *QueryName);
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}

	UEnvQuery* NewQuery = NewObject<UEnvQuery>(Package, UEnvQuery::StaticClass(),
		FName(*QueryName), RF_Public | RF_Standalone);

	if (!NewQuery)
	{
		return CreateErrorResponse(TEXT("Failed to create EQS Query asset"));
	}

	FAssetRegistryModule::AssetCreated(NewQuery);
	NewQuery->MarkPackageDirty();

	ActiveQueries.Add(QueryName, NewQuery);

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("name"), QueryName);
	Data->SetStringField(TEXT("path"), PackagePath);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleDeleteEQSQuery(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	FString PackagePath = Query->GetOutermost()->GetName();
	FString ObjectPath = PackagePath + TEXT(".") + Query->GetName();

	ActiveQueries.Remove(QueryName);

	if (!UEditorAssetLibrary::DeleteAsset(ObjectPath))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to delete: %s"), *ObjectPath));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("deleted"), QueryName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleListEQSQueries(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Path = TEXT("/Game");
	Params->TryGetStringField(TEXT("path"), Path);
	bool bRecursive = true;
	Params->TryGetBoolField(TEXT("recursive"), bRecursive);

	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	TArray<FAssetData> AssetList;
	AssetRegistry.GetAssetsByClass(UEnvQuery::StaticClass()->GetClassPathName(), AssetList);

	TArray<TSharedPtr<FJsonValue>> QueryArray;
	for (const FAssetData& Asset : AssetList)
	{
		FString AssetPathStr = Asset.GetObjectPathString();
		if (!bRecursive)
		{
			FString AssetDir = FPackageName::GetLongPackagePath(Asset.PackageName.ToString());
			if (AssetDir != Path)
			{
				continue;
			}
		}
		else if (!AssetPathStr.StartsWith(Path))
		{
			continue;
		}

		TSharedPtr<FJsonObject> QueryInfo = MakeShared<FJsonObject>();
		QueryInfo->SetStringField(TEXT("name"), Asset.AssetName.ToString());
		QueryInfo->SetStringField(TEXT("path"), AssetPathStr);
		QueryArray.Add(MakeShared<FJsonValueObject>(QueryInfo));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetArrayField(TEXT("eqs_queries"), QueryArray);
	Data->SetNumberField(TEXT("count"), QueryArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleGetEQSQueryInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("name"), Query->GetName());
	Data->SetStringField(TEXT("path"), AssetPath);

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	Data->SetNumberField(TEXT("option_count"), Options.Num());

	TArray<TSharedPtr<FJsonValue>> OptionsArray;
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		if (Options[i])
		{
			OptionsArray.Add(MakeShared<FJsonValueObject>(OptionToJson(Options[i], i)));
		}
	}
	Data->SetArrayField(TEXT("options"), OptionsArray);

	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleSaveEQSQuery(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	UPackage* Package = Query->GetOutermost();
	FString PackageFileName = FPackageName::LongPackageNameToFilename(
		Package->GetName(), FPackageName::GetAssetPackageExtension());

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	bool bSaved = UPackage::SavePackage(Package, Query, *PackageFileName, SaveArgs);

	if (!bSaved)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Failed to save: %s"), *QueryName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("saved"), QueryName);
	Data->SetStringField(TEXT("file"), PackageFileName);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleOpenEQSQuery(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (AssetEditorSubsystem)
	{
		AssetEditorSubsystem->OpenEditorForAsset(Query);
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("opened"), QueryName);
	return CreateSuccessResponse(Data);
}

//=============================================================================
// Option / Generator Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleAddEQSGenerator(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	FString GeneratorType;
	if (!Params->TryGetStringField(TEXT("generator_type"), GeneratorType))
	{
		return CreateErrorResponse(TEXT("Missing 'generator_type'. Examples: SimpleGrid, PathingGrid, OnCircle, Donut, Cone, ActorsOfClass, CurrentLocation, Composite"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	UClass* GenClass = ResolveGeneratorClass(GeneratorType);
	if (!GenClass)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Unknown generator type: %s. Use the suffix after 'EnvQueryGenerator_' (e.g., SimpleGrid, ActorsOfClass) or any custom generator class name."),
			*GeneratorType));
	}

	// Create option and generator
	UEnvQueryOption* Option = NewObject<UEnvQueryOption>(Query);
	UEnvQueryGenerator* Generator = NewObject<UEnvQueryGenerator>(Option, GenClass);

	// Set option name if provided
	FString OptionName;
	if (Params->TryGetStringField(TEXT("option_name"), OptionName))
	{
		Generator->OptionName = OptionName;
	}

	// Set context if provided
	FString ContextType;
	if (Params->TryGetStringField(TEXT("context"), ContextType))
	{
		UClass* CtxClass = ResolveContextClass(ContextType);
		if (CtxClass)
		{
			// Set GenerateAround for grid/circle generators via property
			FProperty* GenAroundProp = GenClass->FindPropertyByName(TEXT("GenerateAround"));
			if (!GenAroundProp)
			{
				GenAroundProp = GenClass->FindPropertyByName(TEXT("SearchCenter"));
			}
			if (!GenAroundProp)
			{
				GenAroundProp = GenClass->FindPropertyByName(TEXT("CenterActor"));
			}

			if (GenAroundProp)
			{
				FClassProperty* ClassProp = CastField<FClassProperty>(GenAroundProp);
				if (ClassProp)
				{
					ClassProp->SetPropertyValue_InContainer(Generator, CtxClass);
				}
			}
		}
	}

	Option->Generator = Generator;
	Query->GetOptionsMutable().Add(Option);
	Query->MarkPackageDirty();

	int32 OptionIndex = Query->GetOptions().Num() - 1;

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("query"), QueryName);
	Data->SetStringField(TEXT("generator_type"), GeneratorType);
	Data->SetNumberField(TEXT("option_index"), OptionIndex);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleRemoveEQSOption(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	int32 OptionIndex = -1;
	if (!Params->TryGetNumberField(TEXT("option_index"), OptionIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'option_index' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	TArray<TObjectPtr<UEnvQueryOption>>& Options = Query->GetOptionsMutable();
	if (!Options.IsValidIndex(OptionIndex))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid option_index: %d (count: %d)"), OptionIndex, Options.Num()));
	}

	Options.RemoveAt(OptionIndex);
	Query->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("query"), QueryName);
	Data->SetNumberField(TEXT("removed_index"), OptionIndex);
	Data->SetNumberField(TEXT("remaining_options"), Options.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleGetEQSOptions(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();

	TArray<TSharedPtr<FJsonValue>> OptionsArray;
	for (int32 i = 0; i < Options.Num(); ++i)
	{
		if (Options[i])
		{
			OptionsArray.Add(MakeShared<FJsonValueObject>(OptionToJson(Options[i], i)));
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("query"), QueryName);
	Data->SetArrayField(TEXT("options"), OptionsArray);
	Data->SetNumberField(TEXT("count"), OptionsArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleSetEQSGeneratorProperty(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	int32 OptionIndex = -1;
	if (!Params->TryGetNumberField(TEXT("option_index"), OptionIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'option_index' parameter"));
	}

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	if (!Options.IsValidIndex(OptionIndex) || !Options[OptionIndex] || !Options[OptionIndex]->Generator)
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid option_index: %d or no generator"), OptionIndex));
	}

	UEnvQueryGenerator* Generator = Options[OptionIndex]->Generator;

	// Handle context class properties specially
	FString ContextValue;
	if (Params->TryGetStringField(TEXT("context_value"), ContextValue))
	{
		UClass* CtxClass = ResolveContextClass(ContextValue);
		if (CtxClass)
		{
			FProperty* Prop = Generator->GetClass()->FindPropertyByName(FName(*PropertyName));
			if (Prop)
			{
				FClassProperty* ClassProp = CastField<FClassProperty>(Prop);
				if (ClassProp)
				{
					ClassProp->SetPropertyValue_InContainer(Generator, CtxClass);
					Query->MarkPackageDirty();

					TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
					Data->SetStringField(TEXT("property"), PropertyName);
					Data->SetStringField(TEXT("value"), ContextValue);
					return CreateSuccessResponse(Data);
				}
			}
		}
	}

	// Handle float properties (FAIDataProviderFloatValue fields)
	double FloatValue = 0.0;
	if (Params->TryGetNumberField(TEXT("float_value"), FloatValue))
	{
		FProperty* Prop = Generator->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (Prop)
		{
			// Try as FAIDataProviderFloatValue
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (StructProp && StructProp->Struct->GetName() == TEXT("AIDataProviderFloatValue"))
			{
				void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(Generator);
				FAIDataProviderFloatValue* DataProvider = static_cast<FAIDataProviderFloatValue*>(ValuePtr);
				DataProvider->DefaultValue = static_cast<float>(FloatValue);
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetNumberField(TEXT("value"), FloatValue);
				return CreateSuccessResponse(Data);
			}

			// Try as plain float
			FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop);
			if (FloatProp)
			{
				FloatProp->SetPropertyValue_InContainer(Generator, static_cast<float>(FloatValue));
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetNumberField(TEXT("value"), FloatValue);
				return CreateSuccessResponse(Data);
			}

			// Try as double
			FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop);
			if (DoubleProp)
			{
				DoubleProp->SetPropertyValue_InContainer(Generator, FloatValue);
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetNumberField(TEXT("value"), FloatValue);
				return CreateSuccessResponse(Data);
			}
		}
	}

	// Handle bool properties (FAIDataProviderBoolValue fields)
	bool BoolValue = false;
	if (Params->TryGetBoolField(TEXT("bool_value"), BoolValue))
	{
		FProperty* Prop = Generator->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (Prop)
		{
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (StructProp && StructProp->Struct->GetName() == TEXT("AIDataProviderBoolValue"))
			{
				void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(Generator);
				FAIDataProviderBoolValue* DataProvider = static_cast<FAIDataProviderBoolValue*>(ValuePtr);
				DataProvider->DefaultValue = BoolValue;
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetBoolField(TEXT("value"), BoolValue);
				return CreateSuccessResponse(Data);
			}

			FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop);
			if (BoolProp)
			{
				BoolProp->SetPropertyValue_InContainer(Generator, BoolValue);
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetBoolField(TEXT("value"), BoolValue);
				return CreateSuccessResponse(Data);
			}
		}
	}

	// Handle int properties
	int32 IntValue = 0;
	if (Params->TryGetNumberField(TEXT("int_value"), IntValue))
	{
		FProperty* Prop = Generator->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (Prop)
		{
			FIntProperty* IntProp = CastField<FIntProperty>(Prop);
			if (IntProp)
			{
				IntProp->SetPropertyValue_InContainer(Generator, IntValue);
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetNumberField(TEXT("value"), IntValue);
				return CreateSuccessResponse(Data);
			}
		}
	}

	return CreateErrorResponse(FString::Printf(
		TEXT("Could not set property '%s'. Provide float_value, bool_value, int_value, or context_value."),
		*PropertyName));
}

//=============================================================================
// Test Commands
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleAddEQSTest(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	int32 OptionIndex = -1;
	if (!Params->TryGetNumberField(TEXT("option_index"), OptionIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'option_index' parameter"));
	}

	FString TestType;
	if (!Params->TryGetStringField(TEXT("test_type"), TestType))
	{
		return CreateErrorResponse(TEXT("Missing 'test_type'. Use the suffix after the class prefix (e.g., Distance, Dot, Trace, ObjectType, PawnAlive). Any registered UEnvQueryTest subclass works."));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	if (!Options.IsValidIndex(OptionIndex) || !Options[OptionIndex])
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid option_index: %d"), OptionIndex));
	}

	UClass* TestClass = ResolveTestClass(TestType);
	if (!TestClass)
	{
		return CreateErrorResponse(FString::Printf(
			TEXT("Unknown test type: %s. Use the suffix after the class prefix (e.g., Distance, Dot, Trace, ObjectType, PawnAlive). Any registered UEnvQueryTest subclass works."),
			*TestType));
	}

	UEnvQueryOption* Option = Options[OptionIndex];
	UEnvQueryTest* NewTest = NewObject<UEnvQueryTest>(Option, TestClass);

	// Set purpose if provided
	FString Purpose;
	if (Params->TryGetStringField(TEXT("purpose"), Purpose))
	{
		if (Purpose == TEXT("Filter"))
		{
			NewTest->TestPurpose = EEnvTestPurpose::Filter;
		}
		else if (Purpose == TEXT("Score"))
		{
			NewTest->TestPurpose = EEnvTestPurpose::Score;
		}
		else if (Purpose == TEXT("FilterAndScore"))
		{
			NewTest->TestPurpose = EEnvTestPurpose::FilterAndScore;
		}
	}

	// Set scoring equation if provided
	FString Scoring;
	if (Params->TryGetStringField(TEXT("scoring"), Scoring))
	{
		if (Scoring == TEXT("Linear")) { NewTest->ScoringEquation = EEnvTestScoreEquation::Linear; }
		else if (Scoring == TEXT("Square")) { NewTest->ScoringEquation = EEnvTestScoreEquation::Square; }
		else if (Scoring == TEXT("InverseLinear")) { NewTest->ScoringEquation = EEnvTestScoreEquation::InverseLinear; }
		else if (Scoring == TEXT("SquareRoot")) { NewTest->ScoringEquation = EEnvTestScoreEquation::SquareRoot; }
		else if (Scoring == TEXT("Constant")) { NewTest->ScoringEquation = EEnvTestScoreEquation::Constant; }
	}

	// Set scoring factor if provided
	double ScoringFactor = 0.0;
	if (Params->TryGetNumberField(TEXT("scoring_factor"), ScoringFactor))
	{
		NewTest->ScoringFactor.DefaultValue = static_cast<float>(ScoringFactor);
	}

	// Set comment if provided
	FString Comment;
	if (Params->TryGetStringField(TEXT("comment"), Comment))
	{
		NewTest->TestComment = Comment;
	}

	Option->Tests.Add(NewTest);
	Query->MarkPackageDirty();

	int32 TestIndex = Option->Tests.Num() - 1;

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("query"), QueryName);
	Data->SetNumberField(TEXT("option_index"), OptionIndex);
	Data->SetStringField(TEXT("test_type"), TestType);
	Data->SetNumberField(TEXT("test_index"), TestIndex);
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleRemoveEQSTest(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	int32 OptionIndex = -1;
	if (!Params->TryGetNumberField(TEXT("option_index"), OptionIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'option_index' parameter"));
	}

	int32 TestIndex = -1;
	if (!Params->TryGetNumberField(TEXT("test_index"), TestIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'test_index' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	if (!Options.IsValidIndex(OptionIndex) || !Options[OptionIndex])
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid option_index: %d"), OptionIndex));
	}

	UEnvQueryOption* Option = Options[OptionIndex];
	if (!Option->Tests.IsValidIndex(TestIndex))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid test_index: %d (count: %d)"), TestIndex, Option->Tests.Num()));
	}

	Option->Tests.RemoveAt(TestIndex);
	Query->MarkPackageDirty();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("query"), QueryName);
	Data->SetNumberField(TEXT("option_index"), OptionIndex);
	Data->SetNumberField(TEXT("removed_test_index"), TestIndex);
	Data->SetNumberField(TEXT("remaining_tests"), Option->Tests.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleGetEQSTests(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	int32 OptionIndex = -1;
	if (!Params->TryGetNumberField(TEXT("option_index"), OptionIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'option_index' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	if (!Options.IsValidIndex(OptionIndex) || !Options[OptionIndex])
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid option_index: %d"), OptionIndex));
	}

	UEnvQueryOption* Option = Options[OptionIndex];

	TArray<TSharedPtr<FJsonValue>> TestsArray;
	for (int32 i = 0; i < Option->Tests.Num(); ++i)
	{
		UEnvQueryTest* Test = Option->Tests[i];
		if (!Test)
		{
			continue;
		}

		TSharedPtr<FJsonObject> TestJson = MakeShared<FJsonObject>();
		TestJson->SetNumberField(TEXT("index"), i);
		TestJson->SetStringField(TEXT("class"), Test->GetClass()->GetName());
		TestJson->SetStringField(TEXT("purpose"), StaticEnum<EEnvTestPurpose::Type>()->GetNameStringByValue(static_cast<int64>(Test->TestPurpose)));
		TestJson->SetStringField(TEXT("filter_type"), StaticEnum<EEnvTestFilterType::Type>()->GetNameStringByValue(static_cast<int64>(Test->FilterType)));
		TestJson->SetStringField(TEXT("scoring_equation"), StaticEnum<EEnvTestScoreEquation::Type>()->GetNameStringByValue(static_cast<int64>(Test->ScoringEquation)));
		TestJson->SetNumberField(TEXT("scoring_factor"), Test->ScoringFactor.DefaultValue);
		TestJson->SetStringField(TEXT("comment"), Test->TestComment);
		TestsArray.Add(MakeShared<FJsonValueObject>(TestJson));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("query"), QueryName);
	Data->SetNumberField(TEXT("option_index"), OptionIndex);
	Data->SetArrayField(TEXT("tests"), TestsArray);
	Data->SetNumberField(TEXT("count"), TestsArray.Num());
	return CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEQSCommands::HandleSetEQSTestProperty(
	const TSharedPtr<FJsonObject>& Params)
{
	FString QueryName;
	if (!Params->TryGetStringField(TEXT("query_name"), QueryName))
	{
		return CreateErrorResponse(TEXT("Missing 'query_name' parameter"));
	}

	int32 OptionIndex = -1;
	if (!Params->TryGetNumberField(TEXT("option_index"), OptionIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'option_index' parameter"));
	}

	int32 TestIndex = -1;
	if (!Params->TryGetNumberField(TEXT("test_index"), TestIndex))
	{
		return CreateErrorResponse(TEXT("Missing 'test_index' parameter"));
	}

	FString PropertyName;
	if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
	{
		return CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
	}

	FString AssetPath;
	UEnvQuery* Query = LoadEQSQuery(QueryName, AssetPath);
	if (!Query)
	{
		return CreateErrorResponse(FString::Printf(TEXT("EQS Query not found: %s"), *QueryName));
	}

	const TArray<UEnvQueryOption*>& Options = Query->GetOptions();
	if (!Options.IsValidIndex(OptionIndex) || !Options[OptionIndex])
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid option_index: %d"), OptionIndex));
	}

	UEnvQueryOption* Option = Options[OptionIndex];
	if (!Option->Tests.IsValidIndex(TestIndex))
	{
		return CreateErrorResponse(FString::Printf(TEXT("Invalid test_index: %d"), TestIndex));
	}

	UEnvQueryTest* Test = Option->Tests[TestIndex];

	// Handle purpose specially
	FString PurposeValue;
	if (PropertyName == TEXT("TestPurpose") && Params->TryGetStringField(TEXT("enum_value"), PurposeValue))
	{
		if (PurposeValue == TEXT("Filter")) { Test->TestPurpose = EEnvTestPurpose::Filter; }
		else if (PurposeValue == TEXT("Score")) { Test->TestPurpose = EEnvTestPurpose::Score; }
		else if (PurposeValue == TEXT("FilterAndScore")) { Test->TestPurpose = EEnvTestPurpose::FilterAndScore; }
		Query->MarkPackageDirty();

		TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("property"), PropertyName);
		Data->SetStringField(TEXT("value"), PurposeValue);
		return CreateSuccessResponse(Data);
	}

	// Handle scoring equation specially
	FString ScoringValue;
	if (PropertyName == TEXT("ScoringEquation") && Params->TryGetStringField(TEXT("enum_value"), ScoringValue))
	{
		if (ScoringValue == TEXT("Linear")) { Test->ScoringEquation = EEnvTestScoreEquation::Linear; }
		else if (ScoringValue == TEXT("Square")) { Test->ScoringEquation = EEnvTestScoreEquation::Square; }
		else if (ScoringValue == TEXT("InverseLinear")) { Test->ScoringEquation = EEnvTestScoreEquation::InverseLinear; }
		else if (ScoringValue == TEXT("SquareRoot")) { Test->ScoringEquation = EEnvTestScoreEquation::SquareRoot; }
		else if (ScoringValue == TEXT("Constant")) { Test->ScoringEquation = EEnvTestScoreEquation::Constant; }
		Query->MarkPackageDirty();

		TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("property"), PropertyName);
		Data->SetStringField(TEXT("value"), ScoringValue);
		return CreateSuccessResponse(Data);
	}

	// Handle filter type
	FString FilterValue;
	if (PropertyName == TEXT("FilterType") && Params->TryGetStringField(TEXT("enum_value"), FilterValue))
	{
		if (FilterValue == TEXT("Minimum")) { Test->FilterType = EEnvTestFilterType::Minimum; }
		else if (FilterValue == TEXT("Maximum")) { Test->FilterType = EEnvTestFilterType::Maximum; }
		else if (FilterValue == TEXT("Range")) { Test->FilterType = EEnvTestFilterType::Range; }
		else if (FilterValue == TEXT("Match")) { Test->FilterType = EEnvTestFilterType::Match; }
		Query->MarkPackageDirty();

		TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
		Data->SetStringField(TEXT("property"), PropertyName);
		Data->SetStringField(TEXT("value"), FilterValue);
		return CreateSuccessResponse(Data);
	}

	// Handle context class properties
	FString ContextValue;
	if (Params->TryGetStringField(TEXT("context_value"), ContextValue))
	{
		UClass* CtxClass = ResolveContextClass(ContextValue);
		if (CtxClass)
		{
			FProperty* Prop = Test->GetClass()->FindPropertyByName(FName(*PropertyName));
			if (Prop)
			{
				FClassProperty* ClassProp = CastField<FClassProperty>(Prop);
				if (ClassProp)
				{
					ClassProp->SetPropertyValue_InContainer(Test, CtxClass);
					Query->MarkPackageDirty();

					TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
					Data->SetStringField(TEXT("property"), PropertyName);
					Data->SetStringField(TEXT("value"), ContextValue);
					return CreateSuccessResponse(Data);
				}
			}
		}
	}

	// Handle float properties (FAIDataProviderFloatValue)
	double FloatValue = 0.0;
	if (Params->TryGetNumberField(TEXT("float_value"), FloatValue))
	{
		FProperty* Prop = Test->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (Prop)
		{
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (StructProp && StructProp->Struct->GetName() == TEXT("AIDataProviderFloatValue"))
			{
				void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(Test);
				FAIDataProviderFloatValue* DataProvider = static_cast<FAIDataProviderFloatValue*>(ValuePtr);
				DataProvider->DefaultValue = static_cast<float>(FloatValue);
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetNumberField(TEXT("value"), FloatValue);
				return CreateSuccessResponse(Data);
			}

			FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop);
			if (FloatProp)
			{
				FloatProp->SetPropertyValue_InContainer(Test, static_cast<float>(FloatValue));
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetNumberField(TEXT("value"), FloatValue);
				return CreateSuccessResponse(Data);
			}
		}
	}

	// Handle bool properties (FAIDataProviderBoolValue)
	bool BoolValue = false;
	if (Params->TryGetBoolField(TEXT("bool_value"), BoolValue))
	{
		FProperty* Prop = Test->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (Prop)
		{
			FStructProperty* StructProp = CastField<FStructProperty>(Prop);
			if (StructProp && StructProp->Struct->GetName() == TEXT("AIDataProviderBoolValue"))
			{
				void* ValuePtr = StructProp->ContainerPtrToValuePtr<void>(Test);
				FAIDataProviderBoolValue* DataProvider = static_cast<FAIDataProviderBoolValue*>(ValuePtr);
				DataProvider->DefaultValue = BoolValue;
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetBoolField(TEXT("value"), BoolValue);
				return CreateSuccessResponse(Data);
			}

			FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop);
			if (BoolProp)
			{
				BoolProp->SetPropertyValue_InContainer(Test, BoolValue);
				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				Data->SetBoolField(TEXT("value"), BoolValue);
				return CreateSuccessResponse(Data);
			}
		}
	}

	// Handle enum values via string
	FString EnumValue;
	if (Params->TryGetStringField(TEXT("enum_value"), EnumValue))
	{
		FProperty* Prop = Test->GetClass()->FindPropertyByName(FName(*PropertyName));
		if (Prop)
		{
			FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop);
			FByteProperty* ByteProp = CastField<FByteProperty>(Prop);

			UEnum* EnumType = nullptr;
			if (EnumProp)
			{
				EnumType = EnumProp->GetEnum();
			}
			else if (ByteProp && ByteProp->Enum)
			{
				EnumType = ByteProp->Enum;
			}

			if (EnumType)
			{
				int64 Value = EnumType->GetValueByNameString(EnumValue);
				if (Value != INDEX_NONE)
				{
					if (EnumProp)
					{
						void* ValuePtr = EnumProp->ContainerPtrToValuePtr<void>(Test);
						EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, Value);
					}
					else if (ByteProp)
					{
						ByteProp->SetPropertyValue_InContainer(Test, static_cast<uint8>(Value));
					}

					Query->MarkPackageDirty();

					TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
					Data->SetStringField(TEXT("property"), PropertyName);
					Data->SetStringField(TEXT("value"), EnumValue);
					return CreateSuccessResponse(Data);
				}
			}
		}
	}

	// Handle array enum properties (e.g., TArray<EMyEnumType> AllowedTypes)
	const TArray<TSharedPtr<FJsonValue>>* ArrayValues = nullptr;
	if (Params->TryGetArrayField(TEXT("array_value"), ArrayValues) && ArrayValues)
	{
		FProperty* Prop = Test->GetClass()->FindPropertyByName(FName(*PropertyName));
		FArrayProperty* ArrayProp = Prop ? CastField<FArrayProperty>(Prop) : nullptr;
		if (ArrayProp)
		{
			// Determine inner enum type
			UEnum* InnerEnum = nullptr;
			FEnumProperty* InnerEnumProp = CastField<FEnumProperty>(ArrayProp->Inner);
			FByteProperty* InnerByteProp = CastField<FByteProperty>(ArrayProp->Inner);

			if (InnerEnumProp)
			{
				InnerEnum = InnerEnumProp->GetEnum();
			}
			else if (InnerByteProp && InnerByteProp->Enum)
			{
				InnerEnum = InnerByteProp->Enum;
			}

			if (InnerEnum)
			{
				FScriptArrayHelper ArrayHelper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Test));
				ArrayHelper.EmptyValues();

				TArray<FString> SetValues;
				for (const TSharedPtr<FJsonValue>& Val : *ArrayValues)
				{
					FString ElemStr = Val->AsString();
					int64 EnumVal = InnerEnum->GetValueByNameString(ElemStr);
					if (EnumVal != INDEX_NONE)
					{
						int32 Idx = ArrayHelper.AddValue();
						void* ElemPtr = ArrayHelper.GetRawPtr(Idx);
						if (InnerEnumProp)
						{
							InnerEnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ElemPtr, EnumVal);
						}
						else if (InnerByteProp)
						{
							*(uint8*)ElemPtr = static_cast<uint8>(EnumVal);
						}
						SetValues.Add(ElemStr);
					}
				}

				Query->MarkPackageDirty();

				TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
				Data->SetStringField(TEXT("property"), PropertyName);
				TArray<TSharedPtr<FJsonValue>> ResultArr;
				for (const FString& S : SetValues)
				{
					ResultArr.Add(MakeShared<FJsonValueString>(S));
				}
				Data->SetArrayField(TEXT("value"), ResultArr);
				return CreateSuccessResponse(Data);
			}
		}
	}

	return CreateErrorResponse(FString::Printf(
		TEXT("Could not set property '%s'. Provide float_value, bool_value, enum_value, context_value, or array_value."),
		*PropertyName));
}

//=============================================================================
// Command Registration
//=============================================================================

void FUnrealMCPEQSCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	// Asset Management
	Registry.RegisterCommand(TEXT("create_eqs_query"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_eqs_query"), P); });
	Registry.RegisterCommand(TEXT("delete_eqs_query"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_eqs_query"), P); });
	Registry.RegisterCommand(TEXT("list_eqs_queries"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_eqs_queries"), P); });
	Registry.RegisterCommand(TEXT("get_eqs_query_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_eqs_query_info"), P); });
	Registry.RegisterCommand(TEXT("save_eqs_query"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("save_eqs_query"), P); });
	Registry.RegisterCommand(TEXT("open_eqs_query"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("open_eqs_query"), P); });

	// Option / Generator
	Registry.RegisterCommand(TEXT("add_eqs_generator"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_eqs_generator"), P); });
	Registry.RegisterCommand(TEXT("remove_eqs_option"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_eqs_option"), P); });
	Registry.RegisterCommand(TEXT("get_eqs_options"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_eqs_options"), P); });
	Registry.RegisterCommand(TEXT("set_eqs_generator_property"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_eqs_generator_property"), P); });

	// Tests
	Registry.RegisterCommand(TEXT("add_eqs_test"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_eqs_test"), P); });
	Registry.RegisterCommand(TEXT("remove_eqs_test"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("remove_eqs_test"), P); });
	Registry.RegisterCommand(TEXT("get_eqs_tests"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_eqs_tests"), P); });
	Registry.RegisterCommand(TEXT("set_eqs_test_property"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_eqs_test_property"), P); });
}
