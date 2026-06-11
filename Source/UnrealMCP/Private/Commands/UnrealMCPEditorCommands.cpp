#include "Commands/UnrealMCPEditorCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Editor.h"
#include "EditorViewportClient.h"
#include "LevelEditorViewport.h"
#include "ImageUtils.h"
#include "HighResScreenshot.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "GameFramework/Actor.h"
#include "Engine/Selection.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "FileHelpers.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "Framework/Docking/TabManager.h"
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Misc/Base64.h"
#include "HAL/IConsoleManager.h"
#include "Components/PrimitiveComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/Material.h"

FUnrealMCPEditorCommands::FUnrealMCPEditorCommands()
{
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
    // Actor manipulation commands
    if (CommandType == TEXT("get_actors_in_level"))
    {
        return HandleGetActorsInLevel(Params);
    }
    else if (CommandType == TEXT("find_actors_by_name"))
    {
        return HandleFindActorsByName(Params);
    }
    else if (CommandType == TEXT("spawn_actor") || CommandType == TEXT("create_actor"))
    {
        if (CommandType == TEXT("create_actor"))
        {
            UE_LOG(LogTemp, Warning, TEXT("'create_actor' command is deprecated and will be removed in a future version. Please use 'spawn_actor' instead."));
        }
        return HandleSpawnActor(Params);
    }
    else if (CommandType == TEXT("delete_actor"))
    {
        return HandleDeleteActor(Params);
    }
    else if (CommandType == TEXT("set_actor_transform"))
    {
        return HandleSetActorTransform(Params);
    }
    else if (CommandType == TEXT("fix_null_static_mesh_actors"))
    {
        return HandleFixNullStaticMeshActors(Params);
    }
    else if (CommandType == TEXT("organize_outliner_by_class"))
    {
        return HandleOrganizeOutlinerByClass(Params);
    }
    else if (CommandType == TEXT("list_static_mesh_actors"))
    {
        return HandleListStaticMeshActors(Params);
    }
    else if (CommandType == TEXT("set_actor_folders"))
    {
        return HandleSetActorFolders(Params);
    }
    else if (CommandType == TEXT("get_actor_properties"))
    {
        return HandleGetActorProperties(Params);
    }
    else if (CommandType == TEXT("set_actor_property"))
    {
        return HandleSetActorProperty(Params);
    }
    // Blueprint actor spawning
    else if (CommandType == TEXT("spawn_blueprint_actor"))
    {
        return HandleSpawnBlueprintActor(Params);
    }
    // Editor viewport commands
    else if (CommandType == TEXT("focus_viewport"))
    {
        return HandleFocusViewport(Params);
    }
    else if (CommandType == TEXT("take_screenshot"))
    {
        return HandleTakeScreenshot(Params);
    }
    // Editor context and introspection commands
    else if (CommandType == TEXT("get_editor_context"))
    {
        return HandleGetEditorContext(Params);
    }
    else if (CommandType == TEXT("get_viewport_camera"))
    {
        return HandleGetViewportCamera(Params);
    }
    else if (CommandType == TEXT("take_editor_screenshot"))
    {
        return HandleTakeEditorScreenshot(Params);
    }
    else if (CommandType == TEXT("get_cvar"))
    {
        return HandleGetCVar(Params);
    }
    else if (CommandType == TEXT("set_cvar"))
    {
        return HandleSetCVar(Params);
    }
    // Actor material inspection
    else if (CommandType == TEXT("get_actor_material_info"))
    {
        return HandleGetActorMaterialInfo(Params);
    }
    else if (CommandType == TEXT("set_actor_material"))
    {
        return HandleSetActorMaterial(Params);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown editor command: %s"), *CommandType));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorsInLevel(const TSharedPtr<FJsonObject>& Params)
{
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> ActorArray;
    for (AActor* Actor : AllActors)
    {
        if (Actor)
        {
            ActorArray.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), ActorArray);
    
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFindActorsByName(const TSharedPtr<FJsonObject>& Params)
{
    FString Pattern;
    if (!Params->TryGetStringField(TEXT("pattern"), Pattern))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'pattern' parameter"));
    }
    
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    TArray<TSharedPtr<FJsonValue>> MatchingActors;
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName().Contains(Pattern))
        {
            MatchingActors.Add(FUnrealMCPCommonUtils::ActorToJson(Actor));
        }
    }
    
    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetArrayField(TEXT("actors"), MatchingActors);
    
    return ResultObj;
}

