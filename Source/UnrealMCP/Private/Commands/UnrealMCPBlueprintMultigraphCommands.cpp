#include "Commands/UnrealMCPBlueprintMultigraphCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Engine/Blueprint.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_Tunnel.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_RemoveDelegate.h"
#include "UObject/Class.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"
#include "UObject/TopLevelAssetPath.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetRegistry/ARFilter.h"

namespace
{
	/**
	 * Resolve a short class name (e.g. "Actor", "StaticMeshComponent", "MyInterface")
	 * to a UClass* by scanning loaded classes and, as a last resort, the asset
	 * registry for Blueprint-generated classes.
	 */
	UClass* ResolveClassByName(const FString& ClassName)
	{
		if (ClassName.IsEmpty())
		{
			return nullptr;
		}

		// Direct path: "/Script/Engine.Actor" or "/Game/.../MyBP.MyBP_C"
		if (ClassName.StartsWith(TEXT("/")))
		{
			if (UClass* ClassFromPath = LoadObject<UClass>(nullptr, *ClassName))
			{
				return ClassFromPath;
			}
			// Also try as an asset path ending without _C
			FString WithC = ClassName;
			if (!WithC.EndsWith(TEXT("_C")))
			{
				WithC += TEXT("_C");
			}
			if (UClass* ClassFromPathC = LoadObject<UClass>(nullptr, *WithC))
			{
				return ClassFromPathC;
			}
		}

		// Try exact name (handles short names like "Actor", "Pawn")
		if (UClass* Direct = FindFirstObject<UClass>(*ClassName, EFindFirstObjectOptions::None))
		{
			return Direct;
		}

		// Try with "U" prefix (e.g. "ActorComponent" -> "UActorComponent")
		if (UClass* WithU = FindFirstObject<UClass>(*(FString(TEXT("U")) + ClassName), EFindFirstObjectOptions::None))
		{
			return WithU;
		}

		// Try with "A" prefix
		if (UClass* WithA = FindFirstObject<UClass>(*(FString(TEXT("A")) + ClassName), EFindFirstObjectOptions::None))
		{
			return WithA;
		}

		// Scan loaded classes for a matching name
		for (TObjectIterator<UClass> It; It; ++It)
		{
			if (It->GetName() == ClassName)
			{
				return *It;
			}
		}

		return nullptr;
	}

	/**
	 * Resolve a short struct name (e.g. "Vector", "Transform", "MyStruct") to a UScriptStruct*.
	 */
	UScriptStruct* ResolveStructByName(const FString& StructName)
	{
		if (StructName.IsEmpty())
		{
			return nullptr;
		}

		// Common engine shortcuts
		if (StructName == TEXT("Vector")) { return TBaseStructure<FVector>::Get(); }
		if (StructName == TEXT("Vector2D")) { return TBaseStructure<FVector2D>::Get(); }
		if (StructName == TEXT("Vector4")) { return TBaseStructure<FVector4>::Get(); }
		if (StructName == TEXT("Rotator")) { return TBaseStructure<FRotator>::Get(); }
		if (StructName == TEXT("Transform")) { return TBaseStructure<FTransform>::Get(); }
		if (StructName == TEXT("Color")) { return TBaseStructure<FColor>::Get(); }
		if (StructName == TEXT("LinearColor")) { return TBaseStructure<FLinearColor>::Get(); }

		if (UScriptStruct* Direct = FindFirstObject<UScriptStruct>(*StructName, EFindFirstObjectOptions::None))
		{
			return Direct;
		}

		// Try "F" prefix
		if (UScriptStruct* WithF = FindFirstObject<UScriptStruct>(*(FString(TEXT("F")) + StructName), EFindFirstObjectOptions::None))
		{
			return WithF;
		}

		return nullptr;
	}

