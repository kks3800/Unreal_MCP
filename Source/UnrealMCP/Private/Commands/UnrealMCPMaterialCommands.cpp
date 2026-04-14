// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPMaterialCommands.h"
#include "Commands/UnrealMCPCommonUtils.h"
#include "MCPCore.h"
#include "Internationalization/Regex.h"

#if WITH_EDITOR
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "MaterialDomain.h"
#include "Materials/MaterialFunction.h"
#include "Materials/MaterialExpressionFunctionInput.h"
#include "Materials/MaterialExpressionFunctionOutput.h"
#include "Materials/MaterialExpressionMaterialFunctionCall.h"
#include "Factories/MaterialFunctionFactoryNew.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant2Vector.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionTextureObject.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionPower.h"
#include "Materials/MaterialExpressionSquareRoot.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionCeil.h"
#include "Materials/MaterialExpressionMin.h"
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionCosine.h"
#include "Materials/MaterialExpressionTime.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionRotator.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionDepthFade.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionCameraPositionWS.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Materials/MaterialExpressionPixelNormalWS.h"
#include "Materials/MaterialExpressionDotProduct.h"
#include "Materials/MaterialExpressionCrossProduct.h"
#include "Materials/MaterialExpressionNormalize.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionDesaturation.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionIf.h"
#include "Materials/MaterialExpressionStaticBool.h"
#include "Materials/MaterialExpressionStaticSwitch.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionParticleRelativeTime.h"
#include "Materials/MaterialExpressionParticleSize.h"
#include "Materials/MaterialExpressionParticleMacroUV.h"
#include "Materials/MaterialExpressionParticleSubUV.h"
#include "Materials/MaterialExpressionObjectRadius.h"
#include "Materials/MaterialExpressionObjectBounds.h"
#include "Materials/MaterialExpressionActorPositionWS.h"
#include "Materials/MaterialExpressionComment.h"
#include "Materials/MaterialExpressionCustom.h"
#include "Materials/MaterialExpressionReroute.h"
#include "Materials/MaterialExpressionNamedReroute.h"
#include "Materials/MaterialExpressionBumpOffset.h"
#include "EditorAssetLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Factories/MaterialFactoryNew.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "UObject/SavePackage.h"
#include "FileHelpers.h"
// Material preview / thumbnail (ThumbnailTools namespace is in ObjectTools.h via UnrealEd)
#include "Misc/ObjectThumbnail.h"
#include "ObjectTools.h"
// Image encoding and Base64
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#include "Misc/Base64.h"
#endif

//=============================================================================
// Helper Functions
//=============================================================================

UMaterial* FUnrealMCPMaterialCommands::LoadMaterial(const FString& MaterialName, FString& OutPath)
{
#if WITH_EDITOR
	// Try direct path first
	if (MaterialName.StartsWith(TEXT("/")))
	{
		OutPath = MaterialName;
		return Cast<UMaterial>(UEditorAssetLibrary::LoadAsset(MaterialName));
	}

	// Search in common locations
	TArray<FString> SearchPaths = {
		FString::Printf(TEXT("/Game/Materials/%s"), *MaterialName),
		FString::Printf(TEXT("/Game/Materials/%s.%s"), *MaterialName, *MaterialName),
		FString::Printf(TEXT("/Game/%s"), *MaterialName),
	};

	for (const FString& Path : SearchPaths)
	{
		if (UObject* Asset = UEditorAssetLibrary::LoadAsset(Path))
		{
			if (UMaterial* Material = Cast<UMaterial>(Asset))
			{
				OutPath = Path;
				return Material;
			}
		}
	}
#endif
	return nullptr;
}

UMaterialInstanceConstant* FUnrealMCPMaterialCommands::LoadMaterialInstance(const FString& InstanceName, FString& OutPath)
{
#if WITH_EDITOR
	if (InstanceName.StartsWith(TEXT("/")))
	{
		OutPath = InstanceName;
		return Cast<UMaterialInstanceConstant>(UEditorAssetLibrary::LoadAsset(InstanceName));
	}

	TArray<FString> SearchPaths = {
		FString::Printf(TEXT("/Game/Materials/%s"), *InstanceName),
		FString::Printf(TEXT("/Game/Materials/%s.%s"), *InstanceName, *InstanceName),
		FString::Printf(TEXT("/Game/%s"), *InstanceName),
	};

	for (const FString& Path : SearchPaths)
	{
		if (UObject* Asset = UEditorAssetLibrary::LoadAsset(Path))
		{
			if (UMaterialInstanceConstant* Instance = Cast<UMaterialInstanceConstant>(Asset))
			{
				OutPath = Path;
				return Instance;
			}
		}
	}
#endif
	return nullptr;
}

UMaterialExpression* FUnrealMCPMaterialCommands::FindExpressionByName(UMaterial* Material, const FString& NodeName)
{
#if WITH_EDITOR
	if (!Material)
	{
		return nullptr;
	}

	for (UMaterialExpression* Expr : Material->GetExpressions())
	{
		if (!Expr)
		{
			continue;
		}

		// Check parameter name for parameter expressions
		if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expr))
		{
			if (ScalarParam->ParameterName.ToString() == NodeName)
			{
				return Expr;
			}
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expr))
		{
			if (VectorParam->ParameterName.ToString() == NodeName)
			{
				return Expr;
			}
		}
		else if (UMaterialExpressionTextureSampleParameter* TextureParam = Cast<UMaterialExpressionTextureSampleParameter>(Expr))
		{
			if (TextureParam->ParameterName.ToString() == NodeName)
			{
				return Expr;
			}
		}
		else if (UMaterialExpressionComment* Comment = Cast<UMaterialExpressionComment>(Expr))
		{
			if (Comment->Text == NodeName)
			{
				return Expr;
			}
		}
		// Check Named Reroute Declaration by its Name property
		else if (UMaterialExpressionNamedRerouteDeclaration* NamedReroute = Cast<UMaterialExpressionNamedRerouteDeclaration>(Expr))
		{
			if (NamedReroute->Name.ToString() == NodeName)
			{
				return Expr;
			}
		}

		// Check description/generic name
		if (Expr->Desc == NodeName || Expr->GetName() == NodeName)
		{
			return Expr;
		}
	}
#endif
	return nullptr;
}

EMaterialDomain FUnrealMCPMaterialCommands::StringToMaterialDomain(const FString& DomainStr)
{
	if (DomainStr.Equals(TEXT("Surface"), ESearchCase::IgnoreCase)) return MD_Surface;
	if (DomainStr.Equals(TEXT("DeferredDecal"), ESearchCase::IgnoreCase)) return MD_DeferredDecal;
	if (DomainStr.Equals(TEXT("LightFunction"), ESearchCase::IgnoreCase)) return MD_LightFunction;
	if (DomainStr.Equals(TEXT("Volume"), ESearchCase::IgnoreCase)) return MD_Volume;
	if (DomainStr.Equals(TEXT("PostProcess"), ESearchCase::IgnoreCase)) return MD_PostProcess;
	if (DomainStr.Equals(TEXT("UI"), ESearchCase::IgnoreCase)) return MD_UI;
	return MD_Surface;
}

EBlendMode FUnrealMCPMaterialCommands::StringToBlendMode(const FString& BlendModeStr)
{
	if (BlendModeStr.Equals(TEXT("Opaque"), ESearchCase::IgnoreCase)) return BLEND_Opaque;
	if (BlendModeStr.Equals(TEXT("Masked"), ESearchCase::IgnoreCase)) return BLEND_Masked;
	if (BlendModeStr.Equals(TEXT("Translucent"), ESearchCase::IgnoreCase)) return BLEND_Translucent;
	if (BlendModeStr.Equals(TEXT("Additive"), ESearchCase::IgnoreCase)) return BLEND_Additive;
	if (BlendModeStr.Equals(TEXT("Modulate"), ESearchCase::IgnoreCase)) return BLEND_Modulate;
	if (BlendModeStr.Equals(TEXT("AlphaComposite"), ESearchCase::IgnoreCase)) return BLEND_AlphaComposite;
	if (BlendModeStr.Equals(TEXT("AlphaHoldout"), ESearchCase::IgnoreCase)) return BLEND_AlphaHoldout;
	return BLEND_Opaque;
}

EMaterialShadingModel FUnrealMCPMaterialCommands::StringToShadingModel(const FString& ShadingModelStr)
{
	if (ShadingModelStr.Equals(TEXT("Unlit"), ESearchCase::IgnoreCase)) return MSM_Unlit;
	if (ShadingModelStr.Equals(TEXT("DefaultLit"), ESearchCase::IgnoreCase)) return MSM_DefaultLit;
	if (ShadingModelStr.Equals(TEXT("Subsurface"), ESearchCase::IgnoreCase)) return MSM_Subsurface;
	if (ShadingModelStr.Equals(TEXT("PreintegratedSkin"), ESearchCase::IgnoreCase)) return MSM_PreintegratedSkin;
	if (ShadingModelStr.Equals(TEXT("SubsurfaceProfile"), ESearchCase::IgnoreCase)) return MSM_SubsurfaceProfile;
	if (ShadingModelStr.Equals(TEXT("TwoSidedFoliage"), ESearchCase::IgnoreCase)) return MSM_TwoSidedFoliage;
	if (ShadingModelStr.Equals(TEXT("Hair"), ESearchCase::IgnoreCase)) return MSM_Hair;
	if (ShadingModelStr.Equals(TEXT("Cloth"), ESearchCase::IgnoreCase)) return MSM_Cloth;
	if (ShadingModelStr.Equals(TEXT("Eye"), ESearchCase::IgnoreCase)) return MSM_Eye;
	if (ShadingModelStr.Equals(TEXT("SingleLayerWater"), ESearchCase::IgnoreCase)) return MSM_SingleLayerWater;
	if (ShadingModelStr.Equals(TEXT("ThinTranslucent"), ESearchCase::IgnoreCase)) return MSM_ThinTranslucent;
	if (ShadingModelStr.Equals(TEXT("ClearCoat"), ESearchCase::IgnoreCase)) return MSM_ClearCoat;
	return MSM_DefaultLit;
}

EMaterialProperty FUnrealMCPMaterialCommands::StringToMaterialProperty(const FString& PropertyStr)
{
	if (PropertyStr.Equals(TEXT("BaseColor"), ESearchCase::IgnoreCase)) return MP_BaseColor;
	if (PropertyStr.Equals(TEXT("Metallic"), ESearchCase::IgnoreCase)) return MP_Metallic;
	if (PropertyStr.Equals(TEXT("Specular"), ESearchCase::IgnoreCase)) return MP_Specular;
	if (PropertyStr.Equals(TEXT("Roughness"), ESearchCase::IgnoreCase)) return MP_Roughness;
	if (PropertyStr.Equals(TEXT("Anisotropy"), ESearchCase::IgnoreCase)) return MP_Anisotropy;
	if (PropertyStr.Equals(TEXT("Normal"), ESearchCase::IgnoreCase)) return MP_Normal;
	if (PropertyStr.Equals(TEXT("Tangent"), ESearchCase::IgnoreCase)) return MP_Tangent;
	if (PropertyStr.Equals(TEXT("EmissiveColor"), ESearchCase::IgnoreCase)) return MP_EmissiveColor;
	if (PropertyStr.Equals(TEXT("Emissive"), ESearchCase::IgnoreCase)) return MP_EmissiveColor;
	if (PropertyStr.Equals(TEXT("Opacity"), ESearchCase::IgnoreCase)) return MP_Opacity;
	if (PropertyStr.Equals(TEXT("OpacityMask"), ESearchCase::IgnoreCase)) return MP_OpacityMask;
	if (PropertyStr.Equals(TEXT("WorldPositionOffset"), ESearchCase::IgnoreCase)) return MP_WorldPositionOffset;
	if (PropertyStr.Equals(TEXT("SubsurfaceColor"), ESearchCase::IgnoreCase)) return MP_SubsurfaceColor;
	if (PropertyStr.Equals(TEXT("AmbientOcclusion"), ESearchCase::IgnoreCase)) return MP_AmbientOcclusion;
	if (PropertyStr.Equals(TEXT("Refraction"), ESearchCase::IgnoreCase)) return MP_Refraction;
	if (PropertyStr.Equals(TEXT("PixelDepthOffset"), ESearchCase::IgnoreCase)) return MP_PixelDepthOffset;
	if (PropertyStr.Equals(TEXT("ShadingModel"), ESearchCase::IgnoreCase)) return MP_ShadingModel;
	if (PropertyStr.Equals(TEXT("FrontMaterial"), ESearchCase::IgnoreCase)) return MP_FrontMaterial;
	if (PropertyStr.Equals(TEXT("SurfaceThickness"), ESearchCase::IgnoreCase)) return MP_SurfaceThickness;
	if (PropertyStr.Equals(TEXT("Displacement"), ESearchCase::IgnoreCase)) return MP_Displacement;
	return MP_BaseColor;
}

UClass* FUnrealMCPMaterialCommands::GetExpressionClassFromType(const FString& NodeType)
{
#if WITH_EDITOR
	// Universal dynamic class lookup using UE reflection
	// Constructs class name from node type and finds it at runtime
	// Works for ALL material expression types without hardcoding

	// Try standard naming: UMaterialExpression{NodeType}
	FString ClassName = FString::Printf(TEXT("MaterialExpression%s"), *NodeType);
	UClass* FoundClass = FindObject<UClass>(nullptr, *ClassName, EFindObjectFlags::ExactClass);

	// Verify it's a valid material expression class
	if (FoundClass && FoundClass->IsChildOf(UMaterialExpression::StaticClass()))
	{
		return FoundClass;
	}

	// Try with "/Script/Engine." prefix for full path lookup
	FString FullClassName = FString::Printf(TEXT("/Script/Engine.MaterialExpression%s"), *NodeType);
	FoundClass = FindObject<UClass>(nullptr, *FullClassName, EFindObjectFlags::None);

	if (FoundClass && FoundClass->IsChildOf(UMaterialExpression::StaticClass()))
	{
		return FoundClass;
	}

	// Special case aliases (only needed for user-friendly names)
	static TMap<FString, FString> Aliases;
	static bool bAliasesInitialized = false;
	if (!bAliasesInitialized)
	{
		Aliases.Add(TEXT("TexCoord"), TEXT("TextureCoordinate"));
		Aliases.Add(TEXT("Lerp"), TEXT("LinearInterpolate"));
		Aliases.Add(TEXT("Sqrt"), TEXT("SquareRoot"));
		Aliases.Add(TEXT("Sin"), TEXT("Sine"));
		Aliases.Add(TEXT("Cos"), TEXT("Cosine"));
		Aliases.Add(TEXT("Append"), TEXT("AppendVector"));
		Aliases.Add(TEXT("FunctionCall"), TEXT("MaterialFunctionCall"));
		bAliasesInitialized = true;
	}

	// Check if this is an alias
	if (const FString* AliasTarget = Aliases.Find(NodeType))
	{
		// Recursively resolve the alias
		return GetExpressionClassFromType(*AliasTarget);
	}

	// Fallback: iterate all loaded UMaterialExpression classes to find case-insensitive match
	for (TObjectIterator<UClass> It; It; ++It)
	{
		UClass* Class = *It;
		if (Class->IsChildOf(UMaterialExpression::StaticClass()) && !Class->HasAnyClassFlags(CLASS_Abstract))
		{
			// Extract class name after "UMaterialExpression" prefix
			FString ClassNameStr = Class->GetName();
			if (ClassNameStr.StartsWith(TEXT("MaterialExpression")))
			{
				FString ShortName = ClassNameStr.Mid(18); // Skip "MaterialExpression"
				if (ShortName.Equals(NodeType, ESearchCase::IgnoreCase))
				{
					return Class;
				}
			}
		}
	}
#endif
	return nullptr;
}

//=============================================================================
// New Enum Converters
//=============================================================================

ETranslucencyLightingMode FUnrealMCPMaterialCommands::StringToTranslucencyLightingMode(const FString& ModeStr)
{
	if (ModeStr.Equals(TEXT("VolumetricNonDirectional"), ESearchCase::IgnoreCase)) return TLM_VolumetricNonDirectional;
	if (ModeStr.Equals(TEXT("VolumetricDirectional"), ESearchCase::IgnoreCase)) return TLM_VolumetricDirectional;
	if (ModeStr.Equals(TEXT("VolumetricPerVertexNonDirectional"), ESearchCase::IgnoreCase)) return TLM_VolumetricPerVertexNonDirectional;
	if (ModeStr.Equals(TEXT("VolumetricPerVertexDirectional"), ESearchCase::IgnoreCase)) return TLM_VolumetricPerVertexDirectional;
	if (ModeStr.Equals(TEXT("Surface"), ESearchCase::IgnoreCase)) return TLM_Surface;
	if (ModeStr.Equals(TEXT("SurfacePerPixelLighting"), ESearchCase::IgnoreCase)) return TLM_SurfacePerPixelLighting;
	if (ModeStr.Equals(TEXT("SurfaceForwardShading"), ESearchCase::IgnoreCase)) return TLM_SurfacePerPixelLighting;
	return TLM_VolumetricNonDirectional;
}

ERefractionMode FUnrealMCPMaterialCommands::StringToRefractionMode(const FString& ModeStr)
{
	if (ModeStr.Equals(TEXT("IndexOfRefraction"), ESearchCase::IgnoreCase)) return RM_IndexOfRefraction;
	if (ModeStr.Equals(TEXT("PixelNormalOffset"), ESearchCase::IgnoreCase)) return RM_PixelNormalOffset;
	if (ModeStr.Equals(TEXT("2DOffset"), ESearchCase::IgnoreCase)) return RM_2DOffset;
	if (ModeStr.Equals(TEXT("None"), ESearchCase::IgnoreCase)) return RM_None;
	return RM_IndexOfRefraction;
}