// Generic reflection-based property setter. Supports dot notation for component access
// (e.g. "StaticMeshComponent0.CastShadow" or "SpotLightComponent0.InnerConeAngle").
// Also handles special cases: asset path loading for UObject* properties.
static bool SetReflectionProperty(UObject* Object, const FString& PropertyPath, const TSharedPtr<FJsonValue>& Value, FString& OutError)
{
    if (!Object)
    {
        OutError = TEXT("Null object");
        return false;
    }

    // Dot notation: "ComponentName.Property" or "Component.Sub.Property"
    FString FirstPart, Remainder;
    if (PropertyPath.Split(TEXT("."), &FirstPart, &Remainder))
    {
        // Try component lookup first (actors only)
        if (AActor* Actor = Cast<AActor>(Object))
        {
            for (UActorComponent* Comp : Actor->GetComponents())
            {
                if (Comp->GetName() == FirstPart || Comp->GetClass()->GetName() == FirstPart)
                {
                    return SetReflectionProperty(Comp, Remainder, Value, OutError);
                }
            }
        }
        // Try struct property on the object itself
        FProperty* StructField = Object->GetClass()->FindPropertyByName(FName(*FirstPart));
        if (FStructProperty* StructProp = CastField<FStructProperty>(StructField))
        {
            // Flatten the struct access into a direct property path on the struct memory
            void* StructPtr = StructProp->ContainerPtrToValuePtr<void>(Object);
            // Recurse: find sub-property (supports arbitrary nesting)
            FString SubFirst, SubRest;
            UScriptStruct* CurrentStruct = StructProp->Struct;
            void* CurrentPtr = StructPtr;
            FString CurrentPath = Remainder;
            while (CurrentPath.Split(TEXT("."), &SubFirst, &SubRest))
            {
                FProperty* SubProp = CurrentStruct->FindPropertyByName(FName(*SubFirst));
                FStructProperty* SubStruct = CastField<FStructProperty>(SubProp);
                if (!SubStruct) { break; }
                CurrentPtr = SubStruct->ContainerPtrToValuePtr<void>(CurrentPtr);
                CurrentStruct = SubStruct->Struct;
                CurrentPath = SubRest;
            }
            // CurrentPath is the final field name, CurrentStruct+CurrentPtr is the container
            FProperty* FinalProp = CurrentStruct->FindPropertyByName(FName(*CurrentPath));
            if (!FinalProp)
            {
                OutError = FString::Printf(TEXT("Field '%s' not found in struct %s"), *CurrentPath, *CurrentStruct->GetName());
                return false;
            }
            void* FinalPtr = FinalProp->ContainerPtrToValuePtr<void>(CurrentPtr);
            // Type dispatch (same set as below)
            if (FBoolProperty* BP = CastField<FBoolProperty>(FinalProp)) { bool bV=false; if(Value->TryGetBool(bV)){BP->SetPropertyValue(FinalPtr,bV);}else{BP->SetPropertyValue(FinalPtr,Value->AsString().ToBool());} return true; }
            if (FFloatProperty* FP = CastField<FFloatProperty>(FinalProp)) { double V=0; Value->TryGetNumber(V); FP->SetPropertyValue(FinalPtr,static_cast<float>(V)); return true; }
            if (FDoubleProperty* DP = CastField<FDoubleProperty>(FinalProp)) { double V=0; Value->TryGetNumber(V); DP->SetPropertyValue(FinalPtr,V); return true; }
            if (FIntProperty* IP = CastField<FIntProperty>(FinalProp)) { double V=0; Value->TryGetNumber(V); IP->SetPropertyValue(FinalPtr,static_cast<int32>(V)); return true; }
            if (FByteProperty* YP = CastField<FByteProperty>(FinalProp)) { double V=0; Value->TryGetNumber(V); YP->SetPropertyValue(FinalPtr,static_cast<uint8>(V)); return true; }
            if (FEnumProperty* EP = CastField<FEnumProperty>(FinalProp)) { FString S; double N; if(Value->TryGetString(S)){int64 E=EP->GetEnum()->GetValueByNameString(S);if(E!=INDEX_NONE){EP->GetUnderlyingProperty()->SetIntPropertyValue(FinalPtr,E);return true;}} if(Value->TryGetNumber(N)){EP->GetUnderlyingProperty()->SetIntPropertyValue(FinalPtr,static_cast<int64>(N));return true;} }
            OutError = FString::Printf(TEXT("Unsupported type for struct field '%s'"), *CurrentPath);
            return false;
        }
        if (Cast<AActor>(Object))
        {
            OutError = FString::Printf(TEXT("'%s' is not a component or struct on %s"), *FirstPart, *Object->GetName());
            return false;
        }
    }

    FProperty* Prop = Object->GetClass()->FindPropertyByName(FName(*PropertyPath));
    if (!Prop)
    {
        OutError = FString::Printf(TEXT("Property '%s' not found on %s"), *PropertyPath, *Object->GetClass()->GetName());
        return false;
    }

    void* ValuePtr = Prop->ContainerPtrToValuePtr<void>(Object);

    if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
    {
        bool bVal = false;
        if (Value->TryGetBool(bVal)) { BoolProp->SetPropertyValue(ValuePtr, bVal); return true; }
        BoolProp->SetPropertyValue(ValuePtr, Value->AsString().ToBool()); return true;
    }
    if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
    {
        double Val = 0; Value->TryGetNumber(Val);
        FloatProp->SetPropertyValue(ValuePtr, static_cast<float>(Val)); return true;
    }
    if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
    {
        double Val = 0; Value->TryGetNumber(Val);
        DoubleProp->SetPropertyValue(ValuePtr, Val); return true;
    }
    if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
    {
        double Val = 0; Value->TryGetNumber(Val);
        IntProp->SetPropertyValue(ValuePtr, static_cast<int32>(Val)); return true;
    }
    if (FStrProperty* StrProp = CastField<FStrProperty>(Prop))
    {
        StrProp->SetPropertyValue(ValuePtr, Value->AsString()); return true;
    }
    if (FNameProperty* NameProp = CastField<FNameProperty>(Prop))
    {
        NameProp->SetPropertyValue(ValuePtr, FName(*Value->AsString())); return true;
    }
    if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Prop))
    {
        FString StrVal; double NumVal;
        if (Value->TryGetString(StrVal))
        {
            int64 EVal = EnumProp->GetEnum()->GetValueByNameString(StrVal);
            if (EVal != INDEX_NONE) { EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, EVal); return true; }
        }
        if (Value->TryGetNumber(NumVal))
        {
            EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(ValuePtr, static_cast<int64>(NumVal)); return true;
        }
    }
    if (FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
    {
        FString StrVal; double NumVal;
        if (ByteProp->Enum && Value->TryGetString(StrVal))
        {
            int64 EVal = ByteProp->Enum->GetValueByNameString(StrVal);
            if (EVal != INDEX_NONE) { ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(EVal)); return true; }
        }
        if (Value->TryGetNumber(NumVal)) { ByteProp->SetPropertyValue(ValuePtr, static_cast<uint8>(NumVal)); return true; }
    }
    // UObject* properties — load by asset path string
    if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Prop))
    {
        FString AssetPath = Value->AsString();
        UObject* Asset = LoadObject<UObject>(nullptr, *AssetPath);
        if (!Asset)
        {
            FString WithDot = AssetPath + TEXT(".") + FPaths::GetCleanFilename(AssetPath);
            Asset = LoadObject<UObject>(nullptr, *WithDot);
        }
        if (Asset)
        {
            ObjProp->SetObjectPropertyValue(ValuePtr, Asset);
            return true;
        }
        OutError = FString::Printf(TEXT("Asset not found: %s"), *AssetPath);
        return false;
    }

    OutError = FString::Printf(TEXT("Unsupported property type for '%s'"), *PropertyPath);
    return false;
}

// Apply a "properties" JSON dict to an actor + its components via reflection
static int32 ApplyPropertiesDict(AActor* Actor, const TSharedPtr<FJsonObject>& PropsObj, TArray<FString>& Errors)
{
    int32 Applied = 0;
    for (const auto& Pair : PropsObj->Values)
    {
        FString Error;
        if (SetReflectionProperty(Actor, Pair.Key, Pair.Value, Error))
        {
            ++Applied;
        }
        else
        {
            Errors.Add(Error);
        }
    }
    return Applied;
}