	/**
	 * Map a user-facing type string to an FEdGraphPinType.
	 *
	 * Supported forms:
	 *   "Boolean" / "Integer" / "Int" / "Int64" / "Byte" / "Float" / "Double"
	 *   "String" / "Name" / "Text"
	 *   "Vector" / "Rotator" / "Transform" / "Color" / "LinearColor" / any FStruct short name
	 *   "Actor" / "Pawn" / "StaticMeshComponent" / any UObject subclass
	 *   "Object:ClassName"    -> hard object reference
	 *   "Class:ClassName"     -> TSubclassOf<ClassName>
	 *   "Interface:ClassName" -> Interface reference
	 *   "Struct:StructName"   -> explicit struct
	 *   "/Script/...", "/Game/..." paths also resolved.
	 */
	FEdGraphPinType ResolvePinType(const FString& TypeStr)
	{
		FEdGraphPinType PinType;

		// Primitives
		if (TypeStr == TEXT("Boolean") || TypeStr == TEXT("Bool"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
			return PinType;
		}
		if (TypeStr == TEXT("Integer") || TypeStr == TEXT("Int") || TypeStr == TEXT("Int32"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
			return PinType;
		}
		if (TypeStr == TEXT("Int64"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Int64;
			return PinType;
		}
		if (TypeStr == TEXT("Byte"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
			return PinType;
		}
		if (TypeStr == TEXT("Float"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Float;
			return PinType;
		}
		if (TypeStr == TEXT("Double"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
			PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
			return PinType;
		}
		if (TypeStr == TEXT("String"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_String;
			return PinType;
		}
		if (TypeStr == TEXT("Name"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
			return PinType;
		}
		if (TypeStr == TEXT("Text"))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
			return PinType;
		}

		// Prefixed forms
		FString Prefix, Remainder;
		if (TypeStr.Split(TEXT(":"), &Prefix, &Remainder))
		{
			if (Prefix == TEXT("Object"))
			{
				UClass* Cls = ResolveClassByName(Remainder);
				PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
				PinType.PinSubCategoryObject = Cls ? Cls : UObject::StaticClass();
				return PinType;
			}
			if (Prefix == TEXT("Class") || Prefix == TEXT("SubclassOf"))
			{
				UClass* Cls = ResolveClassByName(Remainder);
				PinType.PinCategory = UEdGraphSchema_K2::PC_Class;
				PinType.PinSubCategoryObject = Cls ? Cls : UObject::StaticClass();
				return PinType;
			}
			if (Prefix == TEXT("Interface"))
			{
				UClass* Cls = ResolveClassByName(Remainder);
				PinType.PinCategory = UEdGraphSchema_K2::PC_Interface;
				PinType.PinSubCategoryObject = Cls ? Cls : UObject::StaticClass();
				return PinType;
			}
			if (Prefix == TEXT("Struct"))
			{
				UScriptStruct* Struct = ResolveStructByName(Remainder);
				PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
				PinType.PinSubCategoryObject = Struct ? Struct : TBaseStructure<FVector>::Get();
				return PinType;
			}
		}

		// Known struct shortcuts
		if (UScriptStruct* KnownStruct = ResolveStructByName(TypeStr))
		{
			PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
			PinType.PinSubCategoryObject = KnownStruct;
			return PinType;
		}

		// Try as a class (Actor, Pawn, custom UObject)
		if (UClass* KnownClass = ResolveClassByName(TypeStr))
		{
			if (KnownClass->HasAnyClassFlags(CLASS_Interface))
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Interface;
				PinType.PinSubCategoryObject = KnownClass;
			}
			else
			{
				PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
				PinType.PinSubCategoryObject = KnownClass;
			}
			return PinType;
		}

		// Fallback: treat as double so the pin is at least usable
		PinType.PinCategory = UEdGraphSchema_K2::PC_Real;
		PinType.PinSubCategory = UEdGraphSchema_K2::PC_Double;
		return PinType;
	}

	/**
	 * Resolve an interface path/short-name to an FTopLevelAssetPath suitable for
	 * FBlueprintEditorUtils::ImplementNewInterface.
	 *
	 * Accepts:
	 *   "/Script/Engine.MyInterface"    -> used directly
	 *   "/Game/Path/MyBPI.MyBPI_C"      -> used directly
	 *   "MyInterface" / "UMyInterface"  -> resolved via UClass scan + asset registry
	 */
	FTopLevelAssetPath ResolveInterfacePath(const FString& Input)
	{
		if (Input.StartsWith(TEXT("/")))
		{
			// Full path form
			return FTopLevelAssetPath(Input);
		}

		// Short-name: try loaded UClasses first
		if (UClass* Cls = ResolveClassByName(Input))
		{
			if (Cls->HasAnyClassFlags(CLASS_Interface))
			{
				return FTopLevelAssetPath(Cls);
			}
		}

		// Asset registry fallback for Blueprint interfaces
		const IAssetRegistry& Registry = FAssetRegistryModule::GetRegistry();
		FARFilter Filter;
		Filter.ClassPaths.Add(FTopLevelAssetPath(TEXT("/Script/Engine"), TEXT("Blueprint")));
		Filter.bRecursiveClasses = true;
		TArray<FAssetData> Assets;
		Registry.GetAssets(Filter, Assets);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString() == Input)
			{
				// Point at the generated class path (MyBPI.MyBPI_C)
				FString Path = Asset.PackageName.ToString() + TEXT(".") + Asset.AssetName.ToString() + TEXT("_C");
				return FTopLevelAssetPath(Path);
			}
		}

		// Couldn't resolve — return the input as-is so ImplementNewInterface can try its own parse.
		return FTopLevelAssetPath(Input);
	}

	/**
	 * Set the PinDefaultValue / DefaultObject on an auto-created pin by name.
	 * Used after CreateUserDefinedPin to stamp a literal default value on
	 * function entry inputs.
	 */
	void ApplyDefaultValueToEntryPin(UK2Node_FunctionEntry* Entry, const FName PinName, const FString& DefaultValue)
	{
		if (!Entry || DefaultValue.IsEmpty())
		{
			return;
		}
		// Mirror to the UserDefinedPins list so it survives ReconstructNode
		for (TSharedPtr<FUserPinInfo> UDP : Entry->UserDefinedPins)
		{
			if (UDP.IsValid() && UDP->PinName == PinName)
			{
				UDP->PinDefaultValue = DefaultValue;
				break;
			}
		}
		// Apply to the live pin as well
		if (UEdGraphPin* Pin = Entry->FindPin(PinName))
		{
			Pin->DefaultValue = DefaultValue;
			Pin->AutogeneratedDefaultValue = DefaultValue;
		}
	}
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_function_graph_ex"))
	{
		return HandleCreateFunctionGraphEx(Params);
	}
	else if (CommandType == TEXT("create_macro_graph_ex"))
	{
		return HandleCreateMacroGraphEx(Params);
	}
	else if (CommandType == TEXT("create_event_dispatcher"))
	{
		return HandleCreateEventDispatcher(Params);
	}
	else if (CommandType == TEXT("implement_interface"))
	{
		return HandleImplementInterface(Params);
	}
	else if (CommandType == TEXT("add_local_variable_ex"))
	{
		return HandleAddLocalVariableEx(Params);
	}
	else if (CommandType == TEXT("add_bind_delegate_node"))
	{
		return HandleAddBindDelegateNode(Params);
	}
	else if (CommandType == TEXT("add_call_delegate_node"))
	{
		return HandleAddCallDelegateNode(Params);
	}
	else if (CommandType == TEXT("add_remove_delegate_node"))
	{
		return HandleAddRemoveDelegateNode(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(
		FString::Printf(TEXT("Unknown multigraph command: %s"), *CommandType));
}

//=============================================================================
// create_function_graph_ex
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleCreateFunctionGraphEx(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	if (FUnrealMCPCommonUtils::FindGraphByName(Blueprint, FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph with name '%s' already exists"), *FunctionName));
	}

	UEdGraph* FuncGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint, FName(*FunctionName),
		UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!FuncGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create function graph"));
	}

	FBlueprintEditorUtils::AddFunctionGraph<UClass>(Blueprint, FuncGraph, /*bIsUserCreated=*/true, /*SignatureFromObject=*/nullptr);

	// Find entry node (AddFunctionGraph creates it automatically)
	UK2Node_FunctionEntry* Entry = nullptr;
	{
		TArray<UK2Node_FunctionEntry*> EntryNodes;
		FuncGraph->GetNodesOfClass(EntryNodes);
		if (EntryNodes.Num() > 0)
		{
			Entry = EntryNodes[0];
		}
	}

	if (!Entry)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Function entry node not found after graph creation"));
	}

	// Add inputs as entry-node output pins (they flow OUT of entry, INTO the graph body).
	const TArray<TSharedPtr<FJsonValue>>* InputsArray = nullptr;
	if (Params->TryGetArrayField(TEXT("inputs"), InputsArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *InputsArray)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Value->TryGetObject(Obj))
			{
				continue;
			}
			FString ParamName;
			FString ParamType;
			(*Obj)->TryGetStringField(TEXT("name"), ParamName);
			(*Obj)->TryGetStringField(TEXT("type"), ParamType);
			if (ParamName.IsEmpty() || ParamType.IsEmpty())
			{
				continue;
			}

			FEdGraphPinType PinType = ResolvePinType(ParamType);
			Entry->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);

			FString DefaultValue;
			if ((*Obj)->TryGetStringField(TEXT("default_value"), DefaultValue))
			{
				ApplyDefaultValueToEntryPin(Entry, FName(*ParamName), DefaultValue);
			}
		}
	}

	// Add a Result node iff outputs exist
	UK2Node_FunctionResult* Result = nullptr;
	const TArray<TSharedPtr<FJsonValue>>* OutputsArray = nullptr;
	if (Params->TryGetArrayField(TEXT("outputs"), OutputsArray) && OutputsArray->Num() > 0)
	{
		FGraphNodeCreator<UK2Node_FunctionResult> ResultCreator(*FuncGraph);
		Result = ResultCreator.CreateNode();
		if (Result)
		{
			Result->FunctionReference = Entry->FunctionReference;
			Result->NodePosX = Entry->NodePosX + 600;
			Result->NodePosY = Entry->NodePosY;
			ResultCreator.Finalize();

			for (const TSharedPtr<FJsonValue>& Value : *OutputsArray)
			{
				const TSharedPtr<FJsonObject>* Obj = nullptr;
				if (!Value->TryGetObject(Obj))
				{
					continue;
				}
				FString ParamName;
				FString ParamType;
				(*Obj)->TryGetStringField(TEXT("name"), ParamName);
				(*Obj)->TryGetStringField(TEXT("type"), ParamType);
				if (ParamName.IsEmpty() || ParamType.IsEmpty())
				{
					continue;
				}

				FEdGraphPinType PinType = ResolvePinType(ParamType);
				Result->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Input);
			}
		}
	}

	// Flags
	bool bPure = false;
	Params->TryGetBoolField(TEXT("pure"), bPure);
	bool bConst = false;
	Params->TryGetBoolField(TEXT("const"), bConst);
	bool bCallInEditor = false;
	Params->TryGetBoolField(TEXT("call_in_editor"), bCallInEditor);
	FString Replication;
	Params->TryGetStringField(TEXT("replication"), Replication);

	if (bPure)
	{
		Entry->AddExtraFlags(FUNC_BlueprintPure);
	}
	if (bConst)
	{
		Entry->AddExtraFlags(FUNC_Const);
	}

	// Replication mapping:
	//   "none" (default), "multicast", "server", "client", "reliable_multicast"
	const FString ReplLower = Replication.ToLower();
	if (ReplLower == TEXT("multicast"))
	{
		Entry->AddExtraFlags(FUNC_NetMulticast);
	}
	else if (ReplLower == TEXT("reliable_multicast"))
	{
		Entry->AddExtraFlags(FUNC_NetMulticast | FUNC_NetReliable);
	}
	else if (ReplLower == TEXT("server"))
	{
		Entry->AddExtraFlags(FUNC_NetServer | FUNC_NetReliable);
	}
	else if (ReplLower == TEXT("client"))
	{
		Entry->AddExtraFlags(FUNC_NetClient | FUNC_NetReliable);
	}

	Entry->MetaData.bCallInEditor = bCallInEditor;

	// Reconstruct entry/result so pin lists reflect user-defined additions.
	Entry->ReconstructNode();
	if (Result)
	{
		Result->ReconstructNode();
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("graph_name"), FunctionName);
	ResultObj->SetStringField(TEXT("entry_node_guid"), Entry->NodeGuid.ToString());
	if (Result)
	{
		ResultObj->SetStringField(TEXT("result_node_guid"), Result->NodeGuid.ToString());
	}
	ResultObj->SetBoolField(TEXT("pure"), bPure);
	ResultObj->SetBoolField(TEXT("const"), bConst);
	ResultObj->SetBoolField(TEXT("call_in_editor"), bCallInEditor);
	ResultObj->SetStringField(TEXT("replication"), ReplLower.IsEmpty() ? TEXT("none") : ReplLower);
	ResultObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Function graph '%s' created with extended flags"), *FunctionName));
	return ResultObj;
}

//=============================================================================
// create_macro_graph_ex
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleCreateMacroGraphEx(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString MacroName;
	if (!Params->TryGetStringField(TEXT("macro_name"), MacroName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'macro_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	for (UEdGraph* Existing : Blueprint->MacroGraphs)
	{
		if (Existing && Existing->GetName() == MacroName)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Macro graph '%s' already exists"), *MacroName));
		}
	}

	UEdGraph* MacroGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint, FName(*MacroName),
		UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!MacroGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create macro graph"));
	}

	FBlueprintEditorUtils::AddMacroGraph(Blueprint, MacroGraph, /*bIsUserCreated=*/true, /*SignatureFromClass=*/(UClass*)nullptr);

	// Locate the two auto-created tunnel nodes. Entry has bCanHaveOutputs; exit has bCanHaveInputs.
	UK2Node_Tunnel* EntryTunnel = nullptr;
	UK2Node_Tunnel* ExitTunnel = nullptr;
	{
		TArray<UK2Node_Tunnel*> Tunnels;
		MacroGraph->GetNodesOfClass(Tunnels);
		for (UK2Node_Tunnel* Tunnel : Tunnels)
		{
			if (!Tunnel) { continue; }
			if (Tunnel->bCanHaveOutputs && !EntryTunnel) { EntryTunnel = Tunnel; }
			else if (Tunnel->bCanHaveInputs && !ExitTunnel) { ExitTunnel = Tunnel; }
		}
	}

	// Populate inputs on entry tunnel (EGPD_Output), outputs on exit tunnel (EGPD_Input).
	int32 InputCount = 0;
	const TArray<TSharedPtr<FJsonValue>>* InputsArray = nullptr;
	if (EntryTunnel && Params->TryGetArrayField(TEXT("inputs"), InputsArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *InputsArray)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Value->TryGetObject(Obj)) { continue; }
			FString ParamName;
			FString ParamType;
			(*Obj)->TryGetStringField(TEXT("name"), ParamName);
			(*Obj)->TryGetStringField(TEXT("type"), ParamType);
			if (ParamName.IsEmpty() || ParamType.IsEmpty()) { continue; }

			FEdGraphPinType PinType = ResolvePinType(ParamType);
			EntryTunnel->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
			++InputCount;
		}
		EntryTunnel->ReconstructNode();
	}

	int32 OutputCount = 0;
	const TArray<TSharedPtr<FJsonValue>>* OutputsArray = nullptr;
	if (ExitTunnel && Params->TryGetArrayField(TEXT("outputs"), OutputsArray))
	{
		for (const TSharedPtr<FJsonValue>& Value : *OutputsArray)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Value->TryGetObject(Obj)) { continue; }
			FString ParamName;
			FString ParamType;
			(*Obj)->TryGetStringField(TEXT("name"), ParamName);
			(*Obj)->TryGetStringField(TEXT("type"), ParamType);
			if (ParamName.IsEmpty() || ParamType.IsEmpty()) { continue; }

			FEdGraphPinType PinType = ResolvePinType(ParamType);
			ExitTunnel->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Input);
			++OutputCount;
		}
		ExitTunnel->ReconstructNode();
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("graph_name"), MacroName);
	ResultObj->SetNumberField(TEXT("input_count"), InputCount);
	ResultObj->SetNumberField(TEXT("output_count"), OutputCount);
	if (EntryTunnel)
	{
		ResultObj->SetStringField(TEXT("entry_tunnel_guid"), EntryTunnel->NodeGuid.ToString());
	}
	if (ExitTunnel)
	{
		ResultObj->SetStringField(TEXT("exit_tunnel_guid"), ExitTunnel->NodeGuid.ToString());
	}
	ResultObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Macro graph '%s' created with %d inputs and %d outputs"),
			*MacroName, InputCount, OutputCount));
	return ResultObj;
}