EDecalBlendMode FUnrealMCPMaterialCommands::StringToDecalBlendMode(const FString& ModeStr)
{
	if (ModeStr.Equals(TEXT("Translucent"), ESearchCase::IgnoreCase)) return DBM_Translucent;
	if (ModeStr.Equals(TEXT("Stain"), ESearchCase::IgnoreCase)) return DBM_Stain;
	if (ModeStr.Equals(TEXT("Normal"), ESearchCase::IgnoreCase)) return DBM_Normal;
	if (ModeStr.Equals(TEXT("Emissive"), ESearchCase::IgnoreCase)) return DBM_Emissive;
	if (ModeStr.Equals(TEXT("DBuffer_ColorNormalRoughness"), ESearchCase::IgnoreCase)) return DBM_DBuffer_ColorNormalRoughness;
	if (ModeStr.Equals(TEXT("DBuffer_Color"), ESearchCase::IgnoreCase)) return DBM_DBuffer_Color;
	if (ModeStr.Equals(TEXT("DBuffer_ColorNormal"), ESearchCase::IgnoreCase)) return DBM_DBuffer_ColorNormal;
	if (ModeStr.Equals(TEXT("DBuffer_ColorRoughness"), ESearchCase::IgnoreCase)) return DBM_DBuffer_ColorRoughness;
	if (ModeStr.Equals(TEXT("DBuffer_Normal"), ESearchCase::IgnoreCase)) return DBM_DBuffer_Normal;
	if (ModeStr.Equals(TEXT("DBuffer_NormalRoughness"), ESearchCase::IgnoreCase)) return DBM_DBuffer_NormalRoughness;
	if (ModeStr.Equals(TEXT("DBuffer_Roughness"), ESearchCase::IgnoreCase)) return DBM_DBuffer_Roughness;
	if (ModeStr.Equals(TEXT("Volumetric_DistanceFunction"), ESearchCase::IgnoreCase)) return DBM_Volumetric_DistanceFunction;
	return DBM_Translucent;
}

EBlendableLocation FUnrealMCPMaterialCommands::StringToBlendableLocation(const FString& LocationStr)
{
#if ENGINE_MINOR_VERSION >= 4
	if (LocationStr.Equals(TEXT("SceneColorBeforeDOF"), ESearchCase::IgnoreCase)) return BL_SceneColorBeforeDOF;
	if (LocationStr.Equals(TEXT("BeforeTranslucency"), ESearchCase::IgnoreCase)) return BL_SceneColorBeforeDOF;
	if (LocationStr.Equals(TEXT("SceneColorAfterDOF"), ESearchCase::IgnoreCase)) return BL_SceneColorAfterDOF;
	if (LocationStr.Equals(TEXT("BeforeTonemapping"), ESearchCase::IgnoreCase)) return BL_SceneColorAfterDOF;
	if (LocationStr.Equals(TEXT("SSRInput"), ESearchCase::IgnoreCase)) return BL_SSRInput;
	if (LocationStr.Equals(TEXT("SceneColorBeforeBloom"), ESearchCase::IgnoreCase)) return BL_SceneColorBeforeBloom;
	if (LocationStr.Equals(TEXT("ReplacingTonemapper"), ESearchCase::IgnoreCase)) return BL_ReplacingTonemapper;
	if (LocationStr.Equals(TEXT("SceneColorAfterTonemapping"), ESearchCase::IgnoreCase)) return BL_SceneColorAfterTonemapping;
	if (LocationStr.Equals(TEXT("AfterTonemapping"), ESearchCase::IgnoreCase)) return BL_SceneColorAfterTonemapping;
	return BL_SceneColorAfterTonemapping;
#else
	if (LocationStr.Equals(TEXT("BeforeTranslucency"), ESearchCase::IgnoreCase)) return BL_BeforeTranslucency;
	if (LocationStr.Equals(TEXT("SceneColorBeforeDOF"), ESearchCase::IgnoreCase)) return BL_BeforeTranslucency;
	if (LocationStr.Equals(TEXT("BeforeTonemapping"), ESearchCase::IgnoreCase)) return BL_BeforeTonemapping;
	if (LocationStr.Equals(TEXT("SceneColorAfterDOF"), ESearchCase::IgnoreCase)) return BL_BeforeTonemapping;
	if (LocationStr.Equals(TEXT("SSRInput"), ESearchCase::IgnoreCase)) return BL_SSRInput;
	if (LocationStr.Equals(TEXT("SceneColorBeforeBloom"), ESearchCase::IgnoreCase)) return BL_BeforeTonemapping;
	if (LocationStr.Equals(TEXT("ReplacingTonemapper"), ESearchCase::IgnoreCase)) return BL_ReplacingTonemapper;
	if (LocationStr.Equals(TEXT("AfterTonemapping"), ESearchCase::IgnoreCase)) return BL_AfterTonemapping;
	if (LocationStr.Equals(TEXT("SceneColorAfterTonemapping"), ESearchCase::IgnoreCase)) return BL_AfterTonemapping;
	return BL_AfterTonemapping;
#endif
}

EMaterialStencilCompare FUnrealMCPMaterialCommands::StringToStencilCompare(const FString& CompareStr)
{
	if (CompareStr.Equals(TEXT("Less"), ESearchCase::IgnoreCase)) return MSC_Less;
	if (CompareStr.Equals(TEXT("LessEqual"), ESearchCase::IgnoreCase)) return MSC_LessEqual;
	if (CompareStr.Equals(TEXT("Greater"), ESearchCase::IgnoreCase)) return MSC_Greater;
	if (CompareStr.Equals(TEXT("GreaterEqual"), ESearchCase::IgnoreCase)) return MSC_GreaterEqual;
	if (CompareStr.Equals(TEXT("Equal"), ESearchCase::IgnoreCase)) return MSC_Equal;
	if (CompareStr.Equals(TEXT("NotEqual"), ESearchCase::IgnoreCase)) return MSC_NotEqual;
	if (CompareStr.Equals(TEXT("Never"), ESearchCase::IgnoreCase)) return MSC_Never;
	if (CompareStr.Equals(TEXT("Always"), ESearchCase::IgnoreCase)) return MSC_Always;
	return MSC_Always;
}

EMaterialTranslucencyPass FUnrealMCPMaterialCommands::StringToTranslucencyPass(const FString& PassStr)
{
	if (PassStr.Equals(TEXT("BeforeDOF"), ESearchCase::IgnoreCase)) return MTP_BeforeDOF;
	if (PassStr.Equals(TEXT("AfterDOF"), ESearchCase::IgnoreCase)) return MTP_AfterDOF;
	if (PassStr.Equals(TEXT("AfterMotionBlur"), ESearchCase::IgnoreCase)) return MTP_AfterMotionBlur;
	return MTP_BeforeDOF;
}

EFunctionInputType FUnrealMCPMaterialCommands::StringToFunctionInputType(const FString& TypeStr)
{
	if (TypeStr.Equals(TEXT("Scalar"), ESearchCase::IgnoreCase)) return FunctionInput_Scalar;
	if (TypeStr.Equals(TEXT("Vector2"), ESearchCase::IgnoreCase)) return FunctionInput_Vector2;
	if (TypeStr.Equals(TEXT("Vector3"), ESearchCase::IgnoreCase)) return FunctionInput_Vector3;
	if (TypeStr.Equals(TEXT("Vector4"), ESearchCase::IgnoreCase)) return FunctionInput_Vector4;
	if (TypeStr.Equals(TEXT("Texture2D"), ESearchCase::IgnoreCase)) return FunctionInput_Texture2D;
	if (TypeStr.Equals(TEXT("TextureCube"), ESearchCase::IgnoreCase)) return FunctionInput_TextureCube;
	if (TypeStr.Equals(TEXT("Texture2DArray"), ESearchCase::IgnoreCase)) return FunctionInput_Texture2DArray;
	if (TypeStr.Equals(TEXT("VolumeTexture"), ESearchCase::IgnoreCase)) return FunctionInput_VolumeTexture;
	if (TypeStr.Equals(TEXT("StaticBool"), ESearchCase::IgnoreCase)) return FunctionInput_StaticBool;
	if (TypeStr.Equals(TEXT("MaterialAttributes"), ESearchCase::IgnoreCase)) return FunctionInput_MaterialAttributes;
	if (TypeStr.Equals(TEXT("TextureExternal"), ESearchCase::IgnoreCase)) return FunctionInput_TextureExternal;
	if (TypeStr.Equals(TEXT("Bool"), ESearchCase::IgnoreCase)) return FunctionInput_Bool;
	return FunctionInput_Scalar;
}

//=============================================================================
// Material Function Helpers
//=============================================================================

UMaterialFunction* FUnrealMCPMaterialCommands::LoadMaterialFunction(const FString& FunctionName, FString& OutPath)
{
#if WITH_EDITOR
	if (FunctionName.StartsWith(TEXT("/")))
	{
		OutPath = FunctionName;
		return Cast<UMaterialFunction>(UEditorAssetLibrary::LoadAsset(FunctionName));
	}

	TArray<FString> SearchPaths = {
		FString::Printf(TEXT("/Game/Materials/Functions/%s"), *FunctionName),
		FString::Printf(TEXT("/Game/Materials/Functions/%s.%s"), *FunctionName, *FunctionName),
		FString::Printf(TEXT("/Game/Materials/%s"), *FunctionName),
		FString::Printf(TEXT("/Game/%s"), *FunctionName),
	};

	for (const FString& Path : SearchPaths)
	{
		if (UObject* Asset = UEditorAssetLibrary::LoadAsset(Path))
		{
			if (UMaterialFunction* Function = Cast<UMaterialFunction>(Asset))
			{
				OutPath = Path;
				return Function;
			}
		}
	}
#endif
	return nullptr;
}

UMaterialExpression* FUnrealMCPMaterialCommands::FindExpressionInFunction(UMaterialFunction* Function, const FString& NodeName)
{
#if WITH_EDITOR
	if (!Function)
	{
		return nullptr;
	}

	for (UMaterialExpression* Expr : Function->GetExpressions())
	{
		if (!Expr)
		{
			continue;
		}

		// Check FunctionInput name
		if (UMaterialExpressionFunctionInput* FuncInput = Cast<UMaterialExpressionFunctionInput>(Expr))
		{
			if (FuncInput->InputName.ToString() == NodeName)
			{
				return Expr;
			}
		}
		// Check FunctionOutput name
		else if (UMaterialExpressionFunctionOutput* FuncOutput = Cast<UMaterialExpressionFunctionOutput>(Expr))
		{
			if (FuncOutput->OutputName.ToString() == NodeName)
			{
				return Expr;
			}
		}
		// Check parameter name
		else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expr))
		{
			if (ScalarParam->ParameterName.ToString() == NodeName)
			{
				return Expr;
			}
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expr))
		{
			if (VectorParam->ParameterName.ToString() == NodeName)
			{
				return Expr;
			}
		}
		else if (UMaterialExpressionTextureSampleParameter* TextureParam = Cast<UMaterialExpressionTextureSampleParameter>(Expr))
		{
			if (TextureParam->ParameterName.ToString() == NodeName)
			{
				return Expr;
			}
		}

		// Check description/generic name
		if (Expr->Desc == NodeName || Expr->GetName() == NodeName)
		{
			return Expr;
		}
	}
#endif
	return nullptr;
}

//=============================================================================
// Apply Extended Material Properties
//=============================================================================