// Resolve actor class from short name or full class path
static UClass* ResolveActorClass(const FString& TypeStr)
{
    static TMap<FString, FString> ShortNames;
    if (ShortNames.Num() == 0)
    {
        ShortNames.Add(TEXT("StaticMeshActor"),    TEXT("/Script/Engine.StaticMeshActor"));
        ShortNames.Add(TEXT("PointLight"),          TEXT("/Script/Engine.PointLight"));
        ShortNames.Add(TEXT("SpotLight"),           TEXT("/Script/Engine.SpotLight"));
        ShortNames.Add(TEXT("DirectionalLight"),    TEXT("/Script/Engine.DirectionalLight"));
        ShortNames.Add(TEXT("RectLight"),           TEXT("/Script/Engine.RectLight"));
        ShortNames.Add(TEXT("CameraActor"),         TEXT("/Script/Engine.CameraActor"));
        ShortNames.Add(TEXT("CineCameraActor"),     TEXT("/Script/CinematicCamera.CineCameraActor"));
        ShortNames.Add(TEXT("ExponentialHeightFog"),TEXT("/Script/Engine.ExponentialHeightFog"));
        ShortNames.Add(TEXT("SkyLight"),            TEXT("/Script/Engine.SkyLight"));
        ShortNames.Add(TEXT("PostProcessVolume"),   TEXT("/Script/Engine.PostProcessVolume"));
        ShortNames.Add(TEXT("SphereReflectionCapture"), TEXT("/Script/Engine.SphereReflectionCapture"));
        ShortNames.Add(TEXT("BoxReflectionCapture"),TEXT("/Script/Engine.BoxReflectionCapture"));
    }

    // Case-insensitive short name lookup
    for (const auto& Entry : ShortNames)
    {
        if (Entry.Key.Equals(TypeStr, ESearchCase::IgnoreCase))
        {
            return StaticLoadClass(AActor::StaticClass(), nullptr, *Entry.Value);
        }
    }

    // Full class path (e.g. "/Script/Engine.APointLight" or "/Script/CinematicCamera.ACineCameraActor")
    if (TypeStr.StartsWith(TEXT("/")))
    {
        return StaticLoadClass(AActor::StaticClass(), nullptr, *TypeStr);
    }

    // Try common patterns: /Script/Engine.A<Type> or /Script/Engine.<Type>
    UClass* Found = StaticLoadClass(AActor::StaticClass(), nullptr, *FString::Printf(TEXT("/Script/Engine.A%s"), *TypeStr));
    if (!Found)
    {
        Found = StaticLoadClass(AActor::StaticClass(), nullptr, *FString::Printf(TEXT("/Script/Engine.%s"), *TypeStr));
    }
    return Found;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);
    if (Params->HasField(TEXT("location"))) { Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")); }
    if (Params->HasField(TEXT("rotation"))) { Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation")); }
    if (Params->HasField(TEXT("scale")))    { Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")); }

    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World) { return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world")); }

    // Resolve actor class generically
    UClass* ActorClass = ResolveActorClass(ActorType);
    if (!ActorClass)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Cannot resolve actor class: %s"), *ActorType));
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;
    SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

    AActor* NewActor = World->SpawnActor(ActorClass, &Location, &Rotation, SpawnParams);
    if (!NewActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to spawn %s (name '%s' may already exist)"), *ActorType, *ActorName));
    }

    // Apply scale
    FTransform Transform = NewActor->GetTransform();
    Transform.SetScale3D(Scale);
    NewActor->SetActorTransform(Transform);

    // Special case: StaticMeshActor — set mesh from "static_mesh" param
    if (AStaticMeshActor* SMActor = Cast<AStaticMeshActor>(NewActor))
    {
        FString MeshPath;
        if (!Params->TryGetStringField(TEXT("static_mesh"), MeshPath))
        {
            MeshPath = TEXT("/Engine/BasicShapes/Cube.Cube");
        }
        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *MeshPath);
        if (Mesh && SMActor->GetStaticMeshComponent())
        {
            SMActor->GetStaticMeshComponent()->SetStaticMesh(Mesh);
        }
    }

    // Apply "properties" dict via reflection (generic — works for ANY actor type)
    const TSharedPtr<FJsonObject>* PropsObj = nullptr;
    if (Params->TryGetObjectField(TEXT("properties"), PropsObj))
    {
        TArray<FString> Errors;
        ApplyPropertiesDict(NewActor, *PropsObj, Errors);
    }

    return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleDeleteActor(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            // Store actor info before deletion for the response
            TSharedPtr<FJsonObject> ActorInfo = FUnrealMCPCommonUtils::ActorToJsonObject(Actor);
            
            // Delete the actor
            Actor->Destroy();
            
            TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
            ResultObj->SetObjectField(TEXT("deleted_actor"), ActorInfo);
            return ResultObj;
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorTransform(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Get transform parameters
    FTransform NewTransform = TargetActor->GetTransform();

    if (Params->HasField(TEXT("location")))
    {
        NewTransform.SetLocation(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location")));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        NewTransform.SetRotation(FQuat(FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"))));
    }
    if (Params->HasField(TEXT("scale")))
    {
        NewTransform.SetScale3D(FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale")));
    }

    // Set the new transform
    TargetActor->SetActorTransform(NewTransform);

    // Return updated actor info
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorProperties(const TSharedPtr<FJsonObject>& Params)
{
    // Get actor name
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Find the actor
    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Always return detailed properties for this command
    TSharedPtr<FJsonObject> Result = FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);

    // Enumerate components + their primitive (numeric/bool/struct) property values.
    TArray<TSharedPtr<FJsonValue>> CompArray;
    TArray<UActorComponent*> Comps;
    TargetActor->GetComponents(Comps);
    for (UActorComponent* Comp : Comps)
    {
        if (!Comp) continue;
        TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
        CompObj->SetStringField(TEXT("name"), Comp->GetName());
        CompObj->SetStringField(TEXT("class"), Comp->GetClass()->GetName());

        TSharedPtr<FJsonObject> PropsObj = MakeShared<FJsonObject>();
        for (TFieldIterator<FProperty> It(Comp->GetClass()); It; ++It)
        {
            FProperty* Prop = *It;
            const FString PropName = Prop->GetName();
            if (FFloatProperty* FP = CastField<FFloatProperty>(Prop))
            {
                PropsObj->SetNumberField(PropName, FP->GetPropertyValue_InContainer(Comp));
            }
            else if (FDoubleProperty* DP = CastField<FDoubleProperty>(Prop))
            {
                PropsObj->SetNumberField(PropName, DP->GetPropertyValue_InContainer(Comp));
            }
            else if (FIntProperty* IP = CastField<FIntProperty>(Prop))
            {
                PropsObj->SetNumberField(PropName, IP->GetPropertyValue_InContainer(Comp));
            }
            else if (FBoolProperty* BoP = CastField<FBoolProperty>(Prop))
            {
                PropsObj->SetBoolField(PropName, BoP->GetPropertyValue_InContainer(Comp));
            }
            else if (FByteProperty* BYP = CastField<FByteProperty>(Prop))
            {
                if (UEnum* E = BYP->Enum)
                {
                    int64 V = BYP->GetPropertyValue_InContainer(Comp);
                    PropsObj->SetStringField(PropName, E->GetNameStringByValue(V));
                }
                else
                {
                    PropsObj->SetNumberField(PropName, BYP->GetPropertyValue_InContainer(Comp));
                }
            }
            else if (FEnumProperty* EP = CastField<FEnumProperty>(Prop))
            {
                int64 V = EP->GetUnderlyingProperty()->GetSignedIntPropertyValue(
                    EP->ContainerPtrToValuePtr<void>(Comp));
                UEnum* E = EP->GetEnum();
                PropsObj->SetStringField(PropName, E ? E->GetNameStringByValue(V) : FString::FromInt(V));
            }
            else if (FStructProperty* SP = CastField<FStructProperty>(Prop))
            {
                if (SP->Struct == TBaseStructure<FVector>::Get())
                {
                    FVector V = *SP->ContainerPtrToValuePtr<FVector>(Comp);
                    TArray<TSharedPtr<FJsonValue>> A;
                    A.Add(MakeShared<FJsonValueNumber>(V.X));
                    A.Add(MakeShared<FJsonValueNumber>(V.Y));
                    A.Add(MakeShared<FJsonValueNumber>(V.Z));
                    PropsObj->SetArrayField(PropName, A);
                }
                else if (SP->Struct == TBaseStructure<FLinearColor>::Get())
                {
                    FLinearColor C = *SP->ContainerPtrToValuePtr<FLinearColor>(Comp);
                    TArray<TSharedPtr<FJsonValue>> A;
                    A.Add(MakeShared<FJsonValueNumber>(C.R));
                    A.Add(MakeShared<FJsonValueNumber>(C.G));
                    A.Add(MakeShared<FJsonValueNumber>(C.B));
                    A.Add(MakeShared<FJsonValueNumber>(C.A));
                    PropsObj->SetArrayField(PropName, A);
                }
                else if (SP->Struct == TBaseStructure<FRotator>::Get())
                {
                    FRotator R = *SP->ContainerPtrToValuePtr<FRotator>(Comp);
                    TArray<TSharedPtr<FJsonValue>> A;
                    A.Add(MakeShared<FJsonValueNumber>(R.Pitch));
                    A.Add(MakeShared<FJsonValueNumber>(R.Yaw));
                    A.Add(MakeShared<FJsonValueNumber>(R.Roll));
                    PropsObj->SetArrayField(PropName, A);
                }
            }
        }
        CompObj->SetObjectField(TEXT("properties"), PropsObj);
        CompArray.Add(MakeShared<FJsonValueObject>(CompObj));
    }
    Result->SetArrayField(TEXT("components"), CompArray);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetActorMaterialInfo(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }

    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    TSharedPtr<FJsonObject> Data = MakeShared<FJsonObject>();
    Data->SetStringField(TEXT("actor_name"), TargetActor->GetName());
    Data->SetStringField(TEXT("actor_class"), TargetActor->GetClass()->GetName());

    TArray<TSharedPtr<FJsonValue>> ComponentsArray;
    TArray<UPrimitiveComponent*> PrimitiveComponents;
    TargetActor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

    for (UPrimitiveComponent* Comp : PrimitiveComponents)
    {
        if (!Comp) continue;

        TSharedPtr<FJsonObject> CompObj = MakeShared<FJsonObject>();
        CompObj->SetStringField(TEXT("component_name"), Comp->GetName());
        CompObj->SetStringField(TEXT("component_class"), Comp->GetClass()->GetName());
        CompObj->SetNumberField(TEXT("num_materials"), Comp->GetNumMaterials());

        TArray<TSharedPtr<FJsonValue>> MaterialsArray;
        for (int32 SlotIdx = 0; SlotIdx < Comp->GetNumMaterials(); SlotIdx++)
        {
            TSharedPtr<FJsonObject> MatObj = MakeShared<FJsonObject>();
            MatObj->SetNumberField(TEXT("slot_index"), SlotIdx);

            UMaterialInterface* Material = Comp->GetMaterial(SlotIdx);
            if (Material)
            {
                MatObj->SetStringField(TEXT("material_name"), Material->GetName());
                MatObj->SetStringField(TEXT("material_path"), Material->GetPathName());
                MatObj->SetStringField(TEXT("material_class"), Material->GetClass()->GetName());

                if (Cast<UMaterialInstanceDynamic>(Material))
                {
                    MatObj->SetStringField(TEXT("instance_type"), TEXT("Dynamic"));
                }
                else if (Cast<UMaterialInstanceConstant>(Material))
                {
                    MatObj->SetStringField(TEXT("instance_type"), TEXT("Constant"));
                }

                UMaterial* BaseMat = Material->GetMaterial();
                if (BaseMat && BaseMat != Material)
                {
                    MatObj->SetStringField(TEXT("base_material_name"), BaseMat->GetName());
                    MatObj->SetStringField(TEXT("base_material_path"), BaseMat->GetPathName());
                }
            }
            else
            {
                MatObj->SetStringField(TEXT("material_name"), TEXT("None"));
            }

            MaterialsArray.Add(MakeShared<FJsonValueObject>(MatObj));
        }

        CompObj->SetArrayField(TEXT("materials"), MaterialsArray);
        ComponentsArray.Add(MakeShared<FJsonValueObject>(CompObj));
    }

    Data->SetArrayField(TEXT("components"), ComponentsArray);
    Data->SetNumberField(TEXT("total_components"), PrimitiveComponents.Num());

    return FUnrealMCPCommonUtils::CreateSuccessResponse(Data);
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorMaterial(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }
    FString MaterialPath;
    if (!Params->TryGetStringField(TEXT("material_path"), MaterialPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_path' parameter"));
    }
    int32 SlotIndex = 0;
    Params->TryGetNumberField(TEXT("slot_index"), SlotIndex);

    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            TargetActor = Actor;
            break;
        }
    }
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    UMaterialInterface* NewMaterial = LoadObject<UMaterialInterface>(nullptr, *MaterialPath);
    if (!NewMaterial)
    {
        FString WithDot = MaterialPath + TEXT(".") + FPaths::GetCleanFilename(MaterialPath);
        NewMaterial = LoadObject<UMaterialInterface>(nullptr, *WithDot);
    }
    if (!NewMaterial)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialPath));
    }

    TArray<UPrimitiveComponent*> Components;
    TargetActor->GetComponents<UPrimitiveComponent>(Components);
    int32 Applied = 0;
    for (UPrimitiveComponent* Comp : Components)
    {
        if (SlotIndex < Comp->GetNumMaterials())
        {
            Comp->SetMaterial(SlotIndex, NewMaterial);
            ++Applied;
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), Applied > 0);
    ResultObj->SetStringField(TEXT("actor"), ActorName);
    ResultObj->SetStringField(TEXT("material"), MaterialPath);
    ResultObj->SetNumberField(TEXT("components_updated"), Applied);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
{
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    AActor* TargetActor = nullptr;
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName) { TargetActor = Actor; break; }
    }
    if (!TargetActor)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *ActorName));
    }

    // Mode 1: single property via "property_name" + "property_value"
    FString PropertyName;
    if (Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        if (!Params->HasField(TEXT("property_value")))
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
        }
        TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));

        if (PropertyName == TEXT("FolderPath"))
        {
            TargetActor->SetFolderPath(FName(*PropertyValue->AsString()));
        }
        else
        {
            FString Error;
            if (!SetReflectionProperty(TargetActor, PropertyName, PropertyValue, Error))
            {
                return FUnrealMCPCommonUtils::CreateErrorResponse(Error);
            }
        }

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }

    // Mode 2: batch properties via "properties" dict
    const TSharedPtr<FJsonObject>* PropsObj = nullptr;
    if (Params->TryGetObjectField(TEXT("properties"), PropsObj))
    {
        TArray<FString> Errors;
        int32 Applied = ApplyPropertiesDict(TargetActor, *PropsObj, Errors);

        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetBoolField(TEXT("success"), Applied > 0);
        ResultObj->SetNumberField(TEXT("properties_set"), Applied);
        if (Errors.Num() > 0)
        {
            TArray<TSharedPtr<FJsonValue>> ErrArr;
            for (const FString& E : Errors) { ErrArr.Add(MakeShared<FJsonValueString>(E)); }
            ResultObj->SetArrayField(TEXT("errors"), ErrArr);
        }
        return ResultObj;
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Provide 'property_name'+'property_value' or 'properties' dict"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnBlueprintActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString BlueprintName;
    if (!Params->TryGetStringField(TEXT("blueprint_name"), BlueprintName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'blueprint_name' parameter"));
    }

    FString ActorName;
    if (!Params->TryGetStringField(TEXT("actor_name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_name' parameter"));
    }

    // Find the blueprint
    if (BlueprintName.IsEmpty())
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Blueprint name is empty"));
    }

    FString Root      = TEXT("/Game/Blueprints/");
    FString AssetPath = Root + BlueprintName;

    if (!FPackageName::DoesPackageExist(AssetPath))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint '%s' not found – it must reside under /Game/Blueprints"), *BlueprintName));
    }

    UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *AssetPath);
    if (!Blueprint)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Blueprint not found: %s"), *BlueprintName));
    }

    // Get transform parameters
    FVector Location(0.0f, 0.0f, 0.0f);
    FRotator Rotation(0.0f, 0.0f, 0.0f);
    FVector Scale(1.0f, 1.0f, 1.0f);

    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
    }
    if (Params->HasField(TEXT("rotation")))
    {
        Rotation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("rotation"));
    }
    if (Params->HasField(TEXT("scale")))
    {
        Scale = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("scale"));
    }

    // Spawn the actor
    UWorld* World = GEditor->GetEditorWorldContext().World();
    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    FTransform SpawnTransform;
    SpawnTransform.SetLocation(Location);
    SpawnTransform.SetRotation(FQuat(Rotation));
    SpawnTransform.SetScale3D(Scale);

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;
    SpawnParams.NameMode = FActorSpawnParameters::ESpawnActorNameMode::Required_ErrorAndReturnNull;

    AActor* NewActor = World->SpawnActor<AActor>(Blueprint->GeneratedClass, SpawnTransform, SpawnParams);
    if (NewActor)
    {
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to spawn blueprint actor"));
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFocusViewport(const TSharedPtr<FJsonObject>& Params)
{
    // Get target actor name if provided
    FString TargetActorName;
    bool HasTargetActor = Params->TryGetStringField(TEXT("target"), TargetActorName);

    // Get location if provided
    FVector Location(0.0f, 0.0f, 0.0f);
    bool HasLocation = false;
    if (Params->HasField(TEXT("location")))
    {
        Location = FUnrealMCPCommonUtils::GetVectorFromJson(Params, TEXT("location"));
        HasLocation = true;
    }

    // Get distance
    float Distance = 1000.0f;
    if (Params->HasField(TEXT("distance")))
    {
        Distance = Params->GetNumberField(TEXT("distance"));
    }

    // Get orientation if provided
    FRotator Orientation(0.0f, 0.0f, 0.0f);
    bool HasOrientation = false;
    if (Params->HasField(TEXT("orientation")))
    {
        Orientation = FUnrealMCPCommonUtils::GetRotatorFromJson(Params, TEXT("orientation"));
        HasOrientation = true;
    }

    // Get the active viewport
    FLevelEditorViewportClient* ViewportClient = (FLevelEditorViewportClient*)GEditor->GetActiveViewport()->GetClient();
    if (!ViewportClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get active viewport"));
    }

    // If we have a target actor, focus on it
    if (HasTargetActor)
    {
        // Find the actor
        AActor* TargetActor = nullptr;
        TArray<AActor*> AllActors;
        UGameplayStatics::GetAllActorsOfClass(GWorld, AActor::StaticClass(), AllActors);
        
        for (AActor* Actor : AllActors)
        {
            if (Actor && Actor->GetName() == TargetActorName)
            {
                TargetActor = Actor;
                break;
            }
        }

        if (!TargetActor)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor not found: %s"), *TargetActorName));
        }

        // Focus on the actor
        ViewportClient->SetViewLocation(TargetActor->GetActorLocation() - FVector(Distance, 0.0f, 0.0f));
    }
    // Otherwise use the provided location
    else if (HasLocation)
    {
        ViewportClient->SetViewLocation(Location - FVector(Distance, 0.0f, 0.0f));
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Either 'target' or 'location' must be provided"));
    }

    // Set orientation if provided
    if (HasOrientation)
    {
        ViewportClient->SetViewRotation(Orientation);
    }

    // Force viewport to redraw
    ViewportClient->Invalidate();

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetBoolField(TEXT("success"), true);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeScreenshot(const TSharedPtr<FJsonObject>& Params)
{
    // Get file path parameter
    FString FilePath;
    if (!Params->TryGetStringField(TEXT("filepath"), FilePath))
    {
        // Default to project Saved directory
        FilePath = FPaths::ProjectSavedDir() / TEXT("Screenshots") / TEXT("mcp_screenshot.png");
    }

    // Ensure the file path has a proper extension
    if (!FilePath.EndsWith(TEXT(".png")))
    {
        FilePath += TEXT(".png");
    }

    // Sanitize: resolve to absolute and ensure it's under the project directory
    FilePath = FPaths::ConvertRelativePathToFull(FilePath);
    FString ProjectDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectDir());
    FString SavedDir = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    if (!FilePath.StartsWith(ProjectDir) && !FilePath.StartsWith(SavedDir))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(
            TEXT("Screenshot path must be under the project directory"));
    }

    // Get the active viewport
    if (GEditor && GEditor->GetActiveViewport())
    {
        FViewport* Viewport = GEditor->GetActiveViewport();
        TArray<FColor> Bitmap;
        FIntRect ViewportRect(0, 0, Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y);
        
        if (Viewport->ReadPixels(Bitmap, FReadSurfaceDataFlags(), ViewportRect))
        {
            TArray64<uint8> CompressedBitmap;
            FImageUtils::PNGCompressImageArray(Viewport->GetSizeXY().X, Viewport->GetSizeXY().Y, Bitmap, CompressedBitmap);

            if (FFileHelper::SaveArrayToFile(CompressedBitmap, *FilePath))
            {
                TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
                ResultObj->SetStringField(TEXT("filepath"), FilePath);
                return ResultObj;
            }
        }
    }
    
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to take screenshot"));
}