//=============================================================================
// create_event_dispatcher
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleCreateEventDispatcher(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString DispatcherName;
	if (!Params->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	FName DispatcherFName(*DispatcherName);

	// Reject if a member variable with the same name already exists.
	for (const FBPVariableDescription& Existing : Blueprint->NewVariables)
	{
		if (Existing.VarName == DispatcherFName)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Variable '%s' already exists on this Blueprint"), *DispatcherName));
		}
	}

	// 1. Multicast delegate member variable — PC_MCDelegate auto-flags
	//    CPF_BlueprintAssignable | CPF_BlueprintCallable.
	FEdGraphPinType DelegateType;
	DelegateType.PinCategory = UEdGraphSchema_K2::PC_MCDelegate;
	FBlueprintEditorUtils::AddMemberVariable(Blueprint, DispatcherFName, DelegateType);

	// 2. Signature graph — required so Kismet has a function-like skeleton
	//    to inspect when generating the delegate's UFunction at compile time.
	UEdGraph* SigGraph = FBlueprintEditorUtils::CreateNewGraph(
		Blueprint, DispatcherFName,
		UEdGraph::StaticClass(), UEdGraphSchema_K2::StaticClass());
	if (!SigGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create delegate signature graph"));
	}

	SigGraph->bEditable = false;

	UEdGraphSchema_K2* K2Schema = GetMutableDefault<UEdGraphSchema_K2>();
	K2Schema->CreateDefaultNodesForGraph(*SigGraph);
	K2Schema->CreateFunctionGraphTerminators(*SigGraph, static_cast<UClass*>(nullptr));
	K2Schema->AddExtraFunctionFlags(SigGraph, FUNC_BlueprintCallable | FUNC_BlueprintEvent | FUNC_Public);
	K2Schema->MarkFunctionEntryAsEditable(SigGraph, true);

	Blueprint->DelegateSignatureGraphs.Add(SigGraph);

	// 3. Populate signature parameters on the new entry node.
	int32 ParamCount = 0;
	UK2Node_FunctionEntry* EntryNode = nullptr;
	{
		TArray<UK2Node_FunctionEntry*> EntryNodes;
		SigGraph->GetNodesOfClass(EntryNodes);
		if (EntryNodes.Num() > 0)
		{
			EntryNode = EntryNodes[0];
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ParamsArray = nullptr;
	// Accept both "signature_params" (spec) and "parameters" (legacy) for compat.
	if (!Params->TryGetArrayField(TEXT("signature_params"), ParamsArray))
	{
		Params->TryGetArrayField(TEXT("parameters"), ParamsArray);
	}

	if (EntryNode && ParamsArray)
	{
		for (const TSharedPtr<FJsonValue>& Value : *ParamsArray)
		{
			const TSharedPtr<FJsonObject>* Obj = nullptr;
			if (!Value->TryGetObject(Obj)) { continue; }
			FString ParamName;
			FString ParamType;
			(*Obj)->TryGetStringField(TEXT("name"), ParamName);
			(*Obj)->TryGetStringField(TEXT("type"), ParamType);
			if (ParamName.IsEmpty() || ParamType.IsEmpty()) { continue; }

			FEdGraphPinType PinType = ResolvePinType(ParamType);
			EntryNode->CreateUserDefinedPin(FName(*ParamName), PinType, EGPD_Output);
			++ParamCount;
		}
		EntryNode->ReconstructNode();
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	// Regenerate skeleton so downstream Bind/Call/Remove nodes can SetFromProperty
	// against the freshly added multicast delegate without waiting for full compile.
	FKismetEditorUtilities::GenerateBlueprintSkeleton(Blueprint);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("dispatcher_name"), DispatcherName);
	ResultObj->SetNumberField(TEXT("parameter_count"), ParamCount);
	if (EntryNode)
	{
		ResultObj->SetStringField(TEXT("signature_entry_guid"), EntryNode->NodeGuid.ToString());
	}
	ResultObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Event dispatcher '%s' created with %d signature parameters"),
			*DispatcherName, ParamCount));
	return ResultObj;
}