void FUnrealMCPMaterialCommands::ApplyExtendedProperties(UMaterial* Material, const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	if (!Material || !Params.IsValid())
	{
		return;
	}

	// Core Material
	double OpacityMaskClipValue;
	if (Params->TryGetNumberField(TEXT("opacity_mask_clip_value"), OpacityMaskClipValue))
	{
		Material->OpacityMaskClipValue = static_cast<float>(OpacityMaskClipValue);
	}

	bool bIsThinSurface;
	if (Params->TryGetBoolField(TEXT("is_thin_surface"), bIsThinSurface))
	{
		Material->bIsThinSurface = bIsThinSurface;
	}

	bool bUseMaterialAttributes;
	if (Params->TryGetBoolField(TEXT("use_material_attributes"), bUseMaterialAttributes))
	{
		Material->bUseMaterialAttributes = bUseMaterialAttributes;
	}

	bool bCastRayTracedShadows;
	if (Params->TryGetBoolField(TEXT("cast_ray_traced_shadows"), bCastRayTracedShadows))
	{
		Material->bCastRayTracedShadows = bCastRayTracedShadows;
	}

	bool bDitheredLODTransition;
	if (Params->TryGetBoolField(TEXT("dithered_lod_transition"), bDitheredLODTransition))
	{
		Material->DitheredLODTransition = bDitheredLODTransition;
	}

	bool bAllowNegativeEmissiveColor;
	if (Params->TryGetBoolField(TEXT("allow_negative_emissive_color"), bAllowNegativeEmissiveColor))
	{
		Material->bAllowNegativeEmissiveColor = bAllowNegativeEmissiveColor;
	}

	bool bTangentSpaceNormal;
	if (Params->TryGetBoolField(TEXT("tangent_space_normal"), bTangentSpaceNormal))
	{
		Material->bTangentSpaceNormal = bTangentSpaceNormal;
	}

	// Translucency
	FString TranslucencyLightingModeStr;
	if (Params->TryGetStringField(TEXT("translucency_lighting_mode"), TranslucencyLightingModeStr))
	{
		Material->TranslucencyLightingMode = StringToTranslucencyLightingMode(TranslucencyLightingModeStr);
	}

	double DirectionalLightingIntensity;
	if (Params->TryGetNumberField(TEXT("directional_lighting_intensity"), DirectionalLightingIntensity))
	{
		Material->TranslucencyDirectionalLightingIntensity = static_cast<float>(DirectionalLightingIntensity);
	}

	bool bScreenSpaceReflections;
	if (Params->TryGetBoolField(TEXT("screen_space_reflections"), bScreenSpaceReflections))
	{
		Material->bScreenSpaceReflections = bScreenSpaceReflections;
	}

	bool bContactShadows;
	if (Params->TryGetBoolField(TEXT("contact_shadows"), bContactShadows))
	{
		Material->bContactShadows = bContactShadows;
	}

	bool bApplyFogging;
	if (Params->TryGetBoolField(TEXT("apply_fogging"), bApplyFogging))
	{
		Material->bUseTranslucencyVertexFog = bApplyFogging;
	}

	bool bApplyCloudFogging;
	if (Params->TryGetBoolField(TEXT("apply_cloud_fogging"), bApplyCloudFogging))
	{
		Material->bApplyCloudFogging = bApplyCloudFogging;
	}

	bool bComputeFogPerPixel;
	if (Params->TryGetBoolField(TEXT("compute_fog_per_pixel"), bComputeFogPerPixel))
	{
		Material->bComputeFogPerPixel = bComputeFogPerPixel;
	}

	bool bOutputTranslucentVelocity;
	if (Params->TryGetBoolField(TEXT("output_translucent_velocity"), bOutputTranslucentVelocity))
	{
		Material->bOutputTranslucentVelocity = bOutputTranslucentVelocity;
	}

	bool bDisableDepthTest;
	if (Params->TryGetBoolField(TEXT("disable_depth_test"), bDisableDepthTest))
	{
		Material->bDisableDepthTest = bDisableDepthTest;
	}

	bool bWriteOnlyAlpha;
	if (Params->TryGetBoolField(TEXT("write_only_alpha"), bWriteOnlyAlpha))
	{
		Material->bWriteOnlyAlpha = bWriteOnlyAlpha;
	}

	bool bEnableResponsiveAA;
	if (Params->TryGetBoolField(TEXT("enable_responsive_aa"), bEnableResponsiveAA))
	{
		Material->bEnableResponsiveAA = bEnableResponsiveAA;
	}

	FString TranslucencyPassStr;
	if (Params->TryGetStringField(TEXT("translucency_pass"), TranslucencyPassStr))
	{
		Material->TranslucencyPass = StringToTranslucencyPass(TranslucencyPassStr);
	}

	// Translucency Self-Shadowing
	double TranslucentShadowDensityScale;
	if (Params->TryGetNumberField(TEXT("translucent_shadow_density_scale"), TranslucentShadowDensityScale))
	{
		Material->TranslucentShadowDensityScale = static_cast<float>(TranslucentShadowDensityScale);
	}

	double TranslucentSelfShadowDensityScale;
	if (Params->TryGetNumberField(TEXT("translucent_self_shadow_density_scale"), TranslucentSelfShadowDensityScale))
	{
		Material->TranslucentSelfShadowDensityScale = static_cast<float>(TranslucentSelfShadowDensityScale);
	}

	double TranslucentSelfShadowSecondDensityScale;
	if (Params->TryGetNumberField(TEXT("translucent_self_shadow_second_density_scale"), TranslucentSelfShadowSecondDensityScale))
	{
		Material->TranslucentSelfShadowSecondDensityScale = static_cast<float>(TranslucentSelfShadowSecondDensityScale);
	}

	double TranslucentSelfShadowSecondOpacity;
	if (Params->TryGetNumberField(TEXT("translucent_self_shadow_second_opacity"), TranslucentSelfShadowSecondOpacity))
	{
		Material->TranslucentSelfShadowSecondOpacity = static_cast<float>(TranslucentSelfShadowSecondOpacity);
	}

	double TranslucentBackscatteringExponent;
	if (Params->TryGetNumberField(TEXT("translucent_backscattering_exponent"), TranslucentBackscatteringExponent))
	{
		Material->TranslucentBackscatteringExponent = static_cast<float>(TranslucentBackscatteringExponent);
	}

	const TArray<TSharedPtr<FJsonValue>>* ExtinctionArray;
	if (Params->TryGetArrayField(TEXT("translucent_multiple_scattering_extinction"), ExtinctionArray) && ExtinctionArray->Num() >= 3)
	{
		float R = static_cast<float>((*ExtinctionArray)[0]->AsNumber());
		float G = static_cast<float>((*ExtinctionArray)[1]->AsNumber());
		float B = static_cast<float>((*ExtinctionArray)[2]->AsNumber());
		Material->TranslucentMultipleScatteringExtinction = FLinearColor(R, G, B);
	}

	double TranslucentShadowStartOffset;
	if (Params->TryGetNumberField(TEXT("translucent_shadow_start_offset"), TranslucentShadowStartOffset))
	{
		Material->TranslucentShadowStartOffset = static_cast<float>(TranslucentShadowStartOffset);
	}

	// Refraction
	FString RefractionModeStr;
	if (Params->TryGetStringField(TEXT("refraction_mode"), RefractionModeStr))
	{
		Material->RefractionMethod = StringToRefractionMode(RefractionModeStr);
	}

	FString RefractionCoverageModeStr;
	if (Params->TryGetStringField(TEXT("refraction_coverage_mode"), RefractionCoverageModeStr))
	{
		if (RefractionCoverageModeStr.Equals(TEXT("CoverageAccountedFor"), ESearchCase::IgnoreCase))
		{
			Material->RefractionCoverageMode = RCM_CoverageAccountedFor;
		}
		else
		{
			Material->RefractionCoverageMode = RCM_CoverageIgnored;
		}
	}

	// Post-Process
	FString BlendableLocationStr;
	if (Params->TryGetStringField(TEXT("blendable_location"), BlendableLocationStr))
	{
		Material->BlendableLocation = StringToBlendableLocation(BlendableLocationStr);
	}

	double BlendablePriority;
	if (Params->TryGetNumberField(TEXT("blendable_priority"), BlendablePriority))
	{
		Material->BlendablePriority = static_cast<int32>(BlendablePriority);
	}

	bool bBlendableOutputAlpha;
	if (Params->TryGetBoolField(TEXT("blendable_output_alpha"), bBlendableOutputAlpha))
	{
		Material->BlendableOutputAlpha = bBlendableOutputAlpha;
	}

	bool bEnableStencilTest;
	if (Params->TryGetBoolField(TEXT("enable_stencil_test"), bEnableStencilTest))
	{
		Material->bEnableStencilTest = bEnableStencilTest;
	}

	FString StencilCompareStr;
	if (Params->TryGetStringField(TEXT("stencil_compare"), StencilCompareStr))
	{
		Material->StencilCompare = StringToStencilCompare(StencilCompareStr);
	}

	double StencilRefValue;
	if (Params->TryGetNumberField(TEXT("stencil_ref_value"), StencilRefValue))
	{
		Material->StencilRefValue = static_cast<uint8>(FMath::Clamp(static_cast<int32>(StencilRefValue), 0, 255));
	}

	// Usage Flags
	bool bUsedWithSkeletalMesh;
	if (Params->TryGetBoolField(TEXT("used_with_skeletal_mesh"), bUsedWithSkeletalMesh))
	{
		Material->bUsedWithSkeletalMesh = bUsedWithSkeletalMesh;
	}

	bool bUsedWithParticleSprites;
	if (Params->TryGetBoolField(TEXT("used_with_particle_sprites"), bUsedWithParticleSprites))
	{
		Material->bUsedWithParticleSprites = bUsedWithParticleSprites;
	}

	bool bUsedWithNiagaraSprites;
	if (Params->TryGetBoolField(TEXT("used_with_niagara_sprites"), bUsedWithNiagaraSprites))
	{
		Material->bUsedWithNiagaraSprites = bUsedWithNiagaraSprites;
	}

	bool bUsedWithNiagaraMeshParticles;
	if (Params->TryGetBoolField(TEXT("used_with_niagara_mesh_particles"), bUsedWithNiagaraMeshParticles))
	{
		Material->bUsedWithNiagaraMeshParticles = bUsedWithNiagaraMeshParticles;
	}

	bool bUsedWithNiagaraRibbons;
	if (Params->TryGetBoolField(TEXT("used_with_niagara_ribbons"), bUsedWithNiagaraRibbons))
	{
		Material->bUsedWithNiagaraRibbons = bUsedWithNiagaraRibbons;
	}

	bool bUsedWithStaticLighting;
	if (Params->TryGetBoolField(TEXT("used_with_static_lighting"), bUsedWithStaticLighting))
	{
		Material->bUsedWithStaticLighting = bUsedWithStaticLighting;
	}

	bool bUsedWithMorphTargets;
	if (Params->TryGetBoolField(TEXT("used_with_morph_targets"), bUsedWithMorphTargets))
	{
		Material->bUsedWithMorphTargets = bUsedWithMorphTargets;
	}

	bool bUsedWithSplineMeshes;
	if (Params->TryGetBoolField(TEXT("used_with_spline_meshes"), bUsedWithSplineMeshes))
	{
		Material->bUsedWithSplineMeshes = bUsedWithSplineMeshes;
	}

	bool bUsedWithInstancedStaticMeshes;
	if (Params->TryGetBoolField(TEXT("used_with_instanced_static_meshes"), bUsedWithInstancedStaticMeshes))
	{
		Material->bUsedWithInstancedStaticMeshes = bUsedWithInstancedStaticMeshes;
	}

	bool bUsedWithClothing;
	if (Params->TryGetBoolField(TEXT("used_with_clothing"), bUsedWithClothing))
	{
		Material->bUsedWithClothing = bUsedWithClothing;
	}

	bool bUsedWithWater;
	if (Params->TryGetBoolField(TEXT("used_with_water"), bUsedWithWater))
	{
		Material->bUsedWithWater = bUsedWithWater;
	}

	bool bUsedWithHairStrands;
	if (Params->TryGetBoolField(TEXT("used_with_hair_strands"), bUsedWithHairStrands))
	{
		Material->bUsedWithHairStrands = bUsedWithHairStrands;
	}

	bool bUsedWithNanite;
	if (Params->TryGetBoolField(TEXT("used_with_nanite"), bUsedWithNanite))
	{
		Material->bUsedWithNanite = bUsedWithNanite;
	}

	bool bUsedWithVolumetricCloud;
	if (Params->TryGetBoolField(TEXT("used_with_volumetric_cloud"), bUsedWithVolumetricCloud))
	{
		Material->bUsedWithVolumetricCloud = bUsedWithVolumetricCloud;
	}

	bool bAutomaticallySetUsageInEditor;
	if (Params->TryGetBoolField(TEXT("automatically_set_usage_in_editor"), bAutomaticallySetUsageInEditor))
	{
		Material->bAutomaticallySetUsageInEditor = bAutomaticallySetUsageInEditor;
	}

	// Mobile
	bool bFullyRough;
	if (Params->TryGetBoolField(TEXT("fully_rough"), bFullyRough))
	{
		Material->bFullyRough = bFullyRough;
	}

	bool bUseLightmapDirectionality;
	if (Params->TryGetBoolField(TEXT("use_lightmap_directionality"), bUseLightmapDirectionality))
	{
		Material->bUseLightmapDirectionality = bUseLightmapDirectionality;
	}

	bool bUseAlphaToCoverage;
	if (Params->TryGetBoolField(TEXT("use_alpha_to_coverage"), bUseAlphaToCoverage))
	{
		Material->bUseAlphaToCoverage = bUseAlphaToCoverage;
	}

	// Advanced
	double NumCustomizedUVs;
	if (Params->TryGetNumberField(TEXT("num_customized_uvs"), NumCustomizedUVs))
	{
		Material->NumCustomizedUVs = FMath::Clamp(static_cast<int32>(NumCustomizedUVs), 0, 8);
	}

	bool bUseEmissiveForDynamicAreaLighting;
	if (Params->TryGetBoolField(TEXT("use_emissive_for_dynamic_area_lighting"), bUseEmissiveForDynamicAreaLighting))
	{
		Material->bUseEmissiveForDynamicAreaLighting = bUseEmissiveForDynamicAreaLighting;
	}

	bool bCastDynamicShadowAsMasked;
	if (Params->TryGetBoolField(TEXT("cast_dynamic_shadow_as_masked"), bCastDynamicShadowAsMasked))
	{
		Material->bCastDynamicShadowAsMasked = bCastDynamicShadowAsMasked;
	}
#endif
}

//=============================================================================
// Batch Helpers
//=============================================================================

/**
 * Resolve $N.field references in a JSON string value.
 * $N refers to the result of operation index N (zero-indexed).
 * $N.field extracts a specific field from that result.
 */
static FString ResolveMaterialBatchReference(const FString& Value, const TArray<TSharedPtr<FJsonValue>>& PreviousResults)
{
	// Quick check: does the value contain a $ reference?
	if (!Value.Contains(TEXT("$")))
	{
		return Value;
	}

	FString Result = Value;

	// Match patterns like $0.node_name, $1.function_name, $2, etc.
	FRegexPattern Pattern(TEXT("\\$(\\d+)(?:\\.(\\w+))?"));
	FRegexMatcher Matcher(Pattern, Value);

	while (Matcher.FindNext())
	{
		FString FullMatch = Matcher.GetCaptureGroup(0);
		FString IndexStr = Matcher.GetCaptureGroup(1);
		FString FieldName = Matcher.GetCaptureGroup(2);

		int32 Index = FCString::Atoi(*IndexStr);
		if (Index >= 0 && Index < PreviousResults.Num())
		{
			const TSharedPtr<FJsonObject>* ResultObj = nullptr;
			if (PreviousResults[Index]->TryGetObject(ResultObj) && ResultObj && ResultObj->IsValid())
			{
				if (FieldName.IsEmpty())
				{
					// $N without field - try node_name as default
					FString NodeName;
					if ((*ResultObj)->TryGetStringField(TEXT("node_name"), NodeName))
					{
						Result = Result.Replace(*FullMatch, *NodeName);
					}
				}
				else
				{
					// $N.field - extract specific field
					FString FieldValue;
					if ((*ResultObj)->TryGetStringField(*FieldName, FieldValue))
					{
						Result = Result.Replace(*FullMatch, *FieldValue);
					}
				}
			}
		}
	}

	return Result;
}

/** Recursively resolve $N references in all string fields of a JSON object. */
static void ResolveMaterialReferencesInObject(TSharedPtr<FJsonObject>& Obj, const TArray<TSharedPtr<FJsonValue>>& PreviousResults)
{
	TArray<FString> Keys;
	Obj->Values.GetKeys(Keys);

	for (const FString& Key : Keys)
	{
		TSharedPtr<FJsonValue> Val = Obj->Values[Key];
		if (Val->Type == EJson::String)
		{
			FString Resolved = ResolveMaterialBatchReference(Val->AsString(), PreviousResults);
			if (Resolved != Val->AsString())
			{
				Obj->SetStringField(Key, Resolved);
			}
		}
		else if (Val->Type == EJson::Object)
		{
			TSharedPtr<FJsonObject> SubObj = Val->AsObject();
			ResolveMaterialReferencesInObject(SubObj, PreviousResults);
		}
	}
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleExecuteMaterialBatch(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' parameter"));
	}

	const TArray<TSharedPtr<FJsonValue>>* OperationsArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("operations"), OperationsArray))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'operations' array"));
	}

	bool bAutoRecompile = true;
	if (Params->HasField(TEXT("auto_recompile")))
	{
		bAutoRecompile = Params->GetBoolField(TEXT("auto_recompile"));
	}

	bool bAutoSave = false;
	if (Params->HasField(TEXT("auto_save")))
	{
		bAutoSave = Params->GetBoolField(TEXT("auto_save"));
	}

	TArray<TSharedPtr<FJsonValue>> Results;
	int32 SuccessCount = 0;
	int32 FailCount = 0;

	// Execute each operation sequentially
	for (int32 OpIndex = 0; OpIndex < OperationsArray->Num(); ++OpIndex)
	{
		const TSharedPtr<FJsonValue>& OpValue = (*OperationsArray)[OpIndex];

		const TSharedPtr<FJsonObject>* OpObject = nullptr;
		if (!OpValue->TryGetObject(OpObject))
		{
			FailCount++;
			TSharedPtr<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
			ErrorResult->SetBoolField(TEXT("success"), false);
			ErrorResult->SetStringField(TEXT("error"), FString::Printf(TEXT("Operation %d: invalid object"), OpIndex));
			Results.Add(MakeShared<FJsonValueObject>(ErrorResult));
			continue;
		}

		FString OpCommand;
		if (!(*OpObject)->TryGetStringField(TEXT("op"), OpCommand))
		{
			FailCount++;
			TSharedPtr<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
			ErrorResult->SetBoolField(TEXT("success"), false);
			ErrorResult->SetStringField(TEXT("error"), FString::Printf(TEXT("Operation %d: missing 'op' field"), OpIndex));
			Results.Add(MakeShared<FJsonValueObject>(ErrorResult));
			continue;
		}

		// Build params: copy all fields except 'op', inject material_name
		TSharedPtr<FJsonObject> OpParams = MakeShared<FJsonObject>();
		for (const auto& Field : (*OpObject)->Values)
		{
			if (Field.Key != TEXT("op"))
			{
				OpParams->SetField(Field.Key, Field.Value);
			}
		}
		OpParams->SetStringField(TEXT("material_name"), MaterialName);

		// Resolve $N references using previous results
		ResolveMaterialReferencesInObject(OpParams, Results);

		// Route through the global command registry
		TSharedPtr<FJsonObject> OpResult;
		FMCPCommandRegistry& Registry = FMCPCommandRegistry::Get();
		if (Registry.HasCommand(OpCommand))
		{
			OpResult = Registry.ExecuteCommand(OpCommand, OpParams);
		}
		else
		{
			OpResult = FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Unknown batch operation: %s"), *OpCommand));
		}

		// Check success
		bool bOpSuccess = false;
		if (OpResult.IsValid())
		{
			OpResult->TryGetBoolField(TEXT("success"), bOpSuccess);
			// If no explicit success field, check for error field
			if (!OpResult->HasField(TEXT("success")))
			{
				bOpSuccess = !OpResult->HasField(TEXT("error"));
			}
		}

		if (bOpSuccess)
		{
			SuccessCount++;
		}
		else
		{
			FailCount++;
		}

		Results.Add(MakeShared<FJsonValueObject>(OpResult));
	}

	// Auto-recompile once at the end (cheaper than per-op recompile)
	if (bAutoRecompile)
	{
		TSharedPtr<FJsonObject> RecompileParams = MakeShared<FJsonObject>();
		RecompileParams->SetStringField(TEXT("material_name"), MaterialName);
		HandleRecompileMaterial(RecompileParams);
	}

	// Auto-save — support both material and function targets
	if (bAutoSave)
	{
		FString TryPath;
		if (UMaterial* SaveMat = LoadMaterial(MaterialName, TryPath))
		{
			UEditorAssetLibrary::SaveLoadedAsset(SaveMat);
		}
		else if (UMaterialFunction* SaveFunc = LoadMaterialFunction(MaterialName, TryPath))
		{
			UEditorAssetLibrary::SaveLoadedAsset(SaveFunc);
		}
	}

	Response->SetBoolField(TEXT("success"), FailCount == 0);
	Response->SetNumberField(TEXT("success_count"), SuccessCount);
	Response->SetNumberField(TEXT("fail_count"), FailCount);
	Response->SetNumberField(TEXT("total_operations"), OperationsArray->Num());
	Response->SetArrayField(TEXT("results"), Results);
	return Response;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("execute_material_batch requires editor build"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleExecuteFunctionBatch(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	TSharedPtr<FJsonObject> Response = MakeShared<FJsonObject>();

	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'function_name' parameter"));
	}

	const TArray<TSharedPtr<FJsonValue>>* OperationsArray = nullptr;
	if (!Params->TryGetArrayField(TEXT("operations"), OperationsArray))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'operations' array"));
	}

	bool bAutoSave = false;
	if (Params->HasField(TEXT("auto_save")))
	{
		bAutoSave = Params->GetBoolField(TEXT("auto_save"));
	}

	TArray<TSharedPtr<FJsonValue>> Results;
	int32 SuccessCount = 0;
	int32 FailCount = 0;

	// Execute each operation sequentially
	for (int32 OpIndex = 0; OpIndex < OperationsArray->Num(); ++OpIndex)
	{
		const TSharedPtr<FJsonValue>& OpValue = (*OperationsArray)[OpIndex];

		const TSharedPtr<FJsonObject>* OpObject = nullptr;
		if (!OpValue->TryGetObject(OpObject))
		{
			FailCount++;
			TSharedPtr<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
			ErrorResult->SetBoolField(TEXT("success"), false);
			ErrorResult->SetStringField(TEXT("error"), FString::Printf(TEXT("Operation %d: invalid object"), OpIndex));
			Results.Add(MakeShared<FJsonValueObject>(ErrorResult));
			continue;
		}

		FString OpCommand;
		if (!(*OpObject)->TryGetStringField(TEXT("op"), OpCommand))
		{
			FailCount++;
			TSharedPtr<FJsonObject> ErrorResult = MakeShared<FJsonObject>();
			ErrorResult->SetBoolField(TEXT("success"), false);
			ErrorResult->SetStringField(TEXT("error"), FString::Printf(TEXT("Operation %d: missing 'op' field"), OpIndex));
			Results.Add(MakeShared<FJsonValueObject>(ErrorResult));
			continue;
		}

		// Build params: copy all fields except 'op', inject function_name
		TSharedPtr<FJsonObject> OpParams = MakeShared<FJsonObject>();
		for (const auto& Field : (*OpObject)->Values)
		{
			if (Field.Key != TEXT("op"))
			{
				OpParams->SetField(Field.Key, Field.Value);
			}
		}
		OpParams->SetStringField(TEXT("function_name"), FunctionName);

		// Resolve $N references using previous results
		ResolveMaterialReferencesInObject(OpParams, Results);

		// Route through the global command registry
		TSharedPtr<FJsonObject> OpResult;
		FMCPCommandRegistry& Registry = FMCPCommandRegistry::Get();
		if (Registry.HasCommand(OpCommand))
		{
			OpResult = Registry.ExecuteCommand(OpCommand, OpParams);
		}
		else
		{
			OpResult = FUnrealMCPCommonUtils::CreateErrorResponse(
				FString::Printf(TEXT("Unknown batch operation: %s"), *OpCommand));
		}

		// Check success
		bool bOpSuccess = false;
		if (OpResult.IsValid())
		{
			OpResult->TryGetBoolField(TEXT("success"), bOpSuccess);
			// If no explicit success field, check for error field
			if (!OpResult->HasField(TEXT("success")))
			{
				bOpSuccess = !OpResult->HasField(TEXT("error"));
			}
		}

		if (bOpSuccess)
		{
			SuccessCount++;
		}
		else
		{
			FailCount++;
		}

		Results.Add(MakeShared<FJsonValueObject>(OpResult));
	}

	// Auto-save
	if (bAutoSave)
	{
		FString FunctionPath;
		UMaterialFunction* Function = LoadMaterialFunction(FunctionName, FunctionPath);
		if (Function)
		{
			UEditorAssetLibrary::SaveLoadedAsset(Function);
		}
	}

	Response->SetBoolField(TEXT("success"), FailCount == 0);
	Response->SetNumberField(TEXT("success_count"), SuccessCount);
	Response->SetNumberField(TEXT("fail_count"), FailCount);
	Response->SetNumberField(TEXT("total_operations"), OperationsArray->Num());
	Response->SetArrayField(TEXT("results"), Results);
	return Response;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("execute_function_batch requires editor build"));