// ============================================================================
// EDITOR CONTEXT AND INTROSPECTION COMMANDS
// ============================================================================

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetEditorContext(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();

    // Selected actors
    TArray<TSharedPtr<FJsonValue>> SelectedActors;
    if (UEditorActorSubsystem* ActorSub = GEditor->GetEditorSubsystem<UEditorActorSubsystem>())
    {
        TArray<AActor*> Selected = ActorSub->GetSelectedLevelActors();
        for (AActor* Actor : Selected)
        {
            if (!Actor) continue;
            TSharedPtr<FJsonObject> ActorObj = MakeShared<FJsonObject>();
            ActorObj->SetStringField(TEXT("name"), Actor->GetActorLabel());
            ActorObj->SetStringField(TEXT("class"), Actor->GetClass()->GetName());
            FVector Loc = Actor->GetActorLocation();
            TArray<TSharedPtr<FJsonValue>> LocArr = {
                MakeShared<FJsonValueNumber>(Loc.X),
                MakeShared<FJsonValueNumber>(Loc.Y),
                MakeShared<FJsonValueNumber>(Loc.Z)
            };
            ActorObj->SetArrayField(TEXT("location"), LocArr);
            SelectedActors.Add(MakeShared<FJsonValueObject>(ActorObj));
        }
    }
    Result->SetArrayField(TEXT("selected_actors"), SelectedActors);

    // Open asset editors
    TArray<TSharedPtr<FJsonValue>> OpenEditors;
    if (UAssetEditorSubsystem* AssetEditorSub = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>())
    {
        TArray<UObject*> EditedAssets = AssetEditorSub->GetAllEditedAssets();
        for (UObject* Asset : EditedAssets)
        {
            if (!Asset) continue;
            TSharedPtr<FJsonObject> EditorObj = MakeShared<FJsonObject>();
            EditorObj->SetStringField(TEXT("asset"), Asset->GetPathName());
            EditorObj->SetStringField(TEXT("class"), Asset->GetClass()->GetName());
            OpenEditors.Add(MakeShared<FJsonValueObject>(EditorObj));
        }
    }
    Result->SetArrayField(TEXT("open_asset_editors"), OpenEditors);

    // Active tab
    FString ActiveTabLabel = TEXT("Unknown");
    TSharedPtr<SDockTab> ActiveTab = FGlobalTabmanager::Get()->GetActiveTab();
    if (ActiveTab.IsValid())
    {
        ActiveTabLabel = ActiveTab->GetTabLabel().ToString();
    }
    Result->SetStringField(TEXT("active_tab"), ActiveTabLabel);

    // Current level
    FString LevelName = TEXT("Unknown");
    if (GWorld)
    {
        LevelName = GWorld->GetMapName();
    }
    Result->SetStringField(TEXT("current_level"), LevelName);

    // PIE state
    Result->SetBoolField(TEXT("pie_active"), GEditor->PlayWorld != nullptr);

    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("get_editor_context requires editor build"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetViewportCamera(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    // Find first valid level editor viewport
    FLevelEditorViewportClient* ActiveClient = nullptr;
    for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
    {
        if (Client)
        {
            ActiveClient = Client;
            break;
        }
    }

    if (!ActiveClient)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("get_viewport_camera: no active level viewport found"));
    }

    FVector Location = ActiveClient->GetViewLocation();
    FRotator Rotation = ActiveClient->GetViewRotation();

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    TArray<TSharedPtr<FJsonValue>> LocArr = {
        MakeShared<FJsonValueNumber>(Location.X),
        MakeShared<FJsonValueNumber>(Location.Y),
        MakeShared<FJsonValueNumber>(Location.Z)
    };
    Result->SetArrayField(TEXT("location"), LocArr);

    TArray<TSharedPtr<FJsonValue>> RotArr = {
        MakeShared<FJsonValueNumber>(Rotation.Pitch),
        MakeShared<FJsonValueNumber>(Rotation.Yaw),
        MakeShared<FJsonValueNumber>(Rotation.Roll)
    };
    Result->SetArrayField(TEXT("rotation"), RotArr);
    Result->SetNumberField(TEXT("fov"), ActiveClient->ViewFOV);
    Result->SetNumberField(TEXT("view_mode"), static_cast<int32>(ActiveClient->GetViewMode()));

    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("get_viewport_camera requires editor build"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleTakeEditorScreenshot(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
    // Find first level editor viewport client with a valid viewport
    FLevelEditorViewportClient* ActiveClient = nullptr;
    for (FLevelEditorViewportClient* Client : GEditor->GetLevelViewportClients())
    {
        if (Client && Client->Viewport)
        {
            ActiveClient = Client;
            break;
        }
    }

    if (!ActiveClient || !ActiveClient->Viewport)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("take_editor_screenshot: no active viewport found"));
    }

    FViewport* Viewport = ActiveClient->Viewport;
    FIntPoint Size = Viewport->GetSizeXY();
    if (Size.X <= 0 || Size.Y <= 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("take_editor_screenshot: viewport has zero size"));
    }

    TArray<FColor> Pixels;
    Pixels.SetNumUninitialized(Size.X * Size.Y);
    if (!Viewport->ReadPixels(Pixels, FReadSurfaceDataFlags(), FIntRect(0, 0, Size.X, Size.Y)))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("take_editor_screenshot: ReadPixels failed"));
    }

    // Encode to PNG via ImageWrapper module
    IImageWrapperModule& Module = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
    TSharedPtr<IImageWrapper> Wrapper = Module.CreateImageWrapper(EImageFormat::PNG);
    Wrapper->SetRaw(Pixels.GetData(), Pixels.Num() * sizeof(FColor), Size.X, Size.Y, ERGBFormat::BGRA, 8);

    TArray64<uint8> Compressed = Wrapper->GetCompressed(100);

    check(Compressed.Num() <= static_cast<int64>(TNumericLimits<uint32>::Max()));
    FString Base64 = FBase64::Encode(Compressed.GetData(), static_cast<uint32>(Compressed.Num()));

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetBoolField(TEXT("success"), true);
    Result->SetNumberField(TEXT("width"), Size.X);
    Result->SetNumberField(TEXT("height"), Size.Y);
    Result->SetStringField(TEXT("image_base64"), Base64);
    return Result;