//=============================================================================
// implement_interface
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleImplementInterface(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString InterfacePath;
	if (!Params->TryGetStringField(TEXT("interface_path"), InterfacePath))
	{
		// Accept "interface_name" as an alias for shorter calls.
		if (!Params->TryGetStringField(TEXT("interface_name"), InterfacePath))
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'interface_path' parameter"));
		}
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	FTopLevelAssetPath InterfaceAssetPath = ResolveInterfacePath(InterfacePath);
	if (!InterfaceAssetPath.IsValid())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Could not resolve interface: %s"), *InterfacePath));
	}

	const int32 GraphsBefore = Blueprint->FunctionGraphs.Num();
	const int32 InterfacesBefore = Blueprint->ImplementedInterfaces.Num();

	const bool bSuccess = FBlueprintEditorUtils::ImplementNewInterface(Blueprint, InterfaceAssetPath);
	if (!bSuccess)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to implement interface '%s' (already implemented or invalid?)"),
				*InterfaceAssetPath.ToString()));
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);

	// Re-run skeleton generation so the new interface functions get skeleton entries
	// and newly created graphs become addressable by name.
	FKismetEditorUtilities::GenerateBlueprintSkeleton(Blueprint);

	// Collect the names of graphs added by the interface implementation.
	TArray<TSharedPtr<FJsonValue>> NewGraphs;
	for (int32 Index = GraphsBefore; Index < Blueprint->FunctionGraphs.Num(); ++Index)
	{
		if (UEdGraph* NewGraph = Blueprint->FunctionGraphs[Index])
		{
			NewGraphs.Add(MakeShared<FJsonValueString>(NewGraph->GetName()));
		}
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("interface_path"), InterfaceAssetPath.ToString());
	ResultObj->SetNumberField(TEXT("interfaces_added"), Blueprint->ImplementedInterfaces.Num() - InterfacesBefore);
	ResultObj->SetNumberField(TEXT("graphs_added"), NewGraphs.Num());
	ResultObj->SetArrayField(TEXT("new_graph_names"), NewGraphs);
	ResultObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Interface '%s' implemented; %d new function graph(s) added"),
			*InterfaceAssetPath.ToString(), NewGraphs.Num()));
	return ResultObj;
}