#endif
}

//=============================================================================
// HandleBuildMaterial — create/clear + add nodes + wire + recompile in one call
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleBuildMaterial(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("build_material requires 'material_name'"));
	}

	// --- 1. Get or create the material asset ---
	FString MaterialPath;
	UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
	if (!Material)
	{
		// Extract name and path from material_name
		FString AssetName = MaterialName;
		FString AssetPath = TEXT("/Game/Materials");
		int32 LastSlash = INDEX_NONE;
		if (MaterialName.FindLastChar(TEXT('/'), LastSlash))
		{
			AssetPath = MaterialName.Left(LastSlash);
			AssetName = MaterialName.Mid(LastSlash + 1);
		}

		TSharedPtr<FJsonObject> CreateParams = MakeShared<FJsonObject>();
		CreateParams->SetStringField(TEXT("material_name"), AssetName);
		CreateParams->SetStringField(TEXT("path"), AssetPath);

		FString Domain, BlendMode, ShadingModel;
		if (Params->TryGetStringField(TEXT("domain"), Domain)) CreateParams->SetStringField(TEXT("domain"), Domain);
		if (Params->TryGetStringField(TEXT("blend_mode"), BlendMode)) CreateParams->SetStringField(TEXT("blend_mode"), BlendMode);
		if (Params->TryGetStringField(TEXT("shading_model"), ShadingModel)) CreateParams->SetStringField(TEXT("shading_model"), ShadingModel);

		HandleCreateMaterial(CreateParams);
		Material = LoadMaterial(MaterialName, MaterialPath);
		if (!Material)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("build_material: failed to create or load material"));
		}
	}

	// --- 1b. Apply material properties (domain/blend/shading) for existing materials ---
	{
		FString DomainStr;
		if (Params->TryGetStringField(TEXT("domain"), DomainStr)) { Material->MaterialDomain = StringToMaterialDomain(DomainStr); }
		FString BlendModeStr;
		if (Params->TryGetStringField(TEXT("blend_mode"), BlendModeStr)) { Material->BlendMode = StringToBlendMode(BlendModeStr); }
		FString ShadingModelStr;
		if (Params->TryGetStringField(TEXT("shading_model"), ShadingModelStr)) { Material->SetShadingModel(StringToShadingModel(ShadingModelStr)); }
		bool bTwoSided = false;
		if (Params->TryGetBoolField(TEXT("two_sided"), bTwoSided)) { Material->TwoSided = bTwoSided; }
	}

	// --- 2. Clear existing graph ---
	UMaterialEditingLibrary::DeleteAllMaterialExpressions(Material);

	// --- 3. Build node map ---
	const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
	TMap<FString, UMaterialExpression*> NodeMap;
	int32 NodesCreated = 0;
	TArray<FString> CompileErrors;

	if (Params->TryGetArrayField(TEXT("nodes"), NodesArray))
	{
		for (int32 i = 0; i < NodesArray->Num(); i++)
		{
			const TSharedPtr<FJsonObject>* NodeObj = nullptr;
			if (!(*NodesArray)[i]->TryGetObject(NodeObj)) { continue; }

			FString NodeName, NodeType;
			(*NodeObj)->TryGetStringField(TEXT("name"), NodeName);
			(*NodeObj)->TryGetStringField(TEXT("type"), NodeType);

			// Default grid layout, overridden by explicit position
			int32 NodeX = -1200 + (i % 5) * 200;
			int32 NodeY = (i / 5) * 150;
			const TArray<TSharedPtr<FJsonValue>>* PosArray = nullptr;
			if ((*NodeObj)->TryGetArrayField(TEXT("position"), PosArray) && PosArray->Num() >= 2)
			{
				NodeX = static_cast<int32>((*PosArray)[0]->AsNumber());
				NodeY = static_cast<int32>((*PosArray)[1]->AsNumber());
			}

			// Use the existing dynamic class lookup
			UClass* ExprClass = GetExpressionClassFromType(NodeType);
			if (!ExprClass)
			{
				CompileErrors.Add(FString::Printf(TEXT("Unknown node type: %s (skipped)"), *NodeType));
				continue;
			}

			UMaterialExpression* Expr = UMaterialEditingLibrary::CreateMaterialExpression(Material, ExprClass, NodeX, NodeY);
			if (!Expr)
			{
				CompileErrors.Add(FString::Printf(TEXT("Failed to create node: %s (type %s)"), *NodeName, *NodeType));
				continue;
			}

			// Apply per-type properties
			if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expr))
			{
				ScalarParam->ParameterName = FName(*NodeName);
				double Val = 0.0;
				if ((*NodeObj)->TryGetNumberField(TEXT("value"), Val)) { ScalarParam->DefaultValue = static_cast<float>(Val); }
			}
			else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expr))
			{
				VectorParam->ParameterName = FName(*NodeName);
				const TArray<TSharedPtr<FJsonValue>>* ColorArr = nullptr;
				if ((*NodeObj)->TryGetArrayField(TEXT("color"), ColorArr) && ColorArr->Num() >= 3)
				{
					VectorParam->DefaultValue = FLinearColor(
						static_cast<float>((*ColorArr)[0]->AsNumber()),
						static_cast<float>((*ColorArr)[1]->AsNumber()),
						static_cast<float>((*ColorArr)[2]->AsNumber()),
						ColorArr->Num() >= 4 ? static_cast<float>((*ColorArr)[3]->AsNumber()) : 1.0f);
				}
			}
			else if (UMaterialExpressionTextureSampleParameter2D* TexParam = Cast<UMaterialExpressionTextureSampleParameter2D>(Expr))
			{
				TexParam->ParameterName = FName(*NodeName);
				FString TexPath;
				if ((*NodeObj)->TryGetStringField(TEXT("texture_path"), TexPath))
				{
					UTexture* Tex = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(TexPath));
					if (Tex) { TexParam->Texture = Tex; }
				}
			}
			else if (UMaterialExpressionConstant* ConstParam = Cast<UMaterialExpressionConstant>(Expr))
			{
				double Val = 0.0;
				if ((*NodeObj)->TryGetNumberField(TEXT("value"), Val)) { ConstParam->R = static_cast<float>(Val); }
			}

			// Set description for non-parameter nodes so FindExpressionByName can locate them
			if (NodeName.Len() > 0 && !Cast<UMaterialExpressionScalarParameter>(Expr)
				&& !Cast<UMaterialExpressionVectorParameter>(Expr)
				&& !Cast<UMaterialExpressionTextureSampleParameter>(Expr))
			{
				Expr->Desc = NodeName;
			}

			if (!NodeName.IsEmpty())
			{
				NodeMap.Add(NodeName, Expr);
				NodesCreated++;
			}
		}
	}

	// --- 4. Wire node-to-node connections ---
	int32 ConnectionsMade = 0;
	const TArray<TSharedPtr<FJsonValue>>* ConnArray = nullptr;
	if (Params->TryGetArrayField(TEXT("connections"), ConnArray))
	{
		for (const TSharedPtr<FJsonValue>& ConnVal : *ConnArray)
		{
			const TSharedPtr<FJsonObject>* ConnObj = nullptr;
			if (!ConnVal->TryGetObject(ConnObj)) { continue; }

			FString FromName, ToName, InputPin, OutputPin;
			(*ConnObj)->TryGetStringField(TEXT("from"), FromName);
			(*ConnObj)->TryGetStringField(TEXT("to"), ToName);
			(*ConnObj)->TryGetStringField(TEXT("input"), InputPin);
			(*ConnObj)->TryGetStringField(TEXT("output"), OutputPin);

			UMaterialExpression** FromExpr = NodeMap.Find(FromName);
			UMaterialExpression** ToExpr = NodeMap.Find(ToName);
			if (!FromExpr || !ToExpr)
			{
				CompileErrors.Add(FString::Printf(TEXT("Connection skipped: node not found (%s -> %s)"), *FromName, *ToName));
				continue;
			}
			if (UMaterialEditingLibrary::ConnectMaterialExpressions(*FromExpr, OutputPin, *ToExpr, InputPin))
			{
				ConnectionsMade++;
			}
			else
			{
				CompileErrors.Add(FString::Printf(TEXT("Connection failed: %s.%s -> %s.%s"), *FromName, *OutputPin, *ToName, *InputPin));
			}
		}
	}

	// --- 5. Wire to material outputs ---
	int32 OutputsWired = 0;
	const TArray<TSharedPtr<FJsonValue>>* OutArray = nullptr;
	if (Params->TryGetArrayField(TEXT("outputs"), OutArray))
	{
		for (const TSharedPtr<FJsonValue>& OutVal : *OutArray)
		{
			const TSharedPtr<FJsonObject>* OutObj = nullptr;
			if (!OutVal->TryGetObject(OutObj)) { continue; }

			FString FromName, PropertyName, OutputPin;
			(*OutObj)->TryGetStringField(TEXT("from"), FromName);
			(*OutObj)->TryGetStringField(TEXT("property"), PropertyName);
			(*OutObj)->TryGetStringField(TEXT("output"), OutputPin);

			UMaterialExpression** FromExpr = NodeMap.Find(FromName);
			if (!FromExpr)
			{
				CompileErrors.Add(FString::Printf(TEXT("Output skipped: node not found (%s)"), *FromName));
				continue;
			}

			EMaterialProperty Prop = StringToMaterialProperty(PropertyName);
			if (UMaterialEditingLibrary::ConnectMaterialProperty(*FromExpr, OutputPin, Prop))
			{
				OutputsWired++;
			}
			else
			{
				CompileErrors.Add(FString::Printf(TEXT("Output wire failed: %s -> %s"), *FromName, *PropertyName));
			}
		}
	}

	// --- 6. Recompile ---
	bool bAutoRecompile = true;
	Params->TryGetBoolField(TEXT("auto_recompile"), bAutoRecompile);
	if (bAutoRecompile)
	{
		UMaterialEditingLibrary::RecompileMaterial(Material);
	}

	// --- 7. Build response ---
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("material_path"), MaterialPath.IsEmpty() ? MaterialName : MaterialPath);
	ResultObj->SetNumberField(TEXT("nodes_created"), NodesCreated);
	ResultObj->SetNumberField(TEXT("connections_made"), ConnectionsMade);
	ResultObj->SetNumberField(TEXT("outputs_wired"), OutputsWired);

	TArray<TSharedPtr<FJsonValue>> ErrorValues;
	for (const FString& Err : CompileErrors)
	{
		ErrorValues.Add(MakeShared<FJsonValueString>(Err));
	}
	ResultObj->SetArrayField(TEXT("compile_errors"), ErrorValues);

	// --- 8. Optional preview ---
	bool bIncludePreview = false;
	Params->TryGetBoolField(TEXT("include_preview"), bIncludePreview);
	if (bIncludePreview)
	{
		TSharedPtr<FJsonObject> PreviewParams = MakeShared<FJsonObject>();
		PreviewParams->SetStringField(TEXT("material_name"), MaterialName);
		PreviewParams->SetNumberField(TEXT("width"), 256);
		PreviewParams->SetNumberField(TEXT("height"), 256);
		TSharedPtr<FJsonObject> PreviewResult = HandleGetMaterialPreview(PreviewParams);
		FString PreviewBase64;
		if (PreviewResult.IsValid() && PreviewResult->TryGetStringField(TEXT("preview_base64"), PreviewBase64))
		{
			ResultObj->SetStringField(TEXT("preview_base64"), PreviewBase64);
		}
	}

	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("build_material requires editor build"));
#endif
}

//=============================================================================
// HandleGetMaterialPreview — render and return base64-encoded PNG thumbnail
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleGetMaterialPreview(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("get_material_preview requires 'material_name'"));
	}

	double Width = 256.0;
	double Height = 256.0;
	Params->TryGetNumberField(TEXT("width"), Width);
	Params->TryGetNumberField(TEXT("height"), Height);

	FString MaterialPath;
	UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("get_material_preview: material not found: %s"), *MaterialName));
	}

	// Try cached thumbnail first; render fresh if missing or empty
	const FObjectThumbnail* ExistingThumb = ThumbnailTools::FindCachedThumbnail(*Material->GetFullName());
	FObjectThumbnail RenderedThumb;
	if (!ExistingThumb || ExistingThumb->GetImageWidth() == 0)
	{
		ThumbnailTools::RenderThumbnail(
			Material,
			static_cast<uint32>(Width),
			static_cast<uint32>(Height),
			ThumbnailTools::EThumbnailTextureFlushMode::AlwaysFlush,
			nullptr,
			&RenderedThumb);
		ExistingThumb = &RenderedThumb;
	}

	if (!ExistingThumb || ExistingThumb->GetImageWidth() == 0)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			TEXT("get_material_preview: thumbnail not available; recompile the material first"));
	}

	const TArray<uint8>& RawData = ExistingThumb->GetUncompressedImageData();
	int32 ThumbW = ExistingThumb->GetImageWidth();
	int32 ThumbH = ExistingThumb->GetImageHeight();

	if (RawData.IsEmpty())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("get_material_preview: thumbnail has no pixel data"));
	}

	// Encode raw BGRA pixels to PNG
	IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(TEXT("ImageWrapper"));
	TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);
	ImageWrapper->SetRaw(RawData.GetData(), RawData.Num(), ThumbW, ThumbH, ERGBFormat::BGRA, 8);

	TArray64<uint8> PNGData = ImageWrapper->GetCompressed(100);

	check(PNGData.Num() <= static_cast<int64>(TNumericLimits<uint32>::Max()));
	FString Base64 = FBase64::Encode(PNGData.GetData(), static_cast<uint32>(PNGData.Num()));

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetNumberField(TEXT("width"), ThumbW);
	ResultObj->SetNumberField(TEXT("height"), ThumbH);
	ResultObj->SetStringField(TEXT("preview_base64"), Base64);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("get_material_preview requires editor build"));
#endif
}

