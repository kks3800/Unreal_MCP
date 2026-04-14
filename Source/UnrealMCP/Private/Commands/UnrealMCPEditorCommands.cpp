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
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SpotLight.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "EditorSubsystem.h"
#include "Subsystems/EditorActorSubsystem.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
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

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSpawnActor(const TSharedPtr<FJsonObject>& Params)
{
    // Get required parameters
    FString ActorType;
    if (!Params->TryGetStringField(TEXT("type"), ActorType))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'type' parameter"));
    }

    // Get actor name (required parameter)
    FString ActorName;
    if (!Params->TryGetStringField(TEXT("name"), ActorName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'name' parameter"));
    }

    // Get optional transform parameters
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

    // Create the actor based on type
    AActor* NewActor = nullptr;
    UWorld* World = GEditor->GetEditorWorldContext().World();

    if (!World)
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to get editor world"));
    }

    // Check if an actor with this name already exists
    TArray<AActor*> AllActors;
    UGameplayStatics::GetAllActorsOfClass(World, AActor::StaticClass(), AllActors);
    for (AActor* Actor : AllActors)
    {
        if (Actor && Actor->GetName() == ActorName)
        {
            return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Actor with name '%s' already exists"), *ActorName));
        }
    }

    FActorSpawnParameters SpawnParams;
    SpawnParams.Name = *ActorName;

    if (ActorType == TEXT("StaticMeshActor"))
    {
        NewActor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, SpawnParams);

        // Assign a static mesh — use explicit param or default to engine cube
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
    }
    else if (ActorType == TEXT("PointLight"))
    {
        NewActor = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("SpotLight"))
    {
        NewActor = World->SpawnActor<ASpotLight>(ASpotLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("DirectionalLight"))
    {
        NewActor = World->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), Location, Rotation, SpawnParams);
    }
    else if (ActorType == TEXT("CameraActor"))
    {
        NewActor = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), Location, Rotation, SpawnParams);
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown actor type: %s"), *ActorType));
    }

    if (NewActor)
    {
        // Set scale (since SpawnActor only takes location and rotation)
        FTransform Transform = NewActor->GetTransform();
        Transform.SetScale3D(Scale);
        NewActor->SetActorTransform(Transform);

        // Return the created actor's details
        return FUnrealMCPCommonUtils::ActorToJsonObject(NewActor, true);
    }

    return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create actor"));
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
    return FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true);
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

TSharedPtr<FJsonObject> FUnrealMCPEditorCommands::HandleSetActorProperty(const TSharedPtr<FJsonObject>& Params)
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

    // Get property name
    FString PropertyName;
    if (!Params->TryGetStringField(TEXT("property_name"), PropertyName))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_name' parameter"));
    }

    // Get property value
    if (!Params->HasField(TEXT("property_value")))
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'property_value' parameter"));
    }
    
    TSharedPtr<FJsonValue> PropertyValue = Params->Values.FindRef(TEXT("property_value"));

    // Special case: FolderPath is an FName and must be set via SetFolderPath
    if (PropertyName == TEXT("FolderPath"))
    {
        FString FolderValue = PropertyValue->AsString();
        TargetActor->SetFolderPath(FName(*FolderValue));
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        return ResultObj;
    }

    // Set the property using our utility function
    FString ErrorMessage;
    if (FUnrealMCPCommonUtils::SetObjectProperty(TargetActor, PropertyName, PropertyValue, ErrorMessage))
    {
        // Property set successfully
        TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
        ResultObj->SetStringField(TEXT("actor"), ActorName);
        ResultObj->SetStringField(TEXT("property"), PropertyName);
        ResultObj->SetBoolField(TEXT("success"), true);
        
        // Also include the full actor details
        ResultObj->SetObjectField(TEXT("actor_details"), FUnrealMCPCommonUtils::ActorToJsonObject(TargetActor, true));
        return ResultObj;
    }
    else
    {
        return FUnrealMCPCommonUtils::CreateErrorResponse(ErrorMessage);
    }
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
}