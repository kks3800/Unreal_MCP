#include "Commands/UnrealMCPBlueprintSearchCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "BlueprintNodeSpawner.h"
#include "BlueprintActionDatabase.h"
#include "BlueprintFunctionNodeSpawner.h"
#include "BlueprintNodeBinder.h" // IBlueprintNodeBinder::FBindingSet
#include "Kismet2/BlueprintEditorUtils.h"
#include "K2Node_CallFunction.h"
#include "K2Node_Event.h"
#include "UObject/UObjectIterator.h"

//=============================================================================
// HandleCommand - Route to appropriate handler
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintSearchCommands::HandleCommand(
	const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("search_blueprint_actions"))
	{
		return HandleSearchBlueprintActions(Params);
	}
	else if (CommandType == TEXT("get_class_functions"))
	{
		return HandleGetClassFunctions(Params);
	}
	else if (CommandType == TEXT("get_node_type_info"))
	{
		return HandleGetNodeTypeInfo(Params);
	}
	else if (CommandType == TEXT("search_by_category"))
	{
		return HandleSearchByCategory(Params);
	}
	else if (CommandType == TEXT("place_searched_action"))
	{
		return HandlePlaceSearchedAction(Params);
	}
	else if (CommandType == TEXT("search_and_place_action"))
	{
		return HandleSearchAndPlaceAction(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Unknown blueprint search command: %s"), *CommandType));
}

//=============================================================================
// HandleSearchBlueprintActions
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintSearchCommands::HandleSearchBlueprintActions(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Keyword;
	if (!Params->TryGetStringField(TEXT("keyword"), Keyword))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'keyword' parameter"));
	}

	int32 Limit = 50;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = FMath::Clamp(static_cast<int32>(Params->GetNumberField(TEXT("limit"))), 1, 200);
	}

	int32 Offset = 0;
	if (Params->HasField(TEXT("offset")))
	{
		Offset = FMath::Max(0, static_cast<int32>(Params->GetNumberField(TEXT("offset"))));
	}

	FString ClassFilter;
	Params->TryGetStringField(TEXT("class_filter"), ClassFilter);

	FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();
	const FBlueprintActionDatabase::FActionRegistry& AllActions = ActionDB.GetAllActions();

	TArray<TSharedPtr<FJsonValue>> Results;
	int32 TotalMatches = 0;

	for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Pair : AllActions)
	{
		const FObjectKey& Key = Pair.Key;
		const FBlueprintActionDatabase::FActionList& SpawnerList = Pair.Value;

		for (int32 SpawnerIdx = 0; SpawnerIdx < SpawnerList.Num(); ++SpawnerIdx)
		{
			UBlueprintNodeSpawner* Spawner = SpawnerList[SpawnerIdx];
			if (!Spawner)
			{
				continue;
			}

			// Get display info
			const FBlueprintActionUiSpec& UiSpec = Spawner->PrimeDefaultUiSpec();

			FString ActionName = UiSpec.MenuName.ToString();
			FString Category = UiSpec.Category.ToString();
			FString Keywords = UiSpec.Keywords.ToString();

			// Skip empty entries
			if (ActionName.IsEmpty())
			{
				continue;
			}

			// Class filter
			if (!ClassFilter.IsEmpty())
			{
				UObject* OwnerObj = Key.ResolveObjectPtr();
				if (OwnerObj)
				{
					FString OwnerName = OwnerObj->GetName();
					if (!OwnerName.Contains(ClassFilter, ESearchCase::IgnoreCase))
					{
						continue;
					}
				}
				else
				{
					continue;
				}
			}

			// Keyword filter
			bool bMatches = ActionName.Contains(Keyword, ESearchCase::IgnoreCase) ||
				Category.Contains(Keyword, ESearchCase::IgnoreCase) ||
				Keywords.Contains(Keyword, ESearchCase::IgnoreCase);

			if (bMatches)
			{
				if (TotalMatches >= Offset && Results.Num() < Limit)
				{
					TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
					ResultObj->SetStringField(TEXT("action_name"), ActionName);
					ResultObj->SetStringField(TEXT("category"), Category);
					ResultObj->SetStringField(TEXT("keywords"), Keywords);
					ResultObj->SetStringField(TEXT("node_class"),
						Spawner->NodeClass ? Spawner->NodeClass->GetName() : TEXT("Unknown"));

					// Create an action key for place_searched_action
					FString OwnerPath;
					UObject* OwnerObj = Key.ResolveObjectPtr();
					if (OwnerObj)
					{
						OwnerPath = OwnerObj->GetPathName();
					}
					ResultObj->SetStringField(TEXT("owner_path"), OwnerPath);
					ResultObj->SetNumberField(TEXT("spawner_index"), SpawnerIdx);

					Results.Add(MakeShared<FJsonValueObject>(ResultObj));
				}
				TotalMatches++;
			}
		}
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetArrayField(TEXT("results"), Results);
	Data->SetNumberField(TEXT("total_matches"), TotalMatches);
	Data->SetNumberField(TEXT("offset"), Offset);
	Data->SetNumberField(TEXT("limit"), Limit);
	Data->SetNumberField(TEXT("returned"), Results.Num());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetClassFunctions
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintSearchCommands::HandleGetClassFunctions(
	const TSharedPtr<FJsonObject>& Params)
{
	FString ClassName;
	if (!Params->TryGetStringField(TEXT("class_name"), ClassName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'class_name' parameter"));
	}

	bool bIncludeInherited = true;
	if (Params->HasField(TEXT("include_inherited")))
	{
		bIncludeInherited = Params->GetBoolField(TEXT("include_inherited"));
	}

	// Try to find the UClass with various name formats
	UClass* TargetClass = FindObject<UClass>(nullptr, *ClassName);
	if (!TargetClass)
	{
		TargetClass = FindObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + ClassName));
	}
	if (!TargetClass)
	{
		TargetClass = FindObject<UClass>(nullptr, *(TEXT("/Script/CoreUObject.") + ClassName));
	}
	if (!TargetClass)
	{
		// Try with common prefixes
		for (const FString& Prefix : {TEXT("U"), TEXT("A"), TEXT("F")})
		{
			TargetClass = FindObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + Prefix + ClassName));
			if (TargetClass)
			{
				break;
			}
		}
	}
	if (!TargetClass)
	{
		// Try loading it
		TargetClass = LoadObject<UClass>(nullptr, *(TEXT("/Script/Engine.") + ClassName));
	}
	if (!TargetClass)
	{
		// Search all loaded classes by name
		for (TObjectIterator<UClass> It; It; ++It)
		{
			FString Name = It->GetName();
			if (Name.Equals(ClassName, ESearchCase::IgnoreCase) ||
				Name.Equals(TEXT("U") + ClassName, ESearchCase::IgnoreCase) ||
				Name.Equals(TEXT("A") + ClassName, ESearchCase::IgnoreCase))
			{
				TargetClass = *It;
				break;
			}
		}
	}

	if (!TargetClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Class not found: %s"), *ClassName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	TArray<TSharedPtr<FJsonValue>> FunctionsArray;

	EFieldIteratorFlags::SuperClassFlags SuperFlag = bIncludeInherited
		? EFieldIteratorFlags::IncludeSuper
		: EFieldIteratorFlags::ExcludeSuper;

	for (TFieldIterator<UFunction> FuncIt(TargetClass, SuperFlag); FuncIt; ++FuncIt)
	{
		UFunction* Func = *FuncIt;
		if (!Func->HasAnyFunctionFlags(FUNC_BlueprintCallable | FUNC_BlueprintPure))
		{
			continue;
		}
		if (Func->HasAnyFunctionFlags(FUNC_EditorOnly))
		{
			continue;
		}

		TSharedPtr<FJsonObject> FuncObj = MakeShared<FJsonObject>();
		FuncObj->SetStringField(TEXT("name"), Func->GetName());
		FuncObj->SetBoolField(TEXT("is_static"), Func->HasAnyFunctionFlags(FUNC_Static));
		FuncObj->SetBoolField(TEXT("is_pure"), Func->HasAnyFunctionFlags(FUNC_BlueprintPure));
		FuncObj->SetBoolField(TEXT("is_const"), Func->HasAnyFunctionFlags(FUNC_Const));

		// Category from metadata
		FString FuncCategory = Func->GetMetaData(TEXT("Category"));
		FuncObj->SetStringField(TEXT("category"), FuncCategory);

		// Owning class
		UClass* OwnerClass = Func->GetOwnerClass();
		if (OwnerClass)
		{
			FuncObj->SetStringField(TEXT("owning_class"), OwnerClass->GetName());
		}

		// Parameters
		TArray<TSharedPtr<FJsonValue>> ParamsArr;
		for (TFieldIterator<FProperty> PropIt(Func); PropIt; ++PropIt)
		{
			FProperty* Prop = *PropIt;
			if (Prop->HasAnyPropertyFlags(CPF_Parm))
			{
				TSharedPtr<FJsonObject> ParamObj = MakeShared<FJsonObject>();
				ParamObj->SetStringField(TEXT("name"), Prop->GetName());
				ParamObj->SetStringField(TEXT("type"), Prop->GetCPPType());
				ParamObj->SetBoolField(TEXT("is_return"), Prop->HasAnyPropertyFlags(CPF_ReturnParm));
				ParamObj->SetBoolField(TEXT("is_output"),
					Prop->HasAnyPropertyFlags(CPF_OutParm) && !Prop->HasAnyPropertyFlags(CPF_ReturnParm));
				ParamsArr.Add(MakeShared<FJsonValueObject>(ParamObj));
			}
		}
		FuncObj->SetArrayField(TEXT("parameters"), ParamsArr);

		FunctionsArray.Add(MakeShared<FJsonValueObject>(FuncObj));
	}

	Data->SetStringField(TEXT("class_name"), TargetClass->GetName());
	Data->SetStringField(TEXT("class_path"), TargetClass->GetPathName());
	Data->SetArrayField(TEXT("functions"), FunctionsArray);
	Data->SetNumberField(TEXT("function_count"), FunctionsArray.Num());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleGetNodeTypeInfo
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintSearchCommands::HandleGetNodeTypeInfo(
	const TSharedPtr<FJsonObject>& Params)
{
	FString NodeClassName;
	if (!Params->TryGetStringField(TEXT("node_class_name"), NodeClassName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'node_class_name' parameter"));
	}

	// Find the UClass for the node type
	UClass* NodeClass = nullptr;
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->GetName().Equals(NodeClassName, ESearchCase::IgnoreCase) ||
			It->GetName().Equals(TEXT("U") + NodeClassName, ESearchCase::IgnoreCase))
		{
			if (It->IsChildOf(UEdGraphNode::StaticClass()))
			{
				NodeClass = *It;
				break;
			}
		}
	}

	if (!NodeClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Node class not found: %s"), *NodeClassName));
	}

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
	Data->SetStringField(TEXT("class_name"), NodeClass->GetName());
	Data->SetStringField(TEXT("class_path"), NodeClass->GetPathName());

	// Get parent class
	if (NodeClass->GetSuperClass())
	{
		Data->SetStringField(TEXT("parent_class"), NodeClass->GetSuperClass()->GetName());
	}

	// Description from class metadata
	FString Description = NodeClass->GetMetaData(TEXT("ToolTip"));
	if (Description.IsEmpty())
	{
		Description = NodeClass->GetMetaData(TEXT("ShortToolTip"));
	}
	Data->SetStringField(TEXT("description"), Description);

	// Create a temporary CDO to inspect default pins
	UEdGraphNode* DefaultNode = NodeClass->GetDefaultObject<UEdGraphNode>();
	if (DefaultNode)
	{
		Data->SetStringField(TEXT("node_title"),
			DefaultNode->GetNodeTitle(ENodeTitleType::FullTitle).ToString());

		// Try to get default pins from the node
		TArray<TSharedPtr<FJsonValue>> PinsArray;
		for (UEdGraphPin* Pin : DefaultNode->Pins)
		{
			TSharedPtr<FJsonObject> PinObj = FUnrealMCPCommonUtils::PinToJson(Pin, false);
			if (PinObj.IsValid())
			{
				PinsArray.Add(MakeShared<FJsonValueObject>(PinObj));
			}
		}
		Data->SetArrayField(TEXT("default_pins"), PinsArray);
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandleSearchByCategory
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintSearchCommands::HandleSearchByCategory(
	const TSharedPtr<FJsonObject>& Params)
{
	FString Category;
	Params->TryGetStringField(TEXT("category"), Category);

	int32 Limit = 100;
	if (Params->HasField(TEXT("limit")))
	{
		Limit = FMath::Clamp(static_cast<int32>(Params->GetNumberField(TEXT("limit"))), 1, 500);
	}

	FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();
	const FBlueprintActionDatabase::FActionRegistry& AllActions = ActionDB.GetAllActions();

	TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();

	if (Category.IsEmpty())
	{
		// Return top-level categories
		TSet<FString> UniqueCategories;

		for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Pair : AllActions)
		{
			for (UBlueprintNodeSpawner* Spawner : Pair.Value)
			{
				if (!Spawner)
				{
					continue;
				}

				const FBlueprintActionUiSpec& UiSpec = Spawner->PrimeDefaultUiSpec();
				FString Cat = UiSpec.Category.ToString();
				if (!Cat.IsEmpty())
				{
					// Extract top-level category
					FString TopLevel;
					if (Cat.Split(TEXT("|"), &TopLevel, nullptr))
					{
						UniqueCategories.Add(TopLevel.TrimStartAndEnd());
					}
					else
					{
						UniqueCategories.Add(Cat.TrimStartAndEnd());
					}
				}
			}
		}

		TArray<TSharedPtr<FJsonValue>> CategoriesArray;
		TArray<FString> SortedCategories = UniqueCategories.Array();
		SortedCategories.Sort();

		for (const FString& Cat : SortedCategories)
		{
			CategoriesArray.Add(MakeShared<FJsonValueString>(Cat));
		}

		Data->SetArrayField(TEXT("categories"), CategoriesArray);
		Data->SetNumberField(TEXT("category_count"), CategoriesArray.Num());
	}
	else
	{
		// Return actions in the specified category
		TArray<TSharedPtr<FJsonValue>> Results;

		for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Pair : AllActions)
		{
			if (Results.Num() >= Limit)
			{
				break;
			}

			const FObjectKey& Key = Pair.Key;
			const FBlueprintActionDatabase::FActionList& SpawnerList = Pair.Value;

			for (int32 SpawnerIdx = 0; SpawnerIdx < SpawnerList.Num(); ++SpawnerIdx)
			{
				if (Results.Num() >= Limit)
				{
					break;
				}

				UBlueprintNodeSpawner* Spawner = SpawnerList[SpawnerIdx];
				if (!Spawner)
				{
					continue;
				}

				const FBlueprintActionUiSpec& UiSpec = Spawner->PrimeDefaultUiSpec();
				FString ActionCategory = UiSpec.Category.ToString();
				FString ActionName = UiSpec.MenuName.ToString();

				if (ActionName.IsEmpty())
				{
					continue;
				}

				// Check if this action's category starts with the requested category
				if (ActionCategory.StartsWith(Category, ESearchCase::IgnoreCase))
				{
					TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
					ResultObj->SetStringField(TEXT("action_name"), ActionName);
					ResultObj->SetStringField(TEXT("category"), ActionCategory);
					ResultObj->SetStringField(TEXT("keywords"), UiSpec.Keywords.ToString());
					ResultObj->SetStringField(TEXT("node_class"),
						Spawner->NodeClass ? Spawner->NodeClass->GetName() : TEXT("Unknown"));

					FString OwnerPath;
					UObject* OwnerObj = Key.ResolveObjectPtr();
					if (OwnerObj)
					{
						OwnerPath = OwnerObj->GetPathName();
					}
					ResultObj->SetStringField(TEXT("owner_path"), OwnerPath);
					ResultObj->SetNumberField(TEXT("spawner_index"), SpawnerIdx);

					Results.Add(MakeShared<FJsonValueObject>(ResultObj));
				}
			}
		}

		Data->SetArrayField(TEXT("results"), Results);
		Data->SetNumberField(TEXT("result_count"), Results.Num());
		Data->SetStringField(TEXT("category"), Category);
	}

	return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

//=============================================================================
// HandlePlaceSearchedAction
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintSearchCommands::HandlePlaceSearchedAction(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString OwnerPath;
	if (!Params->TryGetStringField(TEXT("owner_path"), OwnerPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'owner_path' parameter"));
	}

	if (!Params->HasField(TEXT("spawner_index")))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'spawner_index' parameter"));
	}
	int32 SpawnerIndex = static_cast<int32>(Params->GetNumberField(TEXT("spawner_index")));

	// Optional graph name, defaults to EventGraph
	FString GraphName = TEXT("EventGraph");
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	// Position
	float PosX = 0.0f;
	float PosY = 0.0f;
	if (Params->HasField(TEXT("position")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
		{
			PosX = static_cast<float>((*PosArray)[0]->AsNumber());
			PosY = static_cast<float>((*PosArray)[1]->AsNumber());
		}
	}

	// Find blueprint and graph
	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	// Find the spawner from the action database
	FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();

	UObject* OwnerObject = StaticFindObject(UObject::StaticClass(), nullptr, *OwnerPath);
	if (!OwnerObject)
	{
		OwnerObject = LoadObject<UObject>(nullptr, *OwnerPath);
	}

	if (!OwnerObject)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Owner object not found at path: %s"), *OwnerPath));
	}

	FObjectKey Key(OwnerObject);
	const FBlueprintActionDatabase::FActionRegistry& AllActions = ActionDB.GetAllActions();

	const FBlueprintActionDatabase::FActionList* SpawnerListPtr = AllActions.Find(Key);
	if (!SpawnerListPtr)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("No actions found for owner: %s"), *OwnerPath));
	}

	if (!SpawnerListPtr->IsValidIndex(SpawnerIndex))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Spawner index %d out of range (found %d spawners)"),
				SpawnerIndex, SpawnerListPtr->Num()));
	}

	UBlueprintNodeSpawner* Spawner = (*SpawnerListPtr)[SpawnerIndex];
	if (!Spawner)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Spawner is null"));
	}

	UEdGraphNode* NewNode = Spawner->Invoke(Graph, UBlueprintNodeSpawner::FBindingSet(), FVector2D(PosX, PosY));
	if (!NewNode)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create node from spawner"));
	}

	if (!FMCPBlueprintContext::Get().IsEditing()) { FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint); }

	TSharedPtr<FJsonObject> NodeJson = FUnrealMCPCommonUtils::NodeToJson(NewNode, true);
	if (!NodeJson.IsValid())
	{
		NodeJson = MakeShared<FJsonObject>();
	}
	NodeJson->SetStringField(TEXT("node_guid"), NewNode->NodeGuid.ToString());

	return FUnrealMCPCommonUtils::CreateSuccessResponse(NodeJson);
}