//=============================================================================
// Command Dispatcher
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCommand(const FString& CommandType, const TSharedPtr<FJsonObject>& Params)
{
	if (CommandType == TEXT("create_material"))
	{
		return HandleCreateMaterial(Params);
	}
	else if (CommandType == TEXT("set_material_properties"))
	{
		return HandleSetMaterialProperties(Params);
	}
	else if (CommandType == TEXT("add_material_node"))
	{
		return HandleAddMaterialNode(Params);
	}
	else if (CommandType == TEXT("set_material_node_property"))
	{
		return HandleSetMaterialNodeProperty(Params);
	}
	else if (CommandType == TEXT("connect_material_nodes"))
	{
		return HandleConnectMaterialNodes(Params);
	}
	else if (CommandType == TEXT("connect_to_material_output"))
	{
		return HandleConnectToMaterialOutput(Params);
	}
	else if (CommandType == TEXT("delete_material_node"))
	{
		return HandleDeleteMaterialNode(Params);
	}
	else if (CommandType == TEXT("recompile_material"))
	{
		return HandleRecompileMaterial(Params);
	}
	else if (CommandType == TEXT("create_material_instance"))
	{
		return HandleCreateMaterialInstance(Params);
	}
	else if (CommandType == TEXT("set_material_instance_parameter"))
	{
		return HandleSetMaterialInstanceParameter(Params);
	}
	else if (CommandType == TEXT("get_material_instance_parameters"))
	{
		return HandleGetMaterialInstanceParameters(Params);
	}
	else if (CommandType == TEXT("get_material_nodes"))
	{
		return HandleGetMaterialNodes(Params);
	}
	else if (CommandType == TEXT("layout_material_nodes"))
	{
		return HandleLayoutMaterialNodes(Params);
	}
	else if (CommandType == TEXT("set_material_node_position"))
	{
		return HandleSetMaterialNodePosition(Params);
	}
	else if (CommandType == TEXT("link_named_reroute_usage"))
	{
		return HandleLinkNamedRerouteUsage(Params);
	}
	else if (CommandType == TEXT("get_material_hierarchy"))
	{
		return HandleGetMaterialHierarchy(Params);
	}
	// Material Function Commands
	else if (CommandType == TEXT("create_material_function"))
	{
		return HandleCreateMaterialFunction(Params);
	}
	else if (CommandType == TEXT("add_function_input"))
	{
		return HandleAddFunctionInput(Params);
	}
	else if (CommandType == TEXT("add_function_output"))
	{
		return HandleAddFunctionOutput(Params);
	}
	else if (CommandType == TEXT("add_function_node"))
	{
		return HandleAddFunctionNode(Params);
	}
	else if (CommandType == TEXT("connect_function_nodes"))
	{
		return HandleConnectFunctionNodes(Params);
	}
	else if (CommandType == TEXT("add_material_function_call"))
	{
		return HandleAddMaterialFunctionCall(Params);
	}
	else if (CommandType == TEXT("get_material_function_info"))
	{
		return HandleGetMaterialFunctionInfo(Params);
	}
	else if (CommandType == TEXT("execute_material_batch"))
	{
		return HandleExecuteMaterialBatch(Params);
	}
	else if (CommandType == TEXT("execute_function_batch"))
	{
		return HandleExecuteFunctionBatch(Params);
	}
	else if (CommandType == TEXT("build_material"))
	{
		return HandleBuildMaterial(Params);
	}
	else if (CommandType == TEXT("get_material_preview"))
	{
		return HandleGetMaterialPreview(Params);
	}

	return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown material command: %s"), *CommandType));
}