//=============================================================================
// add_local_variable_ex
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleAddLocalVariableEx(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}

	FString FunctionGraphName;
	// Accept either "function_graph_name" (spec) or "function_name" (legacy).
	if (!Params->TryGetStringField(TEXT("function_graph_name"), FunctionGraphName))
	{
		if (!Params->TryGetStringField(TEXT("function_name"), FunctionGraphName))
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_graph_name' parameter"));
		}
	}

	FString VarName;
	if (!Params->TryGetStringField(TEXT("variable_name"), VarName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_name' parameter"));
	}

	FString VarType;
	if (!Params->TryGetStringField(TEXT("variable_type"), VarType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'variable_type' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* FuncGraph = FUnrealMCPCommonUtils::FindGraphByName(Blueprint, FunctionGraphName);
	if (!FuncGraph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Function graph not found: %s"), *FunctionGraphName));
	}

	// AddLocalVariable silently returns false on macro graphs; give a clear error.
	const UEdGraphSchema_K2* K2Schema = Cast<const UEdGraphSchema_K2>(FuncGraph->GetSchema());
	if (K2Schema && K2Schema->GetGraphType(FuncGraph) != GT_Function)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Graph '%s' is not a function graph; local variables only work on function graphs"),
				*FunctionGraphName));
	}

	FEdGraphPinType PinType = ResolvePinType(VarType);

	FString DefaultValue;
	Params->TryGetStringField(TEXT("default_value"), DefaultValue);

	const bool bOK = FBlueprintEditorUtils::AddLocalVariable(
		Blueprint, FuncGraph, FName(*VarName), PinType, DefaultValue);
	if (!bOK)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to add local variable '%s' to function '%s'"),
				*VarName, *FunctionGraphName));
	}

	if (!FMCPBlueprintContext::Get().IsEditing())
	{
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetStringField(TEXT("variable_name"), VarName);
	ResultObj->SetStringField(TEXT("variable_type"), VarType);
	ResultObj->SetStringField(TEXT("function_graph_name"), FunctionGraphName);
	ResultObj->SetStringField(TEXT("message"),
		FString::Printf(TEXT("Local variable '%s' (%s) added to function '%s'"),
			*VarName, *VarType, *FunctionGraphName));
	return ResultObj;
}

//=============================================================================
// Delegate node spawning helpers
//=============================================================================

namespace
{
	/** Look up a multicast delegate property on the blueprint, preferring skeleton class. */
	FMulticastDelegateProperty* FindDispatcherProperty(UBlueprint* Blueprint, const FName DispatcherName)
	{
		if (!Blueprint)
		{
			return nullptr;
		}

		if (Blueprint->SkeletonGeneratedClass)
		{
			if (FMulticastDelegateProperty* Prop =
					FindFProperty<FMulticastDelegateProperty>(Blueprint->SkeletonGeneratedClass, DispatcherName))
			{
				return Prop;
			}
		}
		if (Blueprint->GeneratedClass)
		{
			if (FMulticastDelegateProperty* Prop =
					FindFProperty<FMulticastDelegateProperty>(Blueprint->GeneratedClass, DispatcherName))
			{
				return Prop;
			}
		}
		return nullptr;
	}

	/**
	 * Resolve the target graph from common parameters.
	 * Uses "graph_name" if provided, otherwise falls back to EventGraph.
	 */
	UEdGraph* ResolveTargetGraph(UBlueprint* Blueprint, const TSharedPtr<FJsonObject>& Params)
	{
		FString GraphName;
		Params->TryGetStringField(TEXT("graph_name"), GraphName);
		if (GraphName.IsEmpty())
		{
			return FUnrealMCPCommonUtils::FindOrCreateEventGraph(Blueprint);
		}
		return FUnrealMCPCommonUtils::FindGraphByName(Blueprint, GraphName);
	}

	template<typename NodeType>
	NodeType* SpawnDelegateNode(UEdGraph* Graph, FMulticastDelegateProperty* DelegateProp,
	                             UClass* OwnerClass, const FVector2D& Position)
	{
		NodeType* Node = NewObject<NodeType>(Graph);
		if (DelegateProp)
		{
			Node->SetFromProperty(DelegateProp, /*bSelfContext=*/true, OwnerClass);
		}
		Node->CreateNewGuid();
		Node->NodePosX = Position.X;
		Node->NodePosY = Position.Y;
		Node->AllocateDefaultPins();
		Node->PostPlacedNewNode();
		Node->SetFlags(RF_Transactional);
		Graph->AddNode(Node, /*bFromUI=*/true, /*bSelectNewNode=*/false);
		return Node;
	}
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleAddBindDelegateNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}
	FString DispatcherName;
	if (!Params->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = ResolveTargetGraph(Blueprint, Params);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
	}

	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
	}

	FMulticastDelegateProperty* DelegateProp = FindDispatcherProperty(Blueprint, FName(*DispatcherName));
	UClass* OwnerClass = Blueprint->SkeletonGeneratedClass ? Blueprint->SkeletonGeneratedClass : Blueprint->GeneratedClass;

	UK2Node_AddDelegate* Node = SpawnDelegateNode<UK2Node_AddDelegate>(Graph, DelegateProp, OwnerClass, Position);

	if (!FMCPBlueprintContext::Get().IsEditing())
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::NodeToCompactJson(Node);
	ResultObj->SetBoolField(TEXT("dispatcher_resolved"), DelegateProp != nullptr);
	ResultObj->SetStringField(TEXT("dispatcher_name"), DispatcherName);
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleAddCallDelegateNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}
	FString DispatcherName;
	if (!Params->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = ResolveTargetGraph(Blueprint, Params);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
	}

	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
	}

	FMulticastDelegateProperty* DelegateProp = FindDispatcherProperty(Blueprint, FName(*DispatcherName));
	UClass* OwnerClass = Blueprint->SkeletonGeneratedClass ? Blueprint->SkeletonGeneratedClass : Blueprint->GeneratedClass;

	UK2Node_CallDelegate* Node = SpawnDelegateNode<UK2Node_CallDelegate>(Graph, DelegateProp, OwnerClass, Position);

	if (!FMCPBlueprintContext::Get().IsEditing())
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::NodeToCompactJson(Node);
	ResultObj->SetBoolField(TEXT("dispatcher_resolved"), DelegateProp != nullptr);
	ResultObj->SetStringField(TEXT("dispatcher_name"), DispatcherName);
	return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPBlueprintMultigraphCommands::HandleAddRemoveDelegateNode(const TSharedPtr<FJsonObject>& Params)
{
	FString BlueprintName;
	if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
	}
	FString DispatcherName;
	if (!Params->TryGetStringField(TEXT("dispatcher_name"), DispatcherName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'dispatcher_name' parameter"));
	}

	UBlueprint* Blueprint = FUnrealMCPCommonUtils::FindBlueprint(BlueprintName);
	if (!Blueprint)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
	}

	UEdGraph* Graph = ResolveTargetGraph(Blueprint, Params);
	if (!Graph)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Graph not found"));
	}

	FVector2D Position(0.0f, 0.0f);
	if (Params->HasField(TEXT("position")))
	{
		Position = FUnrealMCPCommonUtils::GetVector2DFromJson(Params, TEXT("position"));
	}

	FMulticastDelegateProperty* DelegateProp = FindDispatcherProperty(Blueprint, FName(*DispatcherName));
	UClass* OwnerClass = Blueprint->SkeletonGeneratedClass ? Blueprint->SkeletonGeneratedClass : Blueprint->GeneratedClass;

	UK2Node_RemoveDelegate* Node = SpawnDelegateNode<UK2Node_RemoveDelegate>(Graph, DelegateProp, OwnerClass, Position);

	if (!FMCPBlueprintContext::Get().IsEditing())
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
	}

	TSharedPtr<FJsonObject> ResultObj = FUnrealMCPCommonUtils::NodeToCompactJson(Node);
	ResultObj->SetBoolField(TEXT("dispatcher_resolved"), DelegateProp != nullptr);
	ResultObj->SetStringField(TEXT("dispatcher_name"), DispatcherName);
	return ResultObj;
}

//=============================================================================
// COMMAND REGISTRATION
//=============================================================================

void FUnrealMCPBlueprintMultigraphCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("create_function_graph_ex"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_function_graph_ex"), P); });
	Registry.RegisterCommand(TEXT("create_macro_graph_ex"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_macro_graph_ex"), P); });
	Registry.RegisterCommand(TEXT("create_event_dispatcher"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_event_dispatcher"), P); });
	Registry.RegisterCommand(TEXT("implement_interface"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("implement_interface"), P); });
	Registry.RegisterCommand(TEXT("add_local_variable_ex"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_local_variable_ex"), P); });
	Registry.RegisterCommand(TEXT("add_bind_delegate_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_bind_delegate_node"), P); });
	Registry.RegisterCommand(TEXT("add_call_delegate_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_call_delegate_node"), P); });
	Registry.RegisterCommand(TEXT("add_remove_delegate_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_remove_delegate_node"), P); });
}