// ============================================================================
// HandleSearchAndPlaceAction — search + place in one step
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintSearchCommands::HandleSearchAndPlaceAction(
	const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString SearchKeyword;
	if (!Params->TryGetStringField(TEXT("search_keyword"), SearchKeyword))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'search_keyword' parameter"));
	}

	int32 SearchIndex = 0;
	if (Params->HasField(TEXT("search_index")))
	{
		SearchIndex = static_cast<int32>(Params->GetNumberField(TEXT("search_index")));
	}

	FString ClassFilter;
	Params->TryGetStringField(TEXT("class_filter"), ClassFilter);

	FString GraphName = TEXT("EventGraph");
	Params->TryGetStringField(TEXT("graph_name"), GraphName);

	float PosX = 0.0f;
	float PosY = 0.0f;
	if (Params->HasField(TEXT("position")))
	{
		const TArray<TSharedPtr<FJsonValue>>* PosArray;
		if (Params->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
		{
			PosX = static_cast<float>((*PosArray)[0]->AsNumber());
			PosY = static_cast<float>((*PosArray)[1]->AsNumber());
		}
	}

	// Find blueprint and graph
	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph not found: %s"), *GraphName));
	}

	// Search the action database for a match
	FBlueprintActionDatabase& ActionDB = FBlueprintActionDatabase::Get();
	const FBlueprintActionDatabase::FActionRegistry& AllActions = ActionDB.GetAllActions();

	int32 MatchCount = 0;
	UBlueprintNodeSpawner* FoundSpawner = nullptr;
	FString FoundActionName;

	for (const TPair<FObjectKey, FBlueprintActionDatabase::FActionList>& Pair : AllActions)
	{
		const FBlueprintActionDatabase::FActionList& SpawnerList = Pair.Value;

		for (int32 SpawnerIdx = 0; SpawnerIdx < SpawnerList.Num(); ++SpawnerIdx)
		{
			UBlueprintNodeSpawner* Spawner = SpawnerList[SpawnerIdx];
			if (!Spawner) continue;

			const FBlueprintActionUiSpec& UiSpec = Spawner->PrimeDefaultUiSpec();
			FString ActionName = UiSpec.MenuName.ToString();
			FString Category = UiSpec.Category.ToString();
			FString Keywords = UiSpec.Keywords.ToString();

			if (ActionName.IsEmpty()) continue;

			// Class filter
			if (!ClassFilter.IsEmpty())
			{
				UObject* OwnerObj = Pair.Key.ResolveObjectPtr();
				if (!OwnerObj || !OwnerObj->GetName().Contains(ClassFilter, ESearchCase::IgnoreCase))
				{
					continue;
				}
			}

			bool bMatches = ActionName.Contains(SearchKeyword, ESearchCase::IgnoreCase) ||
				Category.Contains(SearchKeyword, ESearchCase::IgnoreCase) ||
				Keywords.Contains(SearchKeyword, ESearchCase::IgnoreCase);

			if (bMatches)
			{
				if (MatchCount == SearchIndex)
				{
					FoundSpawner = Spawner;
					FoundActionName = ActionName;
					break;
				}
				MatchCount++;
			}
		}
		if (FoundSpawner) break;
	}

	if (!FoundSpawner)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("No action found for keyword '%s' at index %d (found %d matches)"),
				*SearchKeyword, SearchIndex, MatchCount));
	}

	// Place the node
	UEdGraphNode* NewNode = FoundSpawner->Invoke(Graph, UBlueprintNodeSpawner::FBindingSet(), FVector2D(PosX, PosY));
	if (!NewNode)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to create node from spawner for '%s'"), *FoundActionName));
	}

	if (!FMCPBlueprintContext::Get().IsEditing())
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	// Return in compact format (node_id at top level) for $N reference compatibility
	TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::NodeToCompactJson(NewNode);
	if (!ResultObj.IsValid())
	{
		ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("success"), true);
		ResultObj->SetStringField(TEXT("node_id"), NewNode->NodeGuid.ToString());
	}
	ResultObj->SetStringField(TEXT("matched_action"), FoundActionName);

	return ResultObj;
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPBlueprintSearchCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("search_blueprint_actions"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("search_blueprint_actions"), P); });
	Registry.RegisterCommand(TEXT("get_class_functions"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_class_functions"), P); });
	Registry.RegisterCommand(TEXT("get_node_type_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_node_type_info"), P); });
	Registry.RegisterCommand(TEXT("search_by_category"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("search_by_category"), P); });
	Registry.RegisterCommand(TEXT("place_searched_action"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("place_searched_action"), P); });
	Registry.RegisterCommand(TEXT("search_and_place_action"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("search_and_place_action"), P); });
}