//=============================================================================
// Command Handlers
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCreateMaterial(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString Path = TEXT("/Game/Materials");
	Params->TryGetStringField(TEXT("path"), Path);

	// Create the material package
	FString PackagePath = Path / MaterialName;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}

	// Create the material using factory
	UMaterialFactoryNew* MaterialFactory = NewObject<UMaterialFactoryNew>();
	UMaterial* NewMaterial = Cast<UMaterial>(MaterialFactory->FactoryCreateNew(
		UMaterial::StaticClass(),
		Package,
		*MaterialName,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!NewMaterial)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material"));
	}

	// Set optional properties
	FString DomainStr;
	if (Params->TryGetStringField(TEXT("domain"), DomainStr))
	{
		NewMaterial->MaterialDomain = StringToMaterialDomain(DomainStr);
	}

	FString BlendModeStr;
	if (Params->TryGetStringField(TEXT("blend_mode"), BlendModeStr))
	{
		NewMaterial->BlendMode = StringToBlendMode(BlendModeStr);
	}

	FString ShadingModelStr;
	if (Params->TryGetStringField(TEXT("shading_model"), ShadingModelStr))
	{
		NewMaterial->SetShadingModel(StringToShadingModel(ShadingModelStr));
	}

	bool bTwoSided = false;
	if (Params->TryGetBoolField(TEXT("two_sided"), bTwoSided))
	{
		NewMaterial->TwoSided = bTwoSided;
	}

	// Apply all extended properties from the properties dict or top-level params
	ApplyExtendedProperties(NewMaterial, Params);

	// Mark dirty and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewMaterial);

	// Save the package
	FString FilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewMaterial, *FilePath, SaveArgs);

	// Return success
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("material_name"), MaterialName);
	ResultObj->SetStringField(TEXT("material_path"), PackagePath);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material creation requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialProperties(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString MaterialPath;
	UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialName));
	}

	Material->Modify();

	FString DomainStr;
	if (Params->TryGetStringField(TEXT("domain"), DomainStr))
	{
		Material->MaterialDomain = StringToMaterialDomain(DomainStr);
	}

	FString BlendModeStr;
	if (Params->TryGetStringField(TEXT("blend_mode"), BlendModeStr))
	{
		Material->BlendMode = StringToBlendMode(BlendModeStr);
	}

	FString ShadingModelStr;
	if (Params->TryGetStringField(TEXT("shading_model"), ShadingModelStr))
	{
		Material->SetShadingModel(StringToShadingModel(ShadingModelStr));
	}

	bool bTwoSided;
	if (Params->TryGetBoolField(TEXT("two_sided"), bTwoSided))
	{
		Material->TwoSided = bTwoSided;
	}

	bool bWireframe;
	if (Params->TryGetBoolField(TEXT("wireframe"), bWireframe))
	{
		Material->Wireframe = bWireframe;
	}

	// Apply all extended properties
	ApplyExtendedProperties(Material, Params);

	// Recompile and save
	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAddMaterialNode(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString NodeType;
	if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: node_type"));
	}

	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	// Auto-detect: try as UMaterial first, then as UMaterialFunction
	UMaterialFunction* Function = nullptr;
	UMaterial* Material = nullptr;
	{
		FString TryPath;
		Material = LoadMaterial(MaterialName, TryPath);
		if (!Material)
		{
			Function = LoadMaterialFunction(MaterialName, TryPath);
			if (!Function)
			{
				return FUnrealMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Material or MaterialFunction not found: %s"), *MaterialName));
			}
		}
	}

	UClass* ExpressionClass = GetExpressionClassFromType(NodeType);
	if (!ExpressionClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown node type: %s"), *NodeType));
	}

	// Get position
	int32 PosX = 0, PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
	{
		PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
	}

	// Handle texture sample specially to set texture
	UObject* SelectedAsset = nullptr;
	FString TexturePath;
	if (Params->TryGetStringField(TEXT("texture_path"), TexturePath))
	{
		SelectedAsset = UEditorAssetLibrary::LoadAsset(TexturePath);
	}

	// Create expression — routes to material or function depending on which is set
	UMaterialExpression* NewExpression = UMaterialEditingLibrary::CreateMaterialExpressionEx(
		Material,   // nullptr when targeting a function
		Function,   // nullptr when targeting a material
		ExpressionClass,
		SelectedAsset,
		PosX,
		PosY,
		true // bAllowMarkingPackageDirty
	);

	if (!NewExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create expression"));
	}

	// Set node name/description for identification
	FString NodeName;
	if (Params->TryGetStringField(TEXT("node_name"), NodeName))
	{
		NewExpression->Desc = NodeName;

		// Set parameter name if applicable
		if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->ParameterName = FName(*NodeName);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VectorParam->ParameterName = FName(*NodeName);
		}
		else if (UMaterialExpressionTextureSampleParameter* TextureParam = Cast<UMaterialExpressionTextureSampleParameter>(NewExpression))
		{
			TextureParam->ParameterName = FName(*NodeName);
		}
	}

	// Set parameter group if provided
	FString GroupName;
	if (Params->TryGetStringField(TEXT("group"), GroupName))
	{
		if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->Group = FName(*GroupName);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VectorParam->Group = FName(*GroupName);
		}
		else if (UMaterialExpressionTextureSampleParameter* TextureParam = Cast<UMaterialExpressionTextureSampleParameter>(NewExpression))
		{
			TextureParam->Group = FName(*GroupName);
		}
	}

	// Set value if provided
	double Value = 0.0;
	if (Params->TryGetNumberField(TEXT("value"), Value))
	{
		if (UMaterialExpressionConstant* ConstExpr = Cast<UMaterialExpressionConstant>(NewExpression))
		{
			ConstExpr->R = static_cast<float>(Value);
		}
		else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->DefaultValue = static_cast<float>(Value);
		}
	}

	// Set vector value if provided
	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
	{
		float R = static_cast<float>((*ColorArray)[0]->AsNumber());
		float G = static_cast<float>((*ColorArray)[1]->AsNumber());
		float B = static_cast<float>((*ColorArray)[2]->AsNumber());
		float A = ColorArray->Num() >= 4 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;

		if (UMaterialExpressionConstant3Vector* Vec3 = Cast<UMaterialExpressionConstant3Vector>(NewExpression))
		{
			Vec3->Constant = FLinearColor(R, G, B);
		}
		else if (UMaterialExpressionConstant4Vector* Vec4 = Cast<UMaterialExpressionConstant4Vector>(NewExpression))
		{
			Vec4->Constant = FLinearColor(R, G, B, A);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VectorParam->DefaultValue = FLinearColor(R, G, B, A);
		}
	}

	// Handle Custom HLSL expression
	if (UMaterialExpressionCustom* CustomExpr = Cast<UMaterialExpressionCustom>(NewExpression))
	{
		// Set HLSL code
		FString Code;
		if (Params->TryGetStringField(TEXT("code"), Code))
		{
			CustomExpr->Code = Code;
		}

		// Set output type
		FString OutputTypeStr;
		if (Params->TryGetStringField(TEXT("output_type"), OutputTypeStr))
		{
			if (OutputTypeStr.Equals(TEXT("float"), ESearchCase::IgnoreCase))
			{
				CustomExpr->OutputType = CMOT_Float1;
			}
			else if (OutputTypeStr.Equals(TEXT("float2"), ESearchCase::IgnoreCase))
			{
				CustomExpr->OutputType = CMOT_Float2;
			}
			else if (OutputTypeStr.Equals(TEXT("float3"), ESearchCase::IgnoreCase))
			{
				CustomExpr->OutputType = CMOT_Float3;
			}
			else if (OutputTypeStr.Equals(TEXT("float4"), ESearchCase::IgnoreCase))
			{
				CustomExpr->OutputType = CMOT_Float4;
			}
		}

		// Set inputs
		const TArray<TSharedPtr<FJsonValue>>* InputsArray;
		if (Params->TryGetArrayField(TEXT("inputs"), InputsArray))
		{
			CustomExpr->Inputs.Empty();
			for (const TSharedPtr<FJsonValue>& InputVal : *InputsArray)
			{
				FString InputName = InputVal->AsString();
				FCustomInput NewInput;
				NewInput.InputName = FName(*InputName);
				CustomExpr->Inputs.Add(NewInput);
			}
		}

		// Set description as function name if provided
		FString FunctionName;
		if (Params->TryGetStringField(TEXT("function_name"), FunctionName))
		{
			CustomExpr->Description = FunctionName;
		}
	}

	// Handle Named Reroute Declaration - set the Name property
	if (UMaterialExpressionNamedRerouteDeclaration* NamedReroute = Cast<UMaterialExpressionNamedRerouteDeclaration>(NewExpression))
	{
		if (!NodeName.IsEmpty())
		{
			NamedReroute->Name = FName(*NodeName);
		}

		// Set optional node color
		const TArray<TSharedPtr<FJsonValue>>* NodeColorArray;
		if (Params->TryGetArrayField(TEXT("node_color"), NodeColorArray) && NodeColorArray->Num() >= 3)
		{
			float R = static_cast<float>((*NodeColorArray)[0]->AsNumber());
			float G = static_cast<float>((*NodeColorArray)[1]->AsNumber());
			float B = static_cast<float>((*NodeColorArray)[2]->AsNumber());
			float A = NodeColorArray->Num() >= 4 ? static_cast<float>((*NodeColorArray)[3]->AsNumber()) : 1.0f;
			NamedReroute->NodeColor = FLinearColor(R, G, B, A);
		}
	}

	if (Material) { Material->MarkPackageDirty(); }
	else if (Function) { Function->MarkPackageDirty(); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_name"), NewExpression->Desc.IsEmpty() ? NewExpression->GetName() : NewExpression->Desc);
	ResultObj->SetStringField(TEXT("node_type"), NodeType);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialNodeProperty(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString NodeName;
	if (!Params->TryGetStringField(TEXT("node_name"), NodeName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: node_name"));
	}

	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	UMaterialFunction* Function = nullptr;
	UMaterial* Material = nullptr;
	{
		FString TryPath;
		Material = LoadMaterial(MaterialName, TryPath);
		if (!Material)
		{
			Function = LoadMaterialFunction(MaterialName, TryPath);
			if (!Function)
			{
				return FUnrealMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Material or MaterialFunction not found: %s"), *MaterialName));
			}
		}
	}

	UMaterialExpression* Expression = Function
		? FindExpressionInFunction(Function, NodeName)
		: FindExpressionByName(Material, NodeName);
	if (!Expression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeName));
	}

	Expression->Modify();

	// Handle common properties
	double Value;
	if (Params->TryGetNumberField(TEXT("value"), Value))
	{
		if (UMaterialExpressionConstant* ConstExpr = Cast<UMaterialExpressionConstant>(Expression))
		{
			ConstExpr->R = static_cast<float>(Value);
		}
		else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expression))
		{
			ScalarParam->DefaultValue = static_cast<float>(Value);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
	{
		float R = static_cast<float>((*ColorArray)[0]->AsNumber());
		float G = static_cast<float>((*ColorArray)[1]->AsNumber());
		float B = static_cast<float>((*ColorArray)[2]->AsNumber());
		float A = ColorArray->Num() >= 4 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;

		if (UMaterialExpressionConstant3Vector* Vec3 = Cast<UMaterialExpressionConstant3Vector>(Expression))
		{
			Vec3->Constant = FLinearColor(R, G, B);
		}
		else if (UMaterialExpressionConstant4Vector* Vec4 = Cast<UMaterialExpressionConstant4Vector>(Expression))
		{
			Vec4->Constant = FLinearColor(R, G, B, A);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expression))
		{
			VectorParam->DefaultValue = FLinearColor(R, G, B, A);
		}
	}

	// BumpOffset-specific properties
	if (UMaterialExpressionBumpOffset* BumpExpr = Cast<UMaterialExpressionBumpOffset>(Expression))
	{
		double HeightRatio;
		if (Params->TryGetNumberField(TEXT("height_ratio"), HeightRatio))
		{
			BumpExpr->HeightRatio = static_cast<float>(HeightRatio);
		}
		double ReferencePlane;
		if (Params->TryGetNumberField(TEXT("reference_plane"), ReferencePlane))
		{
			BumpExpr->ReferencePlane = static_cast<float>(ReferencePlane);
		}
	}

	// Texture SamplerType (Color, Normal, LinearColor, Masks, Grayscale, Alpha, Data)
	FString SamplerTypeStr;
	if (Params->TryGetStringField(TEXT("sampler_type"), SamplerTypeStr))
	{
		if (UMaterialExpressionTextureBase* TexExpr = Cast<UMaterialExpressionTextureBase>(Expression))
		{
			if (SamplerTypeStr.Equals(TEXT("Color"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_Color;
			}
			else if (SamplerTypeStr.Equals(TEXT("Normal"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_Normal;
			}
			else if (SamplerTypeStr.Equals(TEXT("LinearColor"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_LinearColor;
			}
			else if (SamplerTypeStr.Equals(TEXT("Masks"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_Masks;
			}
			else if (SamplerTypeStr.Equals(TEXT("Grayscale"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_Grayscale;
			}
			else if (SamplerTypeStr.Equals(TEXT("LinearGrayscale"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_LinearGrayscale;
			}
			else if (SamplerTypeStr.Equals(TEXT("Alpha"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_Alpha;
			}
			else if (SamplerTypeStr.Equals(TEXT("Data"), ESearchCase::IgnoreCase))
			{
				TexExpr->SamplerType = SAMPLERTYPE_Data;
			}
		}
	}

	if (Material) { Material->MarkPackageDirty(); }
	else if (Function) { Function->MarkPackageDirty(); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleConnectMaterialNodes(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString FromNode, ToNode, ToInput;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNode))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: from_node"));
	}
	if (!Params->TryGetStringField(TEXT("to_node"), ToNode))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: to_node"));
	}
	if (!Params->TryGetStringField(TEXT("to_input"), ToInput))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: to_input"));
	}

	FString FromOutput = TEXT("");
	Params->TryGetStringField(TEXT("from_output"), FromOutput);

	UMaterialFunction* Function = nullptr;
	UMaterial* Material = nullptr;
	{
		FString TryPath;
		Material = LoadMaterial(MaterialName, TryPath);
		if (!Material)
		{
			Function = LoadMaterialFunction(MaterialName, TryPath);
			if (!Function)
			{
				return FUnrealMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Material or MaterialFunction not found: %s"), *MaterialName));
			}
		}
	}

	UMaterialExpression* FromExpression = Function
		? FindExpressionInFunction(Function, FromNode)
		: FindExpressionByName(Material, FromNode);
	if (!FromExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("From node not found: %s"), *FromNode));
	}

	UMaterialExpression* ToExpression = Function
		? FindExpressionInFunction(Function, ToNode)
		: FindExpressionByName(Material, ToNode);
	if (!ToExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("To node not found: %s"), *ToNode));
	}

	// Universal dynamic pin name resolution using UE reflection system
	TArray<FString> PinNamesToTry;

	// Always try user's requested pin name first
	PinNamesToTry.Add(ToInput);

	// Introspect target node to discover ALL available input pins dynamically
	// This works for any material expression node type without hardcoding
	TArray<FString> DiscoveredInputs;

	// Method 1: Find all FExpressionInput properties via reflection
	for (TFieldIterator<FProperty> PropIt(ToExpression->GetClass()); PropIt; ++PropIt)
	{
		FProperty* Prop = *PropIt;

		// Check for FExpressionInput struct properties (standard input pins)
		if (FStructProperty* StructProp = CastField<FStructProperty>(Prop))
		{
			if (StructProp->Struct && StructProp->Struct->GetName().Equals(TEXT("FExpressionInput")))
			{
				FString InputPinName = Prop->GetName();
				DiscoveredInputs.AddUnique(InputPinName);
			}
		}

		// Check for TArray<FCustomInput> (Custom node dynamic inputs)
		if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Prop))
		{
			if (FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner))
			{
				if (InnerStruct->Struct && InnerStruct->Struct->GetName().Equals(TEXT("CustomInput")))
				{
					// This is a Custom node - extract input names from the Inputs array
					if (UMaterialExpressionCustom* CustomExpr = Cast<UMaterialExpressionCustom>(ToExpression))
					{
						for (const FCustomInput& CustomInput : CustomExpr->Inputs)
						{
							FString CustomInputName = CustomInput.InputName.ToString();
							DiscoveredInputs.AddUnique(CustomInputName);
						}
					}
				}
			}
		}
	}

	// Add all discovered inputs to try list (user's input already first)
	for (const FString& DiscoveredInput : DiscoveredInputs)
	{
		if (!PinNamesToTry.Contains(DiscoveredInput))
		{
			PinNamesToTry.Add(DiscoveredInput);
		}
	}

	// Fallback: try empty string as last resort (some nodes accept unnamed input)
	if (!PinNamesToTry.Contains(TEXT("")))
	{
		PinNamesToTry.Add(TEXT(""));
	}

	bool bSuccess = false;
	FString SuccessfulPinName;

	// Try each pin name variation until one succeeds
	for (const FString& PinName : PinNamesToTry)
	{
		bSuccess = UMaterialEditingLibrary::ConnectMaterialExpressions(
			FromExpression,
			FromOutput,
			ToExpression,
			PinName
		);

		if (bSuccess)
		{
			SuccessfulPinName = PinName;
			break;
		}
	}

	if (!bSuccess)
	{
		// Build helpful error message showing what was tried
		FString TriedPins = FString::Join(PinNamesToTry, TEXT(", "));
		return FUnrealMCPCommonUtils::CreateErrorResponse(
			FString::Printf(TEXT("Failed to connect nodes. Tried input pins: %s"), *TriedPins)
		);
	}

	if (Material) { Material->MarkPackageDirty(); }
	else if (Function) { Function->MarkPackageDirty(); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleConnectToMaterialOutput(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString FromNode, MaterialProperty;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNode))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: from_node"));
	}
	if (!Params->TryGetStringField(TEXT("material_property"), MaterialProperty))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_property"));
	}

	FString FromOutput = TEXT("");
	Params->TryGetStringField(TEXT("from_output"), FromOutput);

	FString MaterialPath;
	UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialName));
	}

	UMaterialExpression* FromExpression = FindExpressionByName(Material, FromNode);
	if (!FromExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("From node not found: %s"), *FromNode));
	}

	EMaterialProperty Property = StringToMaterialProperty(MaterialProperty);

	bool bSuccess = UMaterialEditingLibrary::ConnectMaterialProperty(
		FromExpression,
		FromOutput,
		Property
	);

	if (!bSuccess)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to connect to material property: %s"), *MaterialProperty));
	}

	Material->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleDeleteMaterialNode(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString NodeName;
	if (!Params->TryGetStringField(TEXT("node_name"), NodeName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: node_name"));
	}

	UMaterialFunction* Function = nullptr;
	UMaterial* Material = nullptr;
	{
		FString TryPath;
		Material = LoadMaterial(MaterialName, TryPath);
		if (!Material)
		{
			Function = LoadMaterialFunction(MaterialName, TryPath);
			if (!Function)
			{
				return FUnrealMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Material or MaterialFunction not found: %s"), *MaterialName));
			}
		}
	}

	UMaterialExpression* Expression = Function
		? FindExpressionInFunction(Function, NodeName)
		: FindExpressionByName(Material, NodeName);
	if (!Expression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeName));
	}

	// Safety check: Cannot delete rooted objects
	if (Expression->IsRooted())
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Cannot delete rooted node: %s (delete manually in editor)"), *NodeName));
	}

	// Safety check: Cannot delete objects pending kill
	if (!IsValid(Expression))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node is invalid or pending kill: %s"), *NodeName));
	}

	if (Function)
	{
		UMaterialEditingLibrary::DeleteMaterialExpressionInFunction(Function, Expression);
		Function->MarkPackageDirty();
	}
	else
	{
		UMaterialEditingLibrary::DeleteMaterialExpression(Material, Expression);
		Material->MarkPackageDirty();
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleRecompileMaterial(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString MaterialPath;
	UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialName));
	}

	UMaterialEditingLibrary::RecompileMaterial(Material);
	Material->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCreateMaterialInstance(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString InstanceName;
	if (!Params->TryGetStringField(TEXT("instance_name"), InstanceName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: instance_name"));
	}

	FString ParentMaterialName;
	if (!Params->TryGetStringField(TEXT("parent_material"), ParentMaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: parent_material"));
	}

	FString Path = TEXT("/Game/Materials");
	Params->TryGetStringField(TEXT("path"), Path);

	// Load parent material
	FString ParentPath;
	UMaterial* ParentMaterial = LoadMaterial(ParentMaterialName, ParentPath);
	if (!ParentMaterial)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Parent material not found: %s"), *ParentMaterialName));
	}

	// Create package
	FString PackagePath = Path / InstanceName;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}

	// Create material instance using factory
	UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
	Factory->InitialParent = ParentMaterial;

	UMaterialInstanceConstant* NewInstance = Cast<UMaterialInstanceConstant>(Factory->FactoryCreateNew(
		UMaterialInstanceConstant::StaticClass(),
		Package,
		*InstanceName,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!NewInstance)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material instance"));
	}

	// Mark dirty and save
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewInstance);

	FString FilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewInstance, *FilePath, SaveArgs);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("instance_name"), InstanceName);
	ResultObj->SetStringField(TEXT("instance_path"), PackagePath);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material instance creation requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialInstanceParameter(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString InstanceName;
	if (!Params->TryGetStringField(TEXT("instance_name"), InstanceName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: instance_name"));
	}

	FString ParameterName;
	if (!Params->TryGetStringField(TEXT("parameter_name"), ParameterName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: parameter_name"));
	}

	FString ParameterType;
	if (!Params->TryGetStringField(TEXT("parameter_type"), ParameterType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: parameter_type"));
	}

	FString InstancePath;
	UMaterialInstanceConstant* Instance = LoadMaterialInstance(InstanceName, InstancePath);
	if (!Instance)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found: %s"), *InstanceName));
	}

	if (ParameterType.Equals(TEXT("Scalar"), ESearchCase::IgnoreCase))
	{
		double Value;
		if (!Params->TryGetNumberField(TEXT("value"), Value))
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: value"));
		}
		UMaterialEditingLibrary::SetMaterialInstanceScalarParameterValue(Instance, FName(*ParameterName), static_cast<float>(Value));
	}
	else if (ParameterType.Equals(TEXT("Vector"), ESearchCase::IgnoreCase))
	{
		const TArray<TSharedPtr<FJsonValue>>* ColorArray;
		if (!Params->TryGetArrayField(TEXT("value"), ColorArray) || ColorArray->Num() < 3)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing or invalid value array for vector parameter"));
		}
		float R = static_cast<float>((*ColorArray)[0]->AsNumber());
		float G = static_cast<float>((*ColorArray)[1]->AsNumber());
		float B = static_cast<float>((*ColorArray)[2]->AsNumber());
		float A = ColorArray->Num() >= 4 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;
		UMaterialEditingLibrary::SetMaterialInstanceVectorParameterValue(Instance, FName(*ParameterName), FLinearColor(R, G, B, A));
	}
	else if (ParameterType.Equals(TEXT("Texture"), ESearchCase::IgnoreCase))
	{
		FString TexturePath;
		if (!Params->TryGetStringField(TEXT("value"), TexturePath))
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: value (texture path)"));
		}
		UTexture* Texture = Cast<UTexture>(UEditorAssetLibrary::LoadAsset(TexturePath));
		if (!Texture)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Texture not found: %s"), *TexturePath));
		}
		UMaterialEditingLibrary::SetMaterialInstanceTextureParameterValue(Instance, FName(*ParameterName), Texture);
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown parameter type: %s (use Scalar, Vector, or Texture)"), *ParameterType));
	}

	UMaterialEditingLibrary::UpdateMaterialInstance(Instance);
	Instance->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material instance editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleGetMaterialInstanceParameters(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString InstanceName;
	if (!Params->TryGetStringField(TEXT("instance_name"), InstanceName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: instance_name"));
	}

	FString InstancePath;
	UMaterialInstanceConstant* Instance = LoadMaterialInstance(InstanceName, InstancePath);
	if (!Instance)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material instance not found: %s"), *InstanceName));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("instance_path"), InstancePath);

	// Parent material
	if (Instance->Parent)
	{
		ResultObj->SetStringField(TEXT("parent"), Instance->Parent->GetPathName());
	}

	// Scalar parameters
	TArray<TSharedPtr<FJsonValue>> ScalarArray;
	for (const FScalarParameterValue& Param : Instance->ScalarParameterValues)
	{
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), Param.ParameterInfo.Name.ToString());
		Entry->SetNumberField(TEXT("value"), static_cast<double>(Param.ParameterValue));
		ScalarArray.Add(MakeShared<FJsonValueObject>(Entry));
	}
	ResultObj->SetArrayField(TEXT("scalar_parameters"), ScalarArray);

	// Vector parameters
	TArray<TSharedPtr<FJsonValue>> VectorArray;
	for (const FVectorParameterValue& Param : Instance->VectorParameterValues)
	{
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), Param.ParameterInfo.Name.ToString());
		TArray<TSharedPtr<FJsonValue>> ColorArr;
		ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.R));
		ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.G));
		ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.B));
		ColorArr.Add(MakeShared<FJsonValueNumber>(Param.ParameterValue.A));
		Entry->SetArrayField(TEXT("value"), ColorArr);
		VectorArray.Add(MakeShared<FJsonValueObject>(Entry));
	}
	ResultObj->SetArrayField(TEXT("vector_parameters"), VectorArray);

	// Texture parameters
	TArray<TSharedPtr<FJsonValue>> TextureArray;
	for (const FTextureParameterValue& Param : Instance->TextureParameterValues)
	{
		TSharedPtr<FJsonObject> Entry = MakeShared<FJsonObject>();
		Entry->SetStringField(TEXT("name"), Param.ParameterInfo.Name.ToString());
		Entry->SetStringField(TEXT("value"), Param.ParameterValue ? Param.ParameterValue->GetPathName() : TEXT("None"));
		TextureArray.Add(MakeShared<FJsonValueObject>(Entry));
	}
	ResultObj->SetArrayField(TEXT("texture_parameters"), TextureArray);

	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material instance reading requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleGetMaterialNodes(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	UMaterialFunction* Function = nullptr;
	UMaterial* Material = nullptr;
	{
		FString TryPath;
		Material = LoadMaterial(MaterialName, TryPath);
		if (!Material)
		{
			Function = LoadMaterialFunction(MaterialName, TryPath);
			if (!Function)
			{
				return FUnrealMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Material or MaterialFunction not found: %s"), *MaterialName));
			}
		}
	}

	// Normalize expressions — UMaterial returns TArray<UMaterialExpression*>&,
	// UMaterialFunction returns TArrayView<const TObjectPtr<...>> — copy into a flat array
	TArray<UMaterialExpression*> Expressions;
	if (Function)
	{
		for (const TObjectPtr<UMaterialExpression>& Ptr : Function->GetExpressions())
		{
			if (Ptr) { Expressions.Add(Ptr.Get()); }
		}
	}
	else
	{
		Expressions = Material->GetExpressions();
	}

	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UMaterialExpression* Expr : Expressions)
	{
		if (!Expr)
		{
			continue;
		}

		TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
		NodeObj->SetStringField(TEXT("name"), Expr->Desc.IsEmpty() ? Expr->GetName() : Expr->Desc);
		NodeObj->SetStringField(TEXT("type"), Expr->GetClass()->GetName());
		NodeObj->SetNumberField(TEXT("x"), Expr->MaterialExpressionEditorX);
		NodeObj->SetNumberField(TEXT("y"), Expr->MaterialExpressionEditorY);

		// Add parameter-specific info
		if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(Expr))
		{
			NodeObj->SetStringField(TEXT("parameter_name"), ScalarParam->ParameterName.ToString());
			NodeObj->SetNumberField(TEXT("default_value"), ScalarParam->DefaultValue);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(Expr))
		{
			NodeObj->SetStringField(TEXT("parameter_name"), VectorParam->ParameterName.ToString());
			TArray<TSharedPtr<FJsonValue>> ColorArr;
			ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.R));
			ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.G));
			ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.B));
			ColorArr.Add(MakeShared<FJsonValueNumber>(VectorParam->DefaultValue.A));
			NodeObj->SetArrayField(TEXT("default_value"), ColorArr);
		}
		// Add Named Reroute specific info
		else if (UMaterialExpressionNamedRerouteDeclaration* NamedReroute = Cast<UMaterialExpressionNamedRerouteDeclaration>(Expr))
		{
			NodeObj->SetStringField(TEXT("reroute_name"), NamedReroute->Name.ToString());
			NodeObj->SetStringField(TEXT("reroute_guid"), NamedReroute->VariableGuid.ToString());
			TArray<TSharedPtr<FJsonValue>> ColorArr;
			ColorArr.Add(MakeShared<FJsonValueNumber>(NamedReroute->NodeColor.R));
			ColorArr.Add(MakeShared<FJsonValueNumber>(NamedReroute->NodeColor.G));
			ColorArr.Add(MakeShared<FJsonValueNumber>(NamedReroute->NodeColor.B));
			ColorArr.Add(MakeShared<FJsonValueNumber>(NamedReroute->NodeColor.A));
			NodeObj->SetArrayField(TEXT("node_color"), ColorArr);
		}
		else if (UMaterialExpressionNamedRerouteUsage* RerouteUsage = Cast<UMaterialExpressionNamedRerouteUsage>(Expr))
		{
			NodeObj->SetStringField(TEXT("declaration_guid"), RerouteUsage->DeclarationGuid.ToString());
			if (RerouteUsage->Declaration)
			{
				NodeObj->SetStringField(TEXT("declaration_name"), RerouteUsage->Declaration->Name.ToString());
			}
		}

		NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
	ResultObj->SetNumberField(TEXT("node_count"), NodesArray.Num());
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material inspection requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleLayoutMaterialNodes(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString MaterialPath;
	UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialName));
	}

	UMaterialEditingLibrary::LayoutMaterialExpressions(Material);
	Material->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material layout requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleSetMaterialNodePosition(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	UMaterialFunction* Function = nullptr;
	UMaterial* Material = nullptr;
	{
		FString TryPath;
		Material = LoadMaterial(MaterialName, TryPath);
		if (!Material)
		{
			Function = LoadMaterialFunction(MaterialName, TryPath);
			if (!Function)
			{
				return FUnrealMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Material or MaterialFunction not found: %s"), *MaterialName));
			}
		}
	}

	// Support batch mode: "nodes" array of {name, x, y}
	const TArray<TSharedPtr<FJsonValue>>* NodesArray = nullptr;
	if (Params->TryGetArrayField(TEXT("nodes"), NodesArray) && NodesArray)
	{
		int32 Moved = 0;
		TArray<FString> Errors;
		for (const TSharedPtr<FJsonValue>& NodeVal : *NodesArray)
		{
			const TSharedPtr<FJsonObject>* NodeObj = nullptr;
			if (!NodeVal->TryGetObject(NodeObj) || !NodeObj)
			{
				continue;
			}
			FString NodeName;
			if (!(*NodeObj)->TryGetStringField(TEXT("name"), NodeName))
			{
				continue;
			}
			UMaterialExpression* Expr = Function
				? FindExpressionInFunction(Function, NodeName)
				: FindExpressionByName(Material, NodeName);
			if (!Expr)
			{
				Errors.Add(FString::Printf(TEXT("Node not found: %s"), *NodeName));
				continue;
			}
			int32 X = 0, Y = 0;
			(*NodeObj)->TryGetNumberField(TEXT("x"), X);
			(*NodeObj)->TryGetNumberField(TEXT("y"), Y);
			Expr->MaterialExpressionEditorX = X;
			Expr->MaterialExpressionEditorY = Y;
			Moved++;
		}
		if (Material) { Material->MarkPackageDirty(); }
		else if (Function) { Function->MarkPackageDirty(); }

		TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
		ResultObj->SetBoolField(TEXT("success"), true);
		ResultObj->SetNumberField(TEXT("moved_count"), Moved);
		if (Errors.Num() > 0)
		{
			TArray<TSharedPtr<FJsonValue>> ErrorArray;
			for (const FString& Err : Errors)
			{
				ErrorArray.Add(MakeShared<FJsonValueString>(Err));
			}
			ResultObj->SetArrayField(TEXT("errors"), ErrorArray);
		}
		return ResultObj;
	}

	// Single node mode
	FString NodeName;
	if (!Params->TryGetStringField(TEXT("node_name"), NodeName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: node_name or nodes array"));
	}

	UMaterialExpression* Expression = Function
		? FindExpressionInFunction(Function, NodeName)
		: FindExpressionByName(Material, NodeName);
	if (!Expression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node not found: %s"), *NodeName));
	}

	int32 X = 0, Y = 0;
	Params->TryGetNumberField(TEXT("x"), X);
	Params->TryGetNumberField(TEXT("y"), Y);
	Expression->MaterialExpressionEditorX = X;
	Expression->MaterialExpressionEditorY = Y;
	if (Material) { Material->MarkPackageDirty(); }
	else if (Function) { Function->MarkPackageDirty(); }

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_name"), NodeName);
	ResultObj->SetNumberField(TEXT("x"), X);
	ResultObj->SetNumberField(TEXT("y"), Y);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleLinkNamedRerouteUsage(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString UsageNodeName;
	if (!Params->TryGetStringField(TEXT("usage_node"), UsageNodeName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: usage_node"));
	}

	FString DeclarationNodeName;
	if (!Params->TryGetStringField(TEXT("declaration_node"), DeclarationNodeName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: declaration_node"));
	}

	// Support both materials and material functions
	UMaterialExpression* UsageExpr = nullptr;
	UMaterialExpression* DeclExpr = nullptr;
	UObject* OwnerPackage = nullptr;

	FString FunctionName;
	FString MaterialName;
	if (Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		FString FunctionPath;
		UMaterialFunction* Function = LoadMaterialFunction(FunctionName, FunctionPath);
		if (!Function)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material function not found: %s"), *FunctionName));
		}
		UsageExpr = FindExpressionInFunction(Function, UsageNodeName);
		DeclExpr = FindExpressionInFunction(Function, DeclarationNodeName);
		OwnerPackage = Function->GetPackage();
	}
	else if (Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		FString MaterialPath;
		UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
		if (!Material)
		{
			return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialName));
		}
		UsageExpr = FindExpressionByName(Material, UsageNodeName);
		DeclExpr = FindExpressionByName(Material, DeclarationNodeName);
		OwnerPackage = Material->GetPackage();
	}
	else
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing 'material_name' or 'function_name' parameter"));
	}

	if (!UsageExpr)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Usage node not found: %s"), *UsageNodeName));
	}

	UMaterialExpressionNamedRerouteUsage* Usage = Cast<UMaterialExpressionNamedRerouteUsage>(UsageExpr);
	if (!Usage)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node is not a NamedRerouteUsage: %s"), *UsageNodeName));
	}

	if (!DeclExpr)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Declaration node not found: %s"), *DeclarationNodeName));
	}

	UMaterialExpressionNamedRerouteDeclaration* Declaration = Cast<UMaterialExpressionNamedRerouteDeclaration>(DeclExpr);
	if (!Declaration)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Node is not a NamedRerouteDeclaration: %s"), *DeclarationNodeName));
	}

	// Link the usage to the declaration
	Usage->Declaration = Declaration;
	Usage->DeclarationGuid = Declaration->VariableGuid;

	if (OwnerPackage)
	{
		OwnerPackage->MarkPackageDirty();
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("usage_node"), UsageNodeName);
	ResultObj->SetStringField(TEXT("declaration_node"), DeclarationNodeName);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleGetMaterialHierarchy(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	UMaterialFunction* Function = nullptr;
	UMaterial* Material = nullptr;
	{
		FString TryPath;
		Material = LoadMaterial(MaterialName, TryPath);
		if (!Material)
		{
			Function = LoadMaterialFunction(MaterialName, TryPath);
			if (!Function)
			{
				return FUnrealMCPCommonUtils::CreateErrorResponse(
					FString::Printf(TEXT("Material or MaterialFunction not found: %s"), *MaterialName));
			}
		}
	}

	// Normalize expressions — UMaterial returns TArray<UMaterialExpression*>&,
	// UMaterialFunction returns TArrayView<const TObjectPtr<...>> — copy into a flat array
	TArray<UMaterialExpression*> Expressions;
	if (Function)
	{
		for (const TObjectPtr<UMaterialExpression>& Ptr : Function->GetExpressions())
		{
			if (Ptr) { Expressions.Add(Ptr.Get()); }
		}
	}
	else
	{
		Expressions = Material->GetExpressions();
	}

	// Map expressions to their names for reference
	TMap<UMaterialExpression*, FString> ExprToName;
	for (UMaterialExpression* Expr : Expressions)
	{
		if (Expr)
		{
			ExprToName.Add(Expr, Expr->Desc.IsEmpty() ? Expr->GetName() : Expr->Desc);
		}
	}

	// 1. Build nodes array with inputs/outputs info
	TArray<TSharedPtr<FJsonValue>> NodesArray;
	for (UMaterialExpression* Expr : Expressions)
	{
		if (!Expr)
		{
			continue;
		}

		TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
		NodeObj->SetStringField(TEXT("name"), ExprToName[Expr]);
		NodeObj->SetStringField(TEXT("type"), Expr->GetClass()->GetName());
		NodeObj->SetNumberField(TEXT("x"), Expr->MaterialExpressionEditorX);
		NodeObj->SetNumberField(TEXT("y"), Expr->MaterialExpressionEditorY);

		// List inputs with connection status
		TArray<TSharedPtr<FJsonValue>> InputsArray;
		for (int32 i = 0; i < 16; ++i)  // Max reasonable inputs
		{
			FExpressionInput* Input = Expr->GetInput(i);
			if (!Input)
			{
				break;
			}

			TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
			InputObj->SetNumberField(TEXT("index"), i);
			InputObj->SetStringField(TEXT("name"), Expr->GetInputName(i).ToString());
			InputObj->SetBoolField(TEXT("connected"), Input->Expression != nullptr);
			if (Input->Expression)
			{
				InputObj->SetStringField(TEXT("connected_to"), ExprToName.FindRef(Input->Expression));
				InputObj->SetNumberField(TEXT("connected_output"), Input->OutputIndex);
			}
			InputsArray.Add(MakeShared<FJsonValueObject>(InputObj));
		}
		NodeObj->SetArrayField(TEXT("inputs"), InputsArray);

		// List outputs
		TArray<TSharedPtr<FJsonValue>> OutputsArray;
		for (int32 i = 0; i < Expr->Outputs.Num(); ++i)
		{
			TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
			OutputObj->SetNumberField(TEXT("index"), i);
			OutputObj->SetStringField(TEXT("name"), Expr->Outputs[i].OutputName.ToString());
			OutputsArray.Add(MakeShared<FJsonValueObject>(OutputObj));
		}
		NodeObj->SetArrayField(TEXT("outputs"), OutputsArray);

		NodesArray.Add(MakeShared<FJsonValueObject>(NodeObj));
	}

	// 2. Build connections array
	TArray<TSharedPtr<FJsonValue>> ConnectionsArray;
	for (UMaterialExpression* Expr : Expressions)
	{
		if (!Expr)
		{
			continue;
		}

		for (int32 i = 0; i < 16; ++i)
		{
			FExpressionInput* Input = Expr->GetInput(i);
			if (!Input)
			{
				break;
			}
			if (!Input->Expression)
			{
				continue;
			}

			TSharedPtr<FJsonObject> ConnObj = MakeShared<FJsonObject>();
			ConnObj->SetStringField(TEXT("from_node"), ExprToName.FindRef(Input->Expression));
			ConnObj->SetNumberField(TEXT("from_output"), Input->OutputIndex);
			ConnObj->SetStringField(TEXT("to_node"), ExprToName[Expr]);
			ConnObj->SetNumberField(TEXT("to_input"), i);
			ConnObj->SetStringField(TEXT("to_input_name"), Expr->GetInputName(i).ToString());
			ConnectionsArray.Add(MakeShared<FJsonValueObject>(ConnObj));
		}
	}

	// 3. Build material outputs array (only for UMaterial, not functions)
	TArray<TSharedPtr<FJsonValue>> MaterialOutputsArray;

	if (Material)
	{
		auto AddMaterialOutput = [&](EMaterialProperty Property, const TCHAR* PropertyName)
		{
			FExpressionInput* Input = Material->GetExpressionInputForProperty(Property);
			TSharedPtr<FJsonObject> OutObj = MakeShared<FJsonObject>();
			OutObj->SetStringField(TEXT("property"), PropertyName);
			if (Input && Input->Expression)
			{
				OutObj->SetBoolField(TEXT("connected"), true);
				OutObj->SetStringField(TEXT("connected_node"), ExprToName.FindRef(Input->Expression));
				OutObj->SetNumberField(TEXT("connected_output"), Input->OutputIndex);
			}
			else
			{
				OutObj->SetBoolField(TEXT("connected"), false);
			}
			MaterialOutputsArray.Add(MakeShared<FJsonValueObject>(OutObj));
		};

		AddMaterialOutput(MP_EmissiveColor, TEXT("EmissiveColor"));
		AddMaterialOutput(MP_Opacity, TEXT("Opacity"));
		AddMaterialOutput(MP_OpacityMask, TEXT("OpacityMask"));
		AddMaterialOutput(MP_BaseColor, TEXT("BaseColor"));
		AddMaterialOutput(MP_Normal, TEXT("Normal"));
		AddMaterialOutput(MP_Metallic, TEXT("Metallic"));
		AddMaterialOutput(MP_Roughness, TEXT("Roughness"));
		AddMaterialOutput(MP_Specular, TEXT("Specular"));
		AddMaterialOutput(MP_AmbientOcclusion, TEXT("AmbientOcclusion"));
		AddMaterialOutput(MP_WorldPositionOffset, TEXT("WorldPositionOffset"));
	}

	// Return complete hierarchy
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetArrayField(TEXT("nodes"), NodesArray);
	ResultObj->SetArrayField(TEXT("connections"), ConnectionsArray);
	ResultObj->SetArrayField(TEXT("material_outputs"), MaterialOutputsArray);
	ResultObj->SetNumberField(TEXT("node_count"), NodesArray.Num());
	ResultObj->SetNumberField(TEXT("connection_count"), ConnectionsArray.Num());
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material hierarchy inspection requires editor"));
#endif
}

//=============================================================================
// Material Function Handlers
//=============================================================================

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleCreateMaterialFunction(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: function_name"));
	}

	FString Path = TEXT("/Game/Materials/Functions");
	Params->TryGetStringField(TEXT("path"), Path);

	FString PackagePath = Path / FunctionName;
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Failed to create package: %s"), *PackagePath));
	}

	UMaterialFunctionFactoryNew* Factory = NewObject<UMaterialFunctionFactoryNew>();
	UMaterialFunction* NewFunction = Cast<UMaterialFunction>(Factory->FactoryCreateNew(
		UMaterialFunction::StaticClass(),
		Package,
		*FunctionName,
		RF_Public | RF_Standalone,
		nullptr,
		GWarn
	));

	if (!NewFunction)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create material function"));
	}

	FString Description;
	if (Params->TryGetStringField(TEXT("description"), Description))
	{
		NewFunction->Description = Description;
	}

	bool bExposeToLibrary = false;
	if (Params->TryGetBoolField(TEXT("expose_to_library"), bExposeToLibrary))
	{
		NewFunction->bExposeToLibrary = bExposeToLibrary;
	}

	const TArray<TSharedPtr<FJsonValue>>* CategoriesArray;
	if (Params->TryGetArrayField(TEXT("library_categories"), CategoriesArray))
	{
		NewFunction->LibraryCategoriesText.Empty();
		for (const TSharedPtr<FJsonValue>& Val : *CategoriesArray)
		{
			NewFunction->LibraryCategoriesText.Add(FText::FromString(Val->AsString()));
		}
	}

	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewFunction);

	FString FilePath = FPackageName::LongPackageNameToFilename(PackagePath, FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	UPackage::SavePackage(Package, NewFunction, *FilePath, SaveArgs);

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("function_name"), FunctionName);
	ResultObj->SetStringField(TEXT("function_path"), PackagePath);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material function creation requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAddFunctionInput(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: function_name"));
	}

	FString InputName;
	if (!Params->TryGetStringField(TEXT("input_name"), InputName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: input_name"));
	}

	FString FunctionPath;
	UMaterialFunction* Function = LoadMaterialFunction(FunctionName, FunctionPath);
	if (!Function)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material function not found: %s"), *FunctionName));
	}

	int32 PosX = 0, PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
	{
		PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
	}

	UMaterialExpressionFunctionInput* NewInput = Cast<UMaterialExpressionFunctionInput>(
		UMaterialEditingLibrary::CreateMaterialExpressionEx(
			nullptr,
			Function,
			UMaterialExpressionFunctionInput::StaticClass(),
			nullptr,
			PosX, PosY,
			true
		)
	);

	if (!NewInput)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create function input expression"));
	}

	NewInput->InputName = FName(*InputName);
	NewInput->Desc = InputName;

	FString InputTypeStr = TEXT("Scalar");
	Params->TryGetStringField(TEXT("input_type"), InputTypeStr);
	NewInput->InputType = StringToFunctionInputType(InputTypeStr);

	double SortPriority = 0;
	if (Params->TryGetNumberField(TEXT("sort_priority"), SortPriority))
	{
		NewInput->SortPriority = static_cast<int32>(SortPriority);
	}

	FString InputDescription;
	if (Params->TryGetStringField(TEXT("description"), InputDescription))
	{
		NewInput->Description = InputDescription;
	}

	bool bUsePreviewValueAsDefault = false;
	if (Params->TryGetBoolField(TEXT("use_preview_value_as_default"), bUsePreviewValueAsDefault))
	{
		NewInput->bUsePreviewValueAsDefault = bUsePreviewValueAsDefault;
	}

	const TArray<TSharedPtr<FJsonValue>>* PreviewArray;
	if (Params->TryGetArrayField(TEXT("preview_value"), PreviewArray) && PreviewArray->Num() >= 1)
	{
		FVector4f PreviewVal(0, 0, 0, 0);
		PreviewVal.X = static_cast<float>((*PreviewArray)[0]->AsNumber());
		if (PreviewArray->Num() >= 2) PreviewVal.Y = static_cast<float>((*PreviewArray)[1]->AsNumber());
		if (PreviewArray->Num() >= 3) PreviewVal.Z = static_cast<float>((*PreviewArray)[2]->AsNumber());
		if (PreviewArray->Num() >= 4) PreviewVal.W = static_cast<float>((*PreviewArray)[3]->AsNumber());
		NewInput->PreviewValue = PreviewVal;
	}
	else
	{
		double PreviewScalar;
		if (Params->TryGetNumberField(TEXT("preview_value"), PreviewScalar))
		{
			NewInput->PreviewValue = FVector4f(static_cast<float>(PreviewScalar), 0, 0, 0);
		}
	}

	NewInput->ConditionallyGenerateId(true);
	Function->GetPackage()->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("input_name"), InputName);
	ResultObj->SetStringField(TEXT("input_type"), InputTypeStr);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material function editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAddFunctionOutput(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: function_name"));
	}

	FString OutputName;
	if (!Params->TryGetStringField(TEXT("output_name"), OutputName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: output_name"));
	}

	FString FunctionPath;
	UMaterialFunction* Function = LoadMaterialFunction(FunctionName, FunctionPath);
	if (!Function)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material function not found: %s"), *FunctionName));
	}

	int32 PosX = 0, PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
	{
		PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
	}

	UMaterialExpressionFunctionOutput* NewOutput = Cast<UMaterialExpressionFunctionOutput>(
		UMaterialEditingLibrary::CreateMaterialExpressionEx(
			nullptr,
			Function,
			UMaterialExpressionFunctionOutput::StaticClass(),
			nullptr,
			PosX, PosY,
			true
		)
	);

	if (!NewOutput)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create function output expression"));
	}

	NewOutput->OutputName = FName(*OutputName);
	NewOutput->Desc = OutputName;

	double SortPriority = 0;
	if (Params->TryGetNumberField(TEXT("sort_priority"), SortPriority))
	{
		NewOutput->SortPriority = static_cast<int32>(SortPriority);
	}

	FString OutputDescription;
	if (Params->TryGetStringField(TEXT("description"), OutputDescription))
	{
		NewOutput->Description = OutputDescription;
	}

	NewOutput->ConditionallyGenerateId(true);
	Function->GetPackage()->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("output_name"), OutputName);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material function editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAddFunctionNode(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: function_name"));
	}

	FString NodeType;
	if (!Params->TryGetStringField(TEXT("node_type"), NodeType))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: node_type"));
	}

	FString FunctionPath;
	UMaterialFunction* Function = LoadMaterialFunction(FunctionName, FunctionPath);
	if (!Function)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material function not found: %s"), *FunctionName));
	}

	UClass* ExpressionClass = GetExpressionClassFromType(NodeType);
	if (!ExpressionClass)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Unknown node type: %s"), *NodeType));
	}

	int32 PosX = 0, PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
	{
		PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
	}

	UObject* SelectedAsset = nullptr;
	FString TexturePath;
	if (Params->TryGetStringField(TEXT("texture_path"), TexturePath))
	{
		SelectedAsset = UEditorAssetLibrary::LoadAsset(TexturePath);
	}

	UMaterialExpression* NewExpression = UMaterialEditingLibrary::CreateMaterialExpressionEx(
		nullptr,
		Function,
		ExpressionClass,
		SelectedAsset,
		PosX, PosY,
		true
	);

	if (!NewExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create expression in function"));
	}

	FString NodeName;
	if (Params->TryGetStringField(TEXT("node_name"), NodeName))
	{
		NewExpression->Desc = NodeName;

		if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->ParameterName = FName(*NodeName);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VectorParam->ParameterName = FName(*NodeName);
		}
		else if (UMaterialExpressionTextureSampleParameter* TextureParam = Cast<UMaterialExpressionTextureSampleParameter>(NewExpression))
		{
			TextureParam->ParameterName = FName(*NodeName);
		}
	}

	// Set parameter group if provided
	FString GroupName;
	if (Params->TryGetStringField(TEXT("group"), GroupName))
	{
		if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->Group = FName(*GroupName);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VectorParam->Group = FName(*GroupName);
		}
		else if (UMaterialExpressionTextureSampleParameter* TextureParam = Cast<UMaterialExpressionTextureSampleParameter>(NewExpression))
		{
			TextureParam->Group = FName(*GroupName);
		}
	}

	double Value = 0.0;
	if (Params->TryGetNumberField(TEXT("value"), Value))
	{
		if (UMaterialExpressionConstant* ConstExpr = Cast<UMaterialExpressionConstant>(NewExpression))
		{
			ConstExpr->R = static_cast<float>(Value);
		}
		else if (UMaterialExpressionScalarParameter* ScalarParam = Cast<UMaterialExpressionScalarParameter>(NewExpression))
		{
			ScalarParam->DefaultValue = static_cast<float>(Value);
		}
	}

	const TArray<TSharedPtr<FJsonValue>>* ColorArray;
	if (Params->TryGetArrayField(TEXT("color"), ColorArray) && ColorArray->Num() >= 3)
	{
		float R = static_cast<float>((*ColorArray)[0]->AsNumber());
		float G = static_cast<float>((*ColorArray)[1]->AsNumber());
		float B = static_cast<float>((*ColorArray)[2]->AsNumber());
		float A = ColorArray->Num() >= 4 ? static_cast<float>((*ColorArray)[3]->AsNumber()) : 1.0f;

		if (UMaterialExpressionConstant3Vector* Vec3 = Cast<UMaterialExpressionConstant3Vector>(NewExpression))
		{
			Vec3->Constant = FLinearColor(R, G, B);
		}
		else if (UMaterialExpressionConstant4Vector* Vec4 = Cast<UMaterialExpressionConstant4Vector>(NewExpression))
		{
			Vec4->Constant = FLinearColor(R, G, B, A);
		}
		else if (UMaterialExpressionVectorParameter* VectorParam = Cast<UMaterialExpressionVectorParameter>(NewExpression))
		{
			VectorParam->DefaultValue = FLinearColor(R, G, B, A);
		}
	}

	// Handle Custom HLSL expression
	if (UMaterialExpressionCustom* CustomExpr = Cast<UMaterialExpressionCustom>(NewExpression))
	{
		FString Code;
		if (Params->TryGetStringField(TEXT("code"), Code))
		{
			CustomExpr->Code = Code;
		}
		FString OutputTypeStr;
		if (Params->TryGetStringField(TEXT("output_type"), OutputTypeStr))
		{
			if (OutputTypeStr.Equals(TEXT("float"), ESearchCase::IgnoreCase)) CustomExpr->OutputType = CMOT_Float1;
			else if (OutputTypeStr.Equals(TEXT("float2"), ESearchCase::IgnoreCase)) CustomExpr->OutputType = CMOT_Float2;
			else if (OutputTypeStr.Equals(TEXT("float3"), ESearchCase::IgnoreCase)) CustomExpr->OutputType = CMOT_Float3;
			else if (OutputTypeStr.Equals(TEXT("float4"), ESearchCase::IgnoreCase)) CustomExpr->OutputType = CMOT_Float4;
		}
		const TArray<TSharedPtr<FJsonValue>>* InputsArray;
		if (Params->TryGetArrayField(TEXT("inputs"), InputsArray))
		{
			CustomExpr->Inputs.Empty();
			for (const TSharedPtr<FJsonValue>& InputVal : *InputsArray)
			{
				FCustomInput NewInput;
				NewInput.InputName = FName(*InputVal->AsString());
				CustomExpr->Inputs.Add(NewInput);
			}
		}
	}

	// Handle ComponentMask output_type
	if (UMaterialExpressionComponentMask* MaskExpr = Cast<UMaterialExpressionComponentMask>(NewExpression))
	{
		FString MaskType;
		if (Params->TryGetStringField(TEXT("output_type"), MaskType))
		{
			MaskExpr->R = MaskType.Contains(TEXT("R"));
			MaskExpr->G = MaskType.Contains(TEXT("G"));
			MaskExpr->B = MaskType.Contains(TEXT("B"));
			MaskExpr->A = MaskType.Contains(TEXT("A"));
		}
	}

	// Handle Named Reroute Declaration — set display name and optional color
	if (UMaterialExpressionNamedRerouteDeclaration* NamedReroute = Cast<UMaterialExpressionNamedRerouteDeclaration>(NewExpression))
	{
		if (!NodeName.IsEmpty())
		{
			NamedReroute->Name = FName(*NodeName);
		}
		const TArray<TSharedPtr<FJsonValue>>* NodeColorArray;
		if (Params->TryGetArrayField(TEXT("node_color"), NodeColorArray) && NodeColorArray->Num() >= 3)
		{
			float R = static_cast<float>((*NodeColorArray)[0]->AsNumber());
			float G = static_cast<float>((*NodeColorArray)[1]->AsNumber());
			float B = static_cast<float>((*NodeColorArray)[2]->AsNumber());
			float A = NodeColorArray->Num() >= 4 ? static_cast<float>((*NodeColorArray)[3]->AsNumber()) : 1.0f;
			NamedReroute->NodeColor = FLinearColor(R, G, B, A);
		}
	}

	Function->GetPackage()->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_name"), NewExpression->Desc.IsEmpty() ? NewExpression->GetName() : NewExpression->Desc);
	ResultObj->SetStringField(TEXT("node_type"), NodeType);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material function editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleConnectFunctionNodes(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: function_name"));
	}

	FString FromNode, ToNode, ToInput;
	if (!Params->TryGetStringField(TEXT("from_node"), FromNode))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: from_node"));
	}
	if (!Params->TryGetStringField(TEXT("to_node"), ToNode))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: to_node"));
	}
	if (!Params->TryGetStringField(TEXT("to_input"), ToInput))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: to_input"));
	}

	FString FromOutput = TEXT("");
	Params->TryGetStringField(TEXT("from_output"), FromOutput);

	FString FunctionPath;
	UMaterialFunction* Function = LoadMaterialFunction(FunctionName, FunctionPath);
	if (!Function)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material function not found: %s"), *FunctionName));
	}

	UMaterialExpression* FromExpression = FindExpressionInFunction(Function, FromNode);
	if (!FromExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("From node not found: %s"), *FromNode));
	}

	UMaterialExpression* ToExpression = FindExpressionInFunction(Function, ToNode);
	if (!ToExpression)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("To node not found: %s"), *ToNode));
	}

	// If ToExpression resolved to a FunctionInput, check for a FunctionOutput with the
	// same name.  Connecting TO an input makes no sense (inputs only have output pins),
	// so prefer the output when there is a name collision.
	if (Cast<UMaterialExpressionFunctionInput>(ToExpression))
	{
		for (UMaterialExpression* Expr : Function->GetExpressions())
		{
			if (UMaterialExpressionFunctionOutput* FuncOutput = Cast<UMaterialExpressionFunctionOutput>(Expr))
			{
				if (FuncOutput->OutputName.ToString() == ToNode)
				{
					ToExpression = Expr;
					break;
				}
			}
		}
	}

	bool bSuccess = UMaterialEditingLibrary::ConnectMaterialExpressions(
		FromExpression,
		FromOutput,
		ToExpression,
		ToInput
	);

	if (!bSuccess)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to connect function nodes"));
	}

	Function->GetPackage()->MarkPackageDirty();

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material function editing requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleAddMaterialFunctionCall(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString MaterialName;
	if (!Params->TryGetStringField(TEXT("material_name"), MaterialName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: material_name"));
	}

	FString FunctionPath;
	if (!Params->TryGetStringField(TEXT("function_path"), FunctionPath))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: function_path"));
	}

	FString MaterialPath;
	UMaterial* Material = LoadMaterial(MaterialName, MaterialPath);
	if (!Material)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material not found: %s"), *MaterialName));
	}

	FString LoadedFuncPath;
	UMaterialFunction* Function = LoadMaterialFunction(FunctionPath, LoadedFuncPath);
	if (!Function)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material function not found: %s"), *FunctionPath));
	}

	int32 PosX = 0, PosY = 0;
	const TArray<TSharedPtr<FJsonValue>>* PositionArray;
	if (Params->TryGetArrayField(TEXT("position"), PositionArray) && PositionArray->Num() >= 2)
	{
		PosX = static_cast<int32>((*PositionArray)[0]->AsNumber());
		PosY = static_cast<int32>((*PositionArray)[1]->AsNumber());
	}

	UMaterialExpressionMaterialFunctionCall* FuncCallExpr = Cast<UMaterialExpressionMaterialFunctionCall>(
		UMaterialEditingLibrary::CreateMaterialExpressionEx(
			Material,
			nullptr,
			UMaterialExpressionMaterialFunctionCall::StaticClass(),
			nullptr,
			PosX, PosY,
			true
		)
	);

	if (!FuncCallExpr)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Failed to create MaterialFunctionCall expression"));
	}

	FString NodeName;
	if (Params->TryGetStringField(TEXT("node_name"), NodeName))
	{
		FuncCallExpr->Desc = NodeName;
	}

	FuncCallExpr->SetMaterialFunction(Function);

	Material->MarkPackageDirty();

	// Build response with input/output pin info
	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("node_name"), FuncCallExpr->Desc.IsEmpty() ? FuncCallExpr->GetName() : FuncCallExpr->Desc);

	TArray<TSharedPtr<FJsonValue>> InputsArr;
	for (const FFunctionExpressionInput& FuncInput : FuncCallExpr->FunctionInputs)
	{
		TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
		if (FuncInput.ExpressionInput)
		{
			InputObj->SetStringField(TEXT("name"), FuncInput.ExpressionInput->InputName.ToString());
			InputObj->SetStringField(TEXT("type"), UEnum::GetValueAsString(FuncInput.ExpressionInput->InputType.GetValue()));
		}
		InputsArr.Add(MakeShared<FJsonValueObject>(InputObj));
	}
	ResultObj->SetArrayField(TEXT("inputs"), InputsArr);

	TArray<TSharedPtr<FJsonValue>> OutputsArr;
	for (const FFunctionExpressionOutput& FuncOutput : FuncCallExpr->FunctionOutputs)
	{
		TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
		if (FuncOutput.ExpressionOutput)
		{
			OutputObj->SetStringField(TEXT("name"), FuncOutput.ExpressionOutput->OutputName.ToString());
		}
		OutputsArr.Add(MakeShared<FJsonValueObject>(OutputObj));
	}
	ResultObj->SetArrayField(TEXT("outputs"), OutputsArr);

	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material function call requires editor"));
#endif
}

TSharedPtr<FJsonObject> FUnrealMCPMaterialCommands::HandleGetMaterialFunctionInfo(const TSharedPtr<FJsonObject>& Params)
{
#if WITH_EDITOR
	FString FunctionName;
	if (!Params->TryGetStringField(TEXT("function_name"), FunctionName))
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Missing required parameter: function_name"));
	}

	FString FunctionPath;
	UMaterialFunction* Function = LoadMaterialFunction(FunctionName, FunctionPath);
	if (!Function)
	{
		return FUnrealMCPCommonUtils::CreateErrorResponse(FString::Printf(TEXT("Material function not found: %s"), *FunctionName));
	}

	TSharedPtr<FJsonObject> ResultObj = MakeShared<FJsonObject>();
	ResultObj->SetBoolField(TEXT("success"), true);
	ResultObj->SetStringField(TEXT("name"), Function->GetName());
	ResultObj->SetStringField(TEXT("path"), FunctionPath);
	ResultObj->SetStringField(TEXT("description"), Function->Description);
	ResultObj->SetBoolField(TEXT("expose_to_library"), Function->bExposeToLibrary);

	TArray<TSharedPtr<FJsonValue>> CategoriesArr;
	for (const FText& Category : Function->LibraryCategoriesText)
	{
		CategoriesArr.Add(MakeShared<FJsonValueString>(Category.ToString()));
	}
	ResultObj->SetArrayField(TEXT("library_categories"), CategoriesArr);

	// Inputs
	TArray<TSharedPtr<FJsonValue>> InputsArr;
	// Outputs
	TArray<TSharedPtr<FJsonValue>> OutputsArr;
	// Other nodes
	TArray<TSharedPtr<FJsonValue>> NodesArr;

	for (UMaterialExpression* Expr : Function->GetExpressions())
	{
		if (!Expr)
		{
			continue;
		}

		if (UMaterialExpressionFunctionInput* FuncInput = Cast<UMaterialExpressionFunctionInput>(Expr))
		{
			TSharedPtr<FJsonObject> InputObj = MakeShared<FJsonObject>();
			InputObj->SetStringField(TEXT("name"), FuncInput->InputName.ToString());
			InputObj->SetStringField(TEXT("type"), UEnum::GetValueAsString(FuncInput->InputType.GetValue()));
			InputObj->SetNumberField(TEXT("sort_priority"), FuncInput->SortPriority);
			InputObj->SetStringField(TEXT("description"), FuncInput->Description);
			InputObj->SetBoolField(TEXT("use_preview_value_as_default"), FuncInput->bUsePreviewValueAsDefault != 0);
			InputsArr.Add(MakeShared<FJsonValueObject>(InputObj));
		}
		else if (UMaterialExpressionFunctionOutput* FuncOutput = Cast<UMaterialExpressionFunctionOutput>(Expr))
		{
			TSharedPtr<FJsonObject> OutputObj = MakeShared<FJsonObject>();
			OutputObj->SetStringField(TEXT("name"), FuncOutput->OutputName.ToString());
			OutputObj->SetNumberField(TEXT("sort_priority"), FuncOutput->SortPriority);
			OutputObj->SetStringField(TEXT("description"), FuncOutput->Description);
			OutputsArr.Add(MakeShared<FJsonValueObject>(OutputObj));
		}
		else
		{
			TSharedPtr<FJsonObject> NodeObj = MakeShared<FJsonObject>();
			NodeObj->SetStringField(TEXT("name"), Expr->Desc.IsEmpty() ? Expr->GetName() : Expr->Desc);
			NodeObj->SetStringField(TEXT("type"), Expr->GetClass()->GetName());
			NodeObj->SetNumberField(TEXT("x"), Expr->MaterialExpressionEditorX);
			NodeObj->SetNumberField(TEXT("y"), Expr->MaterialExpressionEditorY);
			NodesArr.Add(MakeShared<FJsonValueObject>(NodeObj));
		}
	}

	ResultObj->SetArrayField(TEXT("inputs"), InputsArr);
	ResultObj->SetArrayField(TEXT("outputs"), OutputsArr);
	ResultObj->SetArrayField(TEXT("nodes"), NodesArr);

	return ResultObj;
#else
	return FUnrealMCPCommonUtils::CreateErrorResponse(TEXT("Material function inspection requires editor"));
#endif
}

// ============================================================================
// COMMAND REGISTRATION
// ============================================================================

void FUnrealMCPMaterialCommands::RegisterCommands(FMCPCommandRegistry& Registry)
{
	Registry.RegisterCommand(TEXT("create_material"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_material"), P); });
	Registry.RegisterCommand(TEXT("set_material_properties"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_material_properties"), P); });
	Registry.RegisterCommand(TEXT("add_material_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_material_node"), P); });
	Registry.RegisterCommand(TEXT("set_material_node_property"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_material_node_property"), P); });
	Registry.RegisterCommand(TEXT("connect_material_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_material_nodes"), P); });
	Registry.RegisterCommand(TEXT("connect_to_material_output"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_to_material_output"), P); });
	Registry.RegisterCommand(TEXT("delete_material_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("delete_material_node"), P); });
	Registry.RegisterCommand(TEXT("recompile_material"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("recompile_material"), P); });
	Registry.RegisterCommand(TEXT("create_material_instance"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_material_instance"), P); });
	Registry.RegisterCommand(TEXT("set_material_instance_parameter"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_material_instance_parameter"), P); });
	Registry.RegisterCommand(TEXT("get_material_instance_parameters"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_material_instance_parameters"), P); });
	Registry.RegisterCommand(TEXT("get_material_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_material_nodes"), P); });
	Registry.RegisterCommand(TEXT("layout_material_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("layout_material_nodes"), P); });
	Registry.RegisterCommand(TEXT("set_material_node_position"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("set_material_node_position"), P); });
	Registry.RegisterCommand(TEXT("link_named_reroute_usage"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("link_named_reroute_usage"), P); });
	Registry.RegisterCommand(TEXT("get_material_hierarchy"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_material_hierarchy"), P); });
	// Material Function Commands
	Registry.RegisterCommand(TEXT("create_material_function"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("create_material_function"), P); });
	Registry.RegisterCommand(TEXT("add_function_input"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_function_input"), P); });
	Registry.RegisterCommand(TEXT("add_function_output"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_function_output"), P); });
	Registry.RegisterCommand(TEXT("add_function_node"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_function_node"), P); });
	Registry.RegisterCommand(TEXT("connect_function_nodes"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("connect_function_nodes"), P); });
	Registry.RegisterCommand(TEXT("add_material_function_call"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("add_material_function_call"), P); });
	Registry.RegisterCommand(TEXT("get_material_function_info"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("get_material_function_info"), P); });
	Registry.RegisterCommand(TEXT("execute_material_batch"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("execute_material_batch"), P); });
	Registry.RegisterCommand(TEXT("execute_function_batch"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleCommand(TEXT("execute_function_batch"), P); });
	Registry.RegisterCommand(TEXT("build_material"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleBuildMaterial(P); });
	Registry.RegisterCommand(TEXT("get_material_preview"),
		[this](const TSharedPtr<FJsonObject>& P) { return HandleGetMaterialPreview(P); });
}
