#pragma once

#include "CoreMinimal.h"
#include "Json.h"

// Forward declarations
class AActor;
class UBlueprint;
class UEdGraph;
class UEdGraphNode;
class UEdGraphPin;
class UK2Node_Event;
class UK2Node_CallFunction;
class UK2Node_VariableGet;
class UK2Node_VariableSet;
class UK2Node_InputAction;
class UK2Node_Self;
class UFunction;

/**
 * Common utilities for UnrealMCP commands
 */
class UNREALMCP_API FUnrealMCPCommonUtils
{
public:
    // JSON utilities
    static TSharedPtr<FJsonObject> CreateErrorResponse(const FString& Message);
    static TSharedPtr<FJsonObject> CreateSuccessResponse(const TSharedPtr<FJsonObject>& Data = nullptr);
    static void GetIntArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<int32>& OutArray);
    static void GetFloatArrayFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName, TArray<float>& OutArray);
    static FVector2D GetVector2DFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static FVector GetVectorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    static FRotator GetRotatorFromJson(const TSharedPtr<FJsonObject>& JsonObject, const FString& FieldName);
    
    // Actor utilities
    static TSharedPtr<FJsonValue> ActorToJson(AActor* Actor);
    static TSharedPtr<FJsonObject> ActorToJsonObject(AActor* Actor, bool bDetailed = false);
    
    // Blueprint utilities
    static UBlueprint* FindBlueprint(const FString& BlueprintName);
    static UBlueprint* FindBlueprintByName(const FString& BlueprintName);
    static UEdGraph* FindOrCreateEventGraph(UBlueprint* Blueprint);
    
    // Blueprint node utilities
    static UK2Node_Event* CreateEventNode(UEdGraph* Graph, const FString& EventName, const FVector2D& Position);
    static UK2Node_CallFunction* CreateFunctionCallNode(UEdGraph* Graph, UFunction* Function, const FVector2D& Position);

    /**
     * Resolve a BlueprintCallable function by target class and function name.
     *
     * Handles the common calling conventions clients use:
     *   - Empty target:  searches the blueprint's own class, then every
     *                    UBlueprintFunctionLibrary subclass.
     *   - Script path:   "/Script/Engine.KismetMathLibrary" — loaded directly.
     *   - Short name:    "KismetMathLibrary" — tries with/without "U" prefix
     *                    and a few canonical engine packages.
     *   - Fallback:      scans every loaded UClass for a name match.
     *
     * Returns nullptr if no matching function is found. Case-insensitive on
     * function name as a last resort (most UE math ops use CamelCase exactly).
     */
    static UFunction* FindCallableFunction(
        const FString& TargetClassName,
        const FString& FunctionName,
        UBlueprint* ContextBlueprint = nullptr);
    static UK2Node_VariableGet* CreateVariableGetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position);
    static UK2Node_VariableSet* CreateVariableSetNode(UEdGraph* Graph, UBlueprint* Blueprint, const FString& VariableName, const FVector2D& Position);
    static UK2Node_InputAction* CreateInputActionNode(UEdGraph* Graph, const FString& ActionName, const FVector2D& Position);
    static UK2Node_Self* CreateSelfReferenceNode(UEdGraph* Graph, const FVector2D& Position);
    static bool ConnectGraphNodes(UEdGraph* Graph, UEdGraphNode* SourceNode, const FString& SourcePinName, 
                                UEdGraphNode* TargetNode, const FString& TargetPinName);
    static UEdGraphPin* FindPin(UEdGraphNode* Node, const FString& PinName, EEdGraphPinDirection Direction = EGPD_MAX);
    static UK2Node_Event* FindExistingEventNode(UEdGraph* Graph, const FString& EventName);

    // Blueprint graph utilities
    static UEdGraph* FindGraphByName(UBlueprint* Blueprint, const FString& GraphName);
    static UEdGraphNode* FindNodeByGuid(UEdGraph* Graph, const FString& GuidString);

    // JSON serialization helpers for inspection
    static TSharedPtr<FJsonObject> PinToJson(UEdGraphPin* Pin, bool bIncludeConnections = true);
    static TSharedPtr<FJsonObject> NodeToJson(UEdGraphNode* Node, bool bIncludePins = false);

    /** Returns compact JSON for a newly created node: GUID + title + non-hidden pins only. */
    static TSharedPtr<FJsonObject> NodeToCompactJson(UEdGraphNode* Node);

    // Property utilities
    static bool SetObjectProperty(UObject* Object, const FString& PropertyName,
                                 const TSharedPtr<FJsonValue>& Value, FString& OutErrorMessage);
};