#else
    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("take_editor_screenshot requires editor build"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleGetCVar(const TSharedPtr<FJsonObject>& Params)
{
    FString VarName;
    if (!Params->TryGetStringField(TEXT("name"), VarName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("get_cvar requires 'name'"));
    }

    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*VarName);
    if (!CVar)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("CVar not found: %s"), *VarName));
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), VarName);
    Result->SetStringField(TEXT("value"), CVar->GetString());

    // Determine type
    if (CVar->IsVariableBool())        { Result->SetStringField(TEXT("type"), TEXT("bool")); }
    else if (CVar->IsVariableInt())    { Result->SetStringField(TEXT("type"), TEXT("int")); }
    else if (CVar->IsVariableFloat())  { Result->SetStringField(TEXT("type"), TEXT("float")); }
    else                               { Result->SetStringField(TEXT("type"), TEXT("string")); }

    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetCVar(const TSharedPtr<FJsonObject>& Params)
{
    FString VarName;
    FString Value;
    if (!Params->TryGetStringField(TEXT("name"), VarName) || !Params->TryGetStringField(TEXT("value"), Value))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("set_cvar requires 'name' and 'value'"));
    }

    IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(*VarName);
    if (!CVar)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("CVar not found: %s"), *VarName));
    }

    CVar->Set(*Value, ECVF_SetByCode);

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetStringField(TEXT("name"), VarName);
    Result->SetStringField(TEXT("new_value"), CVar->GetString());
    return Result;
}

// ============================================================================
// BULK OUTLINER / CLEANUP
// ============================================================================

// Scan the given world for AStaticMeshActor with no StaticMesh set, destroy them.
// Uses UEditorActorSubsystem so the editor properly marks the level dirty.
// Returns count + names of deleted actors.
static int32 ScanAndDestroyNullSMA(UWorld* World, TArray<FString>& OutDeletedNames)
{
    if (!World) return 0;
    UEditorActorSubsystem* ActorSub = GEditor ? GEditor->GetEditorSubsystem<UEditorActorSubsystem>() : nullptr;

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Actors);
    int32 Deleted = 0;
    for (AActor* A : Actors)
    {
        AStaticMeshActor* SMA = Cast<AStaticMeshActor>(A);
        if (!SMA) continue;
        UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent();
        if (!SMC || !SMC->GetStaticMesh())
        {
            OutDeletedNames.Add(SMA->GetName());
            // Explicitly mark level dirty in case the subsystem path doesn't.
            if (ULevel* Lvl = SMA->GetLevel())
            {
                Lvl->MarkPackageDirty();
            }
            bool bOk = false;
            if (ActorSub)
            {
                bOk = ActorSub->DestroyActor(SMA);
            }
            if (!bOk)
            {
                // Fallback: World->DestroyActor with editor flags
                World->EditorDestroyActor(SMA, true);
            }
            ++Deleted;
        }
    }
    return Deleted;
}

// Save the currently loaded editor world's persistent level via the editor's
// proper save path (FEditorFileUtils::SaveLevel), which writes the .umap and
// clears the dirty flag.
static bool SaveCurrentEditorMap()
{
    UWorld* W = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!W || !W->PersistentLevel) return false;
    return FEditorFileUtils::SaveLevel(W->PersistentLevel, FString());
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleFixNullStaticMeshActors(const TSharedPtr<FJsonObject>& Params)
{
    // Parse params
    TArray<FString> MapPaths;
    const TArray<TSharedPtr<FJsonValue>>* MapPathsArray = nullptr;
    if (Params->TryGetArrayField(TEXT("map_paths"), MapPathsArray))
    {
        for (const TSharedPtr<FJsonValue>& V : *MapPathsArray)
        {
            FString S; if (V->TryGetString(S)) MapPaths.Add(S);
        }
    }
    bool bIncludePersistent = true;
    Params->TryGetBoolField(TEXT("include_persistent"), bIncludePersistent);
    bool bSave = true;
    Params->TryGetBoolField(TEXT("save"), bSave);
    bool bRestoreOriginal = true;
    Params->TryGetBoolField(TEXT("restore_original"), bRestoreOriginal);

    // Remember original map (long package name, e.g. /Game/.../ModernMansions)
    FString OriginalMapPath;
    if (UWorld* W = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr)
    {
        OriginalMapPath = W->GetOutermost()->GetName();
    }

    TArray<TSharedPtr<FJsonValue>> Reports;
    int32 GrandTotal = 0;

    auto AppendReport = [&Reports, &GrandTotal](const FString& MapPath, int32 N, bool bSaved, const TArray<FString>& Names, const FString& Err)
    {
        TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
        R->SetStringField(TEXT("map_path"), MapPath);
        R->SetNumberField(TEXT("deleted_count"), N);
        R->SetBoolField(TEXT("saved"), bSaved);
        if (!Err.IsEmpty()) R->SetStringField(TEXT("error"), Err);
        TArray<TSharedPtr<FJsonValue>> NameArr;
        for (const FString& N2 : Names) NameArr.Add(MakeShared<FJsonValueString>(N2));
        R->SetArrayField(TEXT("deleted_names"), NameArr);
        Reports.Add(MakeShared<FJsonValueObject>(R));
        GrandTotal += N;
    };

    // Persistent (current) map first
    if (bIncludePersistent)
    {
        UWorld* CurW = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        TArray<FString> Deleted;
        int32 N = ScanAndDestroyNullSMA(CurW, Deleted);
        bool bSaved = false;
        if (N > 0 && bSave) bSaved = SaveCurrentEditorMap();
        AppendReport(OriginalMapPath, N, bSaved, Deleted, FString());
    }

    // Sublevels: load each, scan, save, continue
    for (const FString& MapPath : MapPaths)
    {
        if (MapPath == OriginalMapPath) continue; // already done

        FString Filename;
        if (!FPackageName::TryConvertLongPackageNameToFilename(MapPath, Filename, FPackageName::GetMapPackageExtension()))
        {
            AppendReport(MapPath, 0, false, {}, TEXT("Could not resolve package path to filename"));
            continue;
        }

        // LoadMap: third arg shows progress; second arg is bLoadAsTemplate
        const bool bLoaded = FEditorFileUtils::LoadMap(Filename, false, false);
        if (!bLoaded)
        {
            AppendReport(MapPath, 0, false, {}, TEXT("FEditorFileUtils::LoadMap failed"));
            continue;
        }

        UWorld* W = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
        TArray<FString> Deleted;
        int32 N = ScanAndDestroyNullSMA(W, Deleted);
        bool bSaved = false;
        if (N > 0 && bSave) bSaved = SaveCurrentEditorMap();
        AppendReport(MapPath, N, bSaved, Deleted, FString());
    }

    // Restore original
    if (bRestoreOriginal && !OriginalMapPath.IsEmpty())
    {
        FString OriginalFilename;
        if (FPackageName::TryConvertLongPackageNameToFilename(OriginalMapPath, OriginalFilename, FPackageName::GetMapPackageExtension()))
        {
            FEditorFileUtils::LoadMap(OriginalFilename, false, false);
        }
    }

    TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
    ResultObj->SetNumberField(TEXT("total_deleted"), GrandTotal);
    ResultObj->SetStringField(TEXT("original_map"), OriginalMapPath);
    ResultObj->SetArrayField(TEXT("per_map"), Reports);
    return ResultObj;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleOrganizeOutlinerByClass(const TSharedPtr<FJsonObject>& Params)
{
    // Required: class_to_folder = { "StaticMeshActor": "Geometry", ... }
    // Optional: only_if_empty (default true) — only touch actors with no current folder
    //           skip_level_instance_content (default true) — skip actors not in the persistent level
    //           match_parent_classes (default true) — if exact class not found, walk up the class chain
    //           save (default true) — save the persistent map afterwards
    const TSharedPtr<FJsonObject>* MapObj = nullptr;
    if (!Params->TryGetObjectField(TEXT("class_to_folder"), MapObj))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'class_to_folder' parameter"));
    }

    TMap<FString, FString> ClassToFolder;
    for (const auto& Pair : (*MapObj)->Values)
    {
        FString V;
        if (Pair.Value->TryGetString(V) && !V.IsEmpty())
        {
            ClassToFolder.Add(Pair.Key, V);
        }
    }
    if (ClassToFolder.Num() == 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'class_to_folder' map is empty"));
    }

    bool bOnlyIfEmpty = true;
    Params->TryGetBoolField(TEXT("only_if_empty"), bOnlyIfEmpty);
    bool bSkipLI = true;
    Params->TryGetBoolField(TEXT("skip_level_instance_content"), bSkipLI);
    bool bMatchParents = true;
    Params->TryGetBoolField(TEXT("match_parent_classes"), bMatchParents);
    bool bSave = true;
    Params->TryGetBoolField(TEXT("save"), bSave);

    UWorld* W = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!W || !W->PersistentLevel)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world"));
    }

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(W, AActor::StaticClass(), Actors);

    int32 ChangedTotal = 0;
    int32 SkippedLI = 0;
    int32 SkippedNonEmpty = 0;
    int32 SkippedNoMatch = 0;
    TMap<FString, int32> CountsByFolder;

    for (AActor* A : Actors)
    {
        if (!A) continue;
        if (bSkipLI && A->GetLevel() != W->PersistentLevel)
        {
            SkippedLI++;
            continue;
        }

        const FString* FolderTarget = ClassToFolder.Find(A->GetClass()->GetName());
        if (!FolderTarget && bMatchParents)
        {
            for (UClass* C = A->GetClass()->GetSuperClass(); C && !FolderTarget; C = C->GetSuperClass())
            {
                FolderTarget = ClassToFolder.Find(C->GetName());
            }
        }
        if (!FolderTarget)
        {
            SkippedNoMatch++;
            continue;
        }

        if (bOnlyIfEmpty)
        {
            FString Cur = A->GetFolderPath().ToString();
            if (!Cur.IsEmpty() && Cur != TEXT("None"))
            {
                SkippedNonEmpty++;
                continue;
            }
        }

        A->SetFolderPath(FName(**FolderTarget));
        CountsByFolder.FindOrAdd(*FolderTarget)++;
        ChangedTotal++;
    }

    bool bSaved = false;
    if (ChangedTotal > 0)
    {
        W->PersistentLevel->MarkPackageDirty();
        if (bSave) bSaved = FEditorFileUtils::SaveLevel(W->PersistentLevel, FString());
    }

    TSharedPtr<FJsonObject> Result = MakeShared<FJsonObject>();
    Result->SetNumberField(TEXT("total_changed"), ChangedTotal);
    Result->SetNumberField(TEXT("skipped_level_instance_content"), SkippedLI);
    Result->SetNumberField(TEXT("skipped_already_in_folder"), SkippedNonEmpty);
    Result->SetNumberField(TEXT("skipped_no_class_match"), SkippedNoMatch);
    Result->SetBoolField(TEXT("saved"), bSaved);
    TSharedPtr<FJsonObject> ByFolder = MakeShared<FJsonObject>();
    for (const auto& P : CountsByFolder) ByFolder->SetNumberField(P.Key, P.Value);
    Result->SetObjectField(TEXT("by_folder"), ByFolder);
    return Result;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleListStaticMeshActors(const TSharedPtr<FJsonObject>& Params)
{
    // persistent_only: bool, default true — restrict to actors in the persistent level
    // folder_filter: optional string — only return actors whose current folder matches exactly
    UWorld* W = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!W) return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world"));

    bool bPersistentOnly = true;
    Params->TryGetBoolField(TEXT("persistent_only"), bPersistentOnly);
    FString FolderFilter;
    Params->TryGetStringField(TEXT("folder_filter"), FolderFilter);

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(W, AStaticMeshActor::StaticClass(), Actors);

    TArray<TSharedPtr<FJsonValue>> Arr;
    for (AActor* A : Actors)
    {
        AStaticMeshActor* SMA = Cast<AStaticMeshActor>(A);
        if (!SMA) continue;
        if (bPersistentOnly && SMA->GetLevel() != W->PersistentLevel) continue;

        FString CurFolder = SMA->GetFolderPath().ToString();
        if (!FolderFilter.IsEmpty() && CurFolder != FolderFilter) continue;

        UStaticMeshComponent* SMC = SMA->GetStaticMeshComponent();
        UStaticMesh* Mesh = SMC ? SMC->GetStaticMesh() : nullptr;

        TSharedPtr<FJsonObject> Item = MakeShared<FJsonObject>();
        Item->SetStringField(TEXT("name"), SMA->GetName());
        Item->SetStringField(TEXT("folder_path"), CurFolder);
        Item->SetStringField(TEXT("mesh_path"), Mesh ? Mesh->GetPathName() : FString());
        Item->SetStringField(TEXT("mesh_name"), Mesh ? Mesh->GetName() : FString());
        FVector L = SMA->GetActorLocation();
        TArray<TSharedPtr<FJsonValue>> Loc;
        Loc.Add(MakeShared<FJsonValueNumber>(L.X));
        Loc.Add(MakeShared<FJsonValueNumber>(L.Y));
        Loc.Add(MakeShared<FJsonValueNumber>(L.Z));
        Item->SetArrayField(TEXT("location"), Loc);
        Arr.Add(MakeShared<FJsonValueObject>(Item));
    }

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetArrayField(TEXT("actors"), Arr);
    R->SetNumberField(TEXT("count"), Arr.Num());
    return R;
}

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorFolders(const TSharedPtr<FJsonObject>& Params)
{
    // actor_to_folder: dict actor_name -> folder_path
    // save: bool, default true
    const TSharedPtr<FJsonObject>* MapObj = nullptr;
    if (!Params->TryGetObjectField(TEXT("actor_to_folder"), MapObj))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'actor_to_folder' parameter"));
    }
    TMap<FString, FString> NameToFolder;
    for (const auto& Pair : (*MapObj)->Values)
    {
        FString V;
        if (Pair.Value->TryGetString(V)) NameToFolder.Add(Pair.Key, V);
    }
    if (NameToFolder.Num() == 0)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("'actor_to_folder' map is empty"));
    }

    bool bSave = true;
    Params->TryGetBoolField(TEXT("save"), bSave);

    UWorld* W = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
    if (!W || !W->PersistentLevel)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("No editor world"));
    }

    TArray<AActor*> Actors;
    UGameplayStatics::GetAllActorsOfClass(W, AActor::StaticClass(), Actors);

    int32 Changed = 0;
    TSet<FString> Seen;
    for (AActor* A : Actors)
    {
        if (!A) continue;
        if (A->GetLevel() != W->PersistentLevel) continue;
        const FString* Tgt = NameToFolder.Find(A->GetName());
        if (!Tgt) continue;
        A->SetFolderPath(FName(**Tgt));
        Seen.Add(A->GetName());
        ++Changed;
    }

    int32 NotFound = 0;
    TArray<TSharedPtr<FJsonValue>> MissingArr;
    for (const auto& P : NameToFolder)
    {
        if (!Seen.Contains(P.Key))
        {
            ++NotFound;
            MissingArr.Add(MakeShared<FJsonValueString>(P.Key));
        }
    }

    bool bSaved = false;
    if (Changed > 0)
    {
        W->PersistentLevel->MarkPackageDirty();
        if (bSave) bSaved = FEditorFileUtils::SaveLevel(W->PersistentLevel, FString());
    }

    TSharedPtr<FJsonObject> R = MakeShared<FJsonObject>();
    R->SetNumberField(TEXT("changed"), Changed);
    R->SetNumberField(TEXT("not_found"), NotFound);
    R->SetArrayField(TEXT("missing_actor_names"), MissingArr);
    R->SetBoolField(TEXT("saved"), bSaved);
    return R;
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPEditorCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("get_actors_in_level"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_actors_in_level"), P); });
	Registry.RegisterCommand(TEXT("find_actors_by_name"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("find_actors_by_name"), P); });
	Registry.RegisterCommand(TEXT("spawn_actor"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("spawn_actor"), P); });
	Registry.RegisterCommand(TEXT("create_actor"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_actor"), P); });
	Registry.RegisterCommand(TEXT("delete_actor"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_actor"), P); });
	Registry.RegisterCommand(TEXT("set_actor_transform"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_actor_transform"), P); });
	Registry.RegisterCommand(TEXT("get_actor_properties"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_actor_properties"), P); });
	Registry.RegisterCommand(TEXT("set_actor_property"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_actor_property"), P); });
	Registry.RegisterCommand(TEXT("spawn_blueprint_actor"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("spawn_blueprint_actor"), P); });
	Registry.RegisterCommand(TEXT("focus_viewport"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("focus_viewport"), P); });
	Registry.RegisterCommand(TEXT("take_screenshot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("take_screenshot"), P); });
	Registry.RegisterCommand(TEXT("get_editor_context"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_editor_context"), P); });
	Registry.RegisterCommand(TEXT("get_viewport_camera"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_viewport_camera"), P); });
	Registry.RegisterCommand(TEXT("take_editor_screenshot"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("take_editor_screenshot"), P); });
	Registry.RegisterCommand(TEXT("get_cvar"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_cvar"), P); });
	Registry.RegisterCommand(TEXT("set_cvar"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_cvar"), P); });
	Registry.RegisterCommand(TEXT("get_actor_material_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_actor_material_info"), P); });
	Registry.RegisterCommand(TEXT("set_actor_material"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_actor_material"), P); });
	Registry.RegisterCommand(TEXT("fix_null_static_mesh_actors"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("fix_null_static_mesh_actors"), P); });
	Registry.RegisterCommand(TEXT("organize_outliner_by_class"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("organize_outliner_by_class"), P); });
	Registry.RegisterCommand(TEXT("list_static_mesh_actors"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("list_static_mesh_actors"), P); });
	Registry.RegisterCommand(TEXT("set_actor_folders"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_actor_folders"), P); });
}