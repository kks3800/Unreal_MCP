// Copyright Epic Games, Inc. All Rights Reserved.

#include "Commands/UnrealMCPPCGPropertyMarshaler.h"
#include "Dom/JsonObject.h"
#include "Misc/PackageName.h"
#include "UObject/Class.h"
#include "UObject/UnrealType.h"
#include "UObject/EnumProperty.h"
#include "UObject/UnrealNames.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "Metadata/PCGAttributePropertySelector.h"

namespace
{
	// Pull a numeric JSON value as double, tolerating JSON numbers that arrived
	// as strings (common when clients serialize via language-specific encoders).
	// Returns false if the value cannot be interpreted as a number at all.
	bool TryAsNumber(const TSharedPtr<FJsonValue>& Value, double& OutNumber)
	{
		if (!Value.IsValid())
		{
			return false;
		}
		if (Value->Type == EJson::Number)
		{
			OutNumber = Value->AsNumber();
			return true;
		}
		if (Value->Type == EJson::String)
		{
			const FString AsStr = Value->AsString();
			if (AsStr.IsNumeric())
			{
				OutNumber = FCString::Atod(*AsStr);
				return true;
			}
		}
		return false;
	}

	// Resolve an enum value from either an integer or the enum value short name.
	// Accepts both short ("Sampler") and qualified ("EPCGSettingsType::Sampler") forms
	// because PCG property docs use the latter while UI tends to use the former.
	bool TryResolveEnumValue(UEnum* Enum, const TSharedPtr<FJsonValue>& Value, int64& OutValue)
	{
		if (!Enum || !Value.IsValid())
		{
			return false;
		}
		if (Value->Type == EJson::Number)
		{
			OutValue = static_cast<int64>(Value->AsNumber());
			return Enum->IsValidEnumValue(OutValue);
		}
		if (Value->Type == EJson::String)
		{
			const FString AsStr = Value->AsString();
			int64 Resolved = Enum->GetValueByNameString(AsStr);
			if (Resolved == INDEX_NONE)
			{
				// Qualified names like "EPCGSettingsType::Sampler" work directly;
				// short names need the enum prefix prepended.
				Resolved = Enum->GetValueByName(FName(*FString::Printf(TEXT("%s::%s"), *Enum->GetName(), *AsStr)));
			}
			if (Resolved != INDEX_NONE)
			{
				OutValue = Resolved;
				return true;
			}
		}
		return false;
	}

	//==========================================================================
	// Struct literal parsers (Task 12)
	//==========================================================================
	//
	// These let clients pass compact string forms like "1,2,3" instead of
	// {"X":1,"Y":2,"Z":3}. The object form is still accepted via the recursive
	// descent path in ApplyJsonValueToProperty — these parsers only fire when the
	// caller passes a JSON string AND the struct matches one of the known types.
	//
	// Why string literals: PCG nodes take a lot of FVector parameters and the
	// ergonomics of "1,2,3" vs three-key objects is a big quality-of-life win
	// for AI callers composing graphs from natural-language descriptions.

	// Split "1,2,3" into trimmed numeric components. Returns the parsed count;
	// does NOT validate Expected against Out.Num() so callers can do lenient
	// matching (FVector4 accepts three components with W=0 fallback).
	int32 SplitNumericCsv(const FString& Source, TArray<double>& OutComponents)
	{
		OutComponents.Reset();
		TArray<FString> Parts;
		Source.ParseIntoArray(Parts, TEXT(","), /*InCullEmpty=*/false);
		for (FString& Part : Parts)
		{
			Part.TrimStartAndEndInline();
			if (Part.IsEmpty() || !Part.IsNumeric())
			{
				OutComponents.Reset();
				return 0;
			}
			OutComponents.Add(FCString::Atod(*Part));
		}
		return OutComponents.Num();
	}

	// Parse a string into a vector struct. Handles Vector/Vector2D/Vector4/Vector3f
	// by reading the component count and writing back via the struct's properties.
	// Written against the compile-time types so we do not pay reflection cost on
	// the hot scalar fields at runtime.
	bool ParseVectorLiteral(void* StructAddr, const FString& Source, UScriptStruct* Struct)
	{
		if (!StructAddr || !Struct)
		{
			return false;
		}

		TArray<double> Components;
		const int32 Count = SplitNumericCsv(Source, Components);
		const FName StructName = Struct->GetFName();

		if (StructName == NAME_Vector)
		{
			if (Count < 3) { return false; }
			FVector& V = *static_cast<FVector*>(StructAddr);
			V.X = Components[0];
			V.Y = Components[1];
			V.Z = Components[2];
			return true;
		}
		if (StructName == NAME_Vector2D)
		{
			if (Count < 2) { return false; }
			FVector2D& V = *static_cast<FVector2D*>(StructAddr);
			V.X = Components[0];
			V.Y = Components[1];
			return true;
		}
		if (StructName == NAME_Vector4)
		{
			// Accept 3 components with W defaulting to 0 — matches how most clients
			// think about XYZ points vs full homogeneous coordinates.
			if (Count < 3) { return false; }
			FVector4& V = *static_cast<FVector4*>(StructAddr);
			V.X = Components[0];
			V.Y = Components[1];
			V.Z = Components[2];
			V.W = (Count >= 4) ? Components[3] : 0.0;
			return true;
		}
		if (StructName == FName(TEXT("Vector3f")))
		{
			if (Count < 3) { return false; }
			FVector3f& V = *static_cast<FVector3f*>(StructAddr);
			V.X = static_cast<float>(Components[0]);
			V.Y = static_cast<float>(Components[1]);
			V.Z = static_cast<float>(Components[2]);
			return true;
		}

		return false;
	}

	// Parse "pitch,yaw,roll" into an FRotator. Rotators store fields in the order
	// (Pitch, Yaw, Roll) but the CSV is written in that same natural order so no
	// reordering is needed here.
	bool ParseRotatorLiteral(void* StructAddr, const FString& Source, UScriptStruct* Struct)
	{
		if (!StructAddr || !Struct || Struct->GetFName() != NAME_Rotator)
		{
			return false;
		}

		TArray<double> Components;
		const int32 Count = SplitNumericCsv(Source, Components);
		if (Count < 3)
		{
			return false;
		}

		FRotator& R = *static_cast<FRotator*>(StructAddr);
		R.Pitch = Components[0];
		R.Yaw = Components[1];
		R.Roll = Components[2];
		return true;
	}

	// Parse "#RRGGBB[AA]" hex or "r,g,b[,a]" float triplet into FLinearColor.
	// Hex is the common case for art/design inputs; float form is for exact
	// scene-referred values that can exceed the 0-1 LDR range.
	bool ParseLinearColorLiteral(void* StructAddr, const FString& Source, UScriptStruct* Struct)
	{
		if (!StructAddr || !Struct || Struct->GetFName() != NAME_LinearColor)
		{
			return false;
		}

		FLinearColor& Color = *static_cast<FLinearColor*>(StructAddr);
		const FString Trimmed = Source.TrimStartAndEnd();

		if (Trimmed.StartsWith(TEXT("#")))
		{
			// Hex form: #RRGGBB or #RRGGBBAA. FColor::FromHex handles both lengths
			// plus the leading # and converts sRGB -> linear with standard gamma.
			const FColor SRGB = FColor::FromHex(Trimmed);
			Color = FLinearColor::FromSRGBColor(SRGB);
			return true;
		}

		TArray<double> Components;
		const int32 Count = SplitNumericCsv(Trimmed, Components);
		if (Count < 3)
		{
			return false;
		}
		Color.R = static_cast<float>(Components[0]);
		Color.G = static_cast<float>(Components[1]);
		Color.B = static_cast<float>(Components[2]);
		Color.A = (Count >= 4) ? static_cast<float>(Components[3]) : 1.0f;
		return true;
	}

	// Parse "loc_x,loc_y,loc_z|rot_p,rot_y,rot_r|scale_x,scale_y,scale_z" into
	// an FTransform. Pipe separator keeps each sub-triple unambiguous even when
	// rotator components are negative (a single comma list would be ambiguous).
	bool ParseTransformLiteral(void* StructAddr, const FString& Source, UScriptStruct* Struct)
	{
		if (!StructAddr || !Struct || Struct->GetFName() != NAME_Transform)
		{
			return false;
		}

		TArray<FString> Parts;
		Source.ParseIntoArray(Parts, TEXT("|"), /*InCullEmpty=*/false);
		if (Parts.Num() != 3)
		{
			return false;
		}

		TArray<double> LocComponents;
		TArray<double> RotComponents;
		TArray<double> ScaleComponents;
		if (SplitNumericCsv(Parts[0], LocComponents) < 3 ||
			SplitNumericCsv(Parts[1], RotComponents) < 3 ||
			SplitNumericCsv(Parts[2], ScaleComponents) < 3)
		{
			return false;
		}

		FTransform& T = *static_cast<FTransform*>(StructAddr);
		T.SetLocation(FVector(LocComponents[0], LocComponents[1], LocComponents[2]));
		T.SetRotation(FRotator(RotComponents[0], RotComponents[1], RotComponents[2]).Quaternion());
		T.SetScale3D(FVector(ScaleComponents[0], ScaleComponents[1], ScaleComponents[2]));
		return true;
	}

	// Try every known literal parser in priority order. Returns true if the
	// struct name matched any parser AND the string parsed successfully — a
	// false return means the caller should fall through to recursive descent.
	bool TryApplyStructLiteral(void* StructAddr, UScriptStruct* Struct, const FString& Literal)
	{
		if (!StructAddr || !Struct)
		{
			return false;
		}

		const FName StructName = Struct->GetFName();
		if (StructName == NAME_Vector || StructName == NAME_Vector2D ||
			StructName == NAME_Vector4 || StructName == FName(TEXT("Vector3f")))
		{
			return ParseVectorLiteral(StructAddr, Literal, Struct);
		}
		if (StructName == NAME_Rotator)
		{
			return ParseRotatorLiteral(StructAddr, Literal, Struct);
		}
		if (StructName == NAME_LinearColor)
		{
			return ParseLinearColorLiteral(StructAddr, Literal, Struct);
		}
		if (StructName == NAME_Transform)
		{
			return ParseTransformLiteral(StructAddr, Literal, Struct);
		}
		return false;
	}

	//==========================================================================
	// Object path normalization (Task 13)
	//==========================================================================
	//
	// Clients pass asset references as short package paths like "/Game/Foo/Bar".
	// StaticLoadObject expects the canonical object path "/Game/Foo/Bar.Bar"
	// (with the inner asset name suffixed). Normalize here so callers don't
	// need to know this distinction and can copy paths from anywhere in the
	// editor UI (which sometimes shows one form, sometimes the other).
	FString NormalizeObjectPath(const FString& Path)
	{
		// If the path already contains a '.' after the last '/', assume it is
		// the canonical Package.Object form and leave it alone. This also covers
		// the Package.InnerRenamedObject case where the inner name diverges.
		int32 LastSlash = INDEX_NONE;
		Path.FindLastChar(TEXT('/'), LastSlash);
		if (LastSlash != INDEX_NONE && Path.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd, Path.Len()) > LastSlash)
		{
			return Path;
		}

		// Treat the input as a package name and append the leaf name as the
		// inner object suffix. FPackageName::GetLongPackageAssetName handles the
		// trailing-slash and empty-segment edge cases.
		const FString AssetName = FPackageName::GetLongPackageAssetName(Path);
		if (AssetName.IsEmpty())
		{
			return Path;
		}
		return FString::Printf(TEXT("%s.%s"), *Path, *AssetName);
	}

	// PCG's attribute selector struct accepts strings like "$Density", "@Position.X",
	// or "MyAttribute". FPCGAttributePropertySelector::Update is the engine entry
	// point that parses this mini-language into the right (selection, property, name,
	// extra-names) combo so we don't have to re-implement the parse here.
	bool IsPCGAttributePropertySelector(UScriptStruct* Struct)
	{
		if (!Struct)
		{
			return false;
		}
		// Walk up the struct inheritance chain because PCG ships typed wrappers
		// (FPCGAttributePropertyInputSelector, FPCGAttributePropertyOutputSelector)
		// that share the same Update() surface but have distinct struct names.
		for (UScriptStruct* Cur = Struct; Cur != nullptr; Cur = Cast<UScriptStruct>(Cur->GetSuperStruct()))
		{
			if (Cur->GetFName() == FName(TEXT("PCGAttributePropertySelector")))
			{
				return true;
			}
		}
		return false;
	}
}

FString FUnrealMCPPCGPropertyMarshaler::ApplyJsonValueToProperty(
	void* Container,
	FProperty* Property,
	const TSharedPtr<FJsonValue>& Value)
{
	if (!Container || !Property)
	{
		return TEXT("Null container or property");
	}
	if (!Value.IsValid())
	{
		return TEXT("Null JSON value");
	}

	// --- Bool ---
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		if (Value->Type != EJson::Boolean)
		{
			return TEXT("Expected JSON boolean for bool property");
		}
		BoolProp->SetPropertyValue_InContainer(Container, Value->AsBool());
		return FString();
	}

	// --- Enum (FEnumProperty wraps an underlying numeric property) ---
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		int64 EnumValue = 0;
		if (!TryResolveEnumValue(EnumProp->GetEnum(), Value, EnumValue))
		{
			return FString::Printf(TEXT("Invalid value for enum %s"), *EnumProp->GetEnum()->GetName());
		}
		void* PropPtr = EnumProp->ContainerPtrToValuePtr<void>(Container);
		EnumProp->GetUnderlyingProperty()->SetIntPropertyValue(PropPtr, EnumValue);
		return FString();
	}

	// --- Byte-backed enum (legacy TEnumAsByte<...>) ---
	if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		if (ByteProp->Enum)
		{
			int64 EnumValue = 0;
			if (!TryResolveEnumValue(ByteProp->Enum, Value, EnumValue))
			{
				return FString::Printf(TEXT("Invalid value for enum %s"), *ByteProp->Enum->GetName());
			}
			ByteProp->SetPropertyValue_InContainer(Container, static_cast<uint8>(EnumValue));
			return FString();
		}

		// Plain uint8 — fall through to numeric handling.
		double AsNum = 0.0;
		if (!TryAsNumber(Value, AsNum))
		{
			return TEXT("Expected JSON number for byte property");
		}
		ByteProp->SetPropertyValue_InContainer(Container, static_cast<uint8>(AsNum));
		return FString();
	}

	// --- Numeric (int*/float/double) ---
	if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
	{
		double AsNum = 0.0;
		if (!TryAsNumber(Value, AsNum))
		{
			return TEXT("Expected JSON number for numeric property");
		}
		void* PropPtr = NumericProp->ContainerPtrToValuePtr<void>(Container);
		if (NumericProp->IsFloatingPoint())
		{
			NumericProp->SetFloatingPointPropertyValue(PropPtr, AsNum);
		}
		else
		{
			NumericProp->SetIntPropertyValue(PropPtr, static_cast<int64>(AsNum));
		}
		return FString();
	}

	// --- String / Name / Text ---
	if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		if (Value->Type != EJson::String)
		{
			return TEXT("Expected JSON string for string property");
		}
		StrProp->SetPropertyValue_InContainer(Container, Value->AsString());
		return FString();
	}

	if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		if (Value->Type != EJson::String)
		{
			return TEXT("Expected JSON string for name property");
		}
		NameProp->SetPropertyValue_InContainer(Container, FName(*Value->AsString()));
		return FString();
	}

	if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		if (Value->Type != EJson::String)
		{
			return TEXT("Expected JSON string for text property");
		}
		TextProp->SetPropertyValue_InContainer(Container, FText::FromString(Value->AsString()));
		return FString();
	}

	// --- Struct (FVector / FRotator / FLinearColor / FTransform / FPCGAttributePropertySelector / arbitrary) ---
	if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Container);
		UScriptStruct* ScriptStruct = StructProp->Struct;
		if (!StructAddr || !ScriptStruct)
		{
			return TEXT("Struct property has null storage or script struct");
		}

		// Path 1: string literal. Handle PCG attribute selector first because it
		// also accepts strings but with different semantics than the convenience
		// literals (it is its own mini-language).
		if (Value->Type == EJson::String)
		{
			const FString Literal = Value->AsString();

			if (IsPCGAttributePropertySelector(ScriptStruct))
			{
				FPCGAttributePropertySelector* Selector = static_cast<FPCGAttributePropertySelector*>(StructAddr);
				if (!Selector->Update(Literal))
				{
					return FString::Printf(
						TEXT("FPCGAttributePropertySelector::Update rejected literal '%s'"), *Literal);
				}
				return FString();
			}

			if (TryApplyStructLiteral(StructAddr, ScriptStruct, Literal))
			{
				return FString();
			}

			return FString::Printf(
				TEXT("Struct %s does not accept a string literal (expected JSON object with fields)"),
				*ScriptStruct->GetName());
		}

		// Path 2: JSON object -> recursive descent. Walk every key the caller
		// provided, find the matching sub-property on the struct, and marshal
		// recursively. Keys that don't resolve are collected and reported so the
		// caller gets a specific error rather than a silent partial write.
		if (Value->Type == EJson::Object)
		{
			const TSharedPtr<FJsonObject>& Obj = Value->AsObject();
			if (!Obj.IsValid())
			{
				return TEXT("Null struct JSON object");
			}

			TArray<FString> FieldErrors;
			for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Obj->Values)
			{
				FProperty* SubProperty = ScriptStruct->FindPropertyByName(FName(*Pair.Key));
				if (!SubProperty)
				{
					FieldErrors.Add(FString::Printf(
						TEXT("%s.%s: field not found"), *ScriptStruct->GetName(), *Pair.Key));
					continue;
				}

				// Recurse with the struct address as the container — sub-property
				// offsets are computed relative to the struct base the same way
				// as relative to a UObject base, so ContainerPtrToValuePtr works.
				const FString SubError = ApplyJsonValueToProperty(StructAddr, SubProperty, Pair.Value);
				if (!SubError.IsEmpty())
				{
					FieldErrors.Add(FString::Printf(TEXT("%s.%s: %s"),
						*ScriptStruct->GetName(), *Pair.Key, *SubError));
				}
			}

			if (FieldErrors.Num() > 0)
			{
				return FString::Join(FieldErrors, TEXT("; "));
			}
			return FString();
		}

		return FString::Printf(
			TEXT("Expected JSON object or string literal for struct %s"),
			*ScriptStruct->GetName());
	}

	// --- Class reference (FClassProperty must come before FObjectProperty because
	// FClassProperty is a subclass — CastField on the parent would match first). ---
	if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		if (Value->Type != EJson::String)
		{
			return TEXT("Expected JSON string (class path) for class property");
		}
		const FString Raw = Value->AsString();
		if (Raw.IsEmpty())
		{
			ClassProp->SetObjectPropertyValue_InContainer(Container, nullptr);
			return FString();
		}

		// Classes live in a package with the same name, so normalization matches
		// the object-path form: /Game/Foo/BP_Bar -> /Game/Foo/BP_Bar.BP_Bar_C is
		// what generated classes need. We try both the plain normalized form and
		// the "_C" suffix form because both naming conventions exist in practice.
		FString Normalized = NormalizeObjectPath(Raw);
		UClass* LoadedClass = LoadClass<UObject>(nullptr, *Normalized);
		if (!LoadedClass)
		{
			LoadedClass = LoadClass<UObject>(nullptr, *(Normalized + TEXT("_C")));
		}
		if (!LoadedClass)
		{
			return FString::Printf(TEXT("Failed to load class from path: %s"), *Raw);
		}

		if (ClassProp->MetaClass && !LoadedClass->IsChildOf(ClassProp->MetaClass))
		{
			return FString::Printf(
				TEXT("Class at %s is %s, expected subclass of %s"),
				*Raw, *LoadedClass->GetName(), *ClassProp->MetaClass->GetName());
		}

		ClassProp->SetObjectPropertyValue_InContainer(Container, LoadedClass);
		return FString();
	}

	// --- Soft class reference (must precede FSoftObjectProperty for the same
	// inheritance reason as FClassProperty above). ---
	if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Property))
	{
		if (Value->Type != EJson::String)
		{
			return TEXT("Expected JSON string (class path) for soft class property");
		}
		const FString Raw = Value->AsString();
		// Soft refs do not load — we store the path verbatim after normalization
		// so runtime resolution happens when the containing asset is loaded.
		const FSoftObjectPath SoftPath(Raw.IsEmpty() ? FString() : NormalizeObjectPath(Raw));
		void* PropPtr = SoftClassProp->ContainerPtrToValuePtr<void>(Container);
		SoftClassProp->SetPropertyValue(PropPtr, FSoftObjectPtr(SoftPath));
		return FString();
	}

	// --- Object reference (loaded eagerly so we can type-check against PropertyClass). ---
	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		if (Value->Type != EJson::String)
		{
			return TEXT("Expected JSON string (asset path) for object property");
		}
		const FString Raw = Value->AsString();
		if (Raw.IsEmpty())
		{
			ObjProp->SetObjectPropertyValue_InContainer(Container, nullptr);
			return FString();
		}

		const FString Normalized = NormalizeObjectPath(Raw);
		// StaticLoadObject with the property's expected class gives us type
		// checking for free — a Material at a StaticMesh slot returns nullptr.
		// We still do an explicit IsA check below to produce a clearer error.
		UObject* Loaded = StaticLoadObject(UObject::StaticClass(), nullptr, *Normalized);
		if (!Loaded)
		{
			return FString::Printf(TEXT("Failed to load asset: %s"), *Raw);
		}

		if (ObjProp->PropertyClass && !Loaded->IsA(ObjProp->PropertyClass))
		{
			return FString::Printf(
				TEXT("Asset at %s is a %s, expected %s"),
				*Raw, *Loaded->GetClass()->GetName(), *ObjProp->PropertyClass->GetName());
		}

		ObjProp->SetObjectPropertyValue_InContainer(Container, Loaded);
		return FString();
	}

	// --- Soft object reference (no load, stored as FSoftObjectPath). ---
	if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
	{
		if (Value->Type != EJson::String)
		{
			return TEXT("Expected JSON string (asset path) for soft object property");
		}
		const FString Raw = Value->AsString();
		const FSoftObjectPath SoftPath(Raw.IsEmpty() ? FString() : NormalizeObjectPath(Raw));
		void* PropPtr = SoftObjProp->ContainerPtrToValuePtr<void>(Container);
		SoftObjProp->SetPropertyValue(PropPtr, FSoftObjectPtr(SoftPath));
		return FString();
	}

	// --- Array (TArray<T>) ---
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		if (Value->Type != EJson::Array)
		{
			return TEXT("Expected JSON array for array property");
		}

		const TArray<TSharedPtr<FJsonValue>>& JsonArr = Value->AsArray();

		// ScriptArrayHelper adapts the raw FScriptArray memory behind Inner's
		// property system without needing to know the concrete element type.
		// We resize first then marshal element-by-element so per-element errors
		// roll up with the array index for actionable diagnostics.
		void* ArrayAddr = ArrayProp->ContainerPtrToValuePtr<void>(Container);
		FScriptArrayHelper ArrayHelper(ArrayProp, ArrayAddr);
		ArrayHelper.EmptyAndAddValues(JsonArr.Num());

		TArray<FString> ElementErrors;
		for (int32 Index = 0; Index < JsonArr.Num(); ++Index)
		{
			// GetRawPtr returns the raw element buffer — for non-container inner
			// properties (structs, scalars) this IS the container the recursive
			// ApplyJsonValueToProperty call expects, because Inner's offset is 0
			// relative to that buffer.
			uint8* ElementPtr = ArrayHelper.GetRawPtr(Index);
			const FString SubError = ApplyJsonValueToProperty(ElementPtr, ArrayProp->Inner, JsonArr[Index]);
			if (!SubError.IsEmpty())
			{
				ElementErrors.Add(FString::Printf(TEXT("[%d]: %s"), Index, *SubError));
			}
		}

		if (ElementErrors.Num() > 0)
		{
			return FString::Join(ElementErrors, TEXT("; "));
		}
		return FString();
	}

	// --- Map (TMap<K, V>) ---
	if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		if (Value->Type != EJson::Object)
		{
			return TEXT("Expected JSON object for map property");
		}

		const TSharedPtr<FJsonObject>& Obj = Value->AsObject();
		if (!Obj.IsValid())
		{
			return TEXT("Null map JSON object");
		}

		void* MapAddr = MapProp->ContainerPtrToValuePtr<void>(Container);
		FScriptMapHelper MapHelper(MapProp, MapAddr);
		MapHelper.EmptyValues();

		// JSON object keys are always strings, so we synthesize a JSON string
		// value from each key and feed it through the regular marshaler against
		// the map's KeyProp. That lets FName/FString/enum keys all work via the
		// existing scalar paths without a second parser here.
		TArray<FString> EntryErrors;
		for (const TPair<FString, TSharedPtr<FJsonValue>>& Pair : Obj->Values)
		{
			// AddDefaultValue_Invalid_NeedsRehash creates a default-constructed
			// (key, value) slot whose key MUST be overwritten before rehashing,
			// otherwise duplicate default-keys would collapse on Rehash.
			const int32 NewIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();

			// Map pairs are laid out as [key | value] contiguously. KeyProp's
			// Offset_Internal is 0 and ValueProp's Offset_Internal equals
			// MapLayout.ValueOffset (set by FMapProperty::LinkInternal), so the
			// pair pointer IS the correct "container" for BOTH key and value
			// marshaling — ContainerPtrToValuePtr applies the right offset once.
			// Using GetValuePtr here would double-offset because GetValuePtr
			// already adds ValueOffset and then ContainerPtrToValuePtr would
			// add it a second time, writing into the next pair or past end.
			uint8* PairPtr = MapHelper.GetKeyPtr(NewIndex);

			const TSharedPtr<FJsonValue> KeyAsJson = MakeShared<FJsonValueString>(Pair.Key);
			const FString KeyError = ApplyJsonValueToProperty(PairPtr, MapHelper.GetKeyProperty(), KeyAsJson);
			if (!KeyError.IsEmpty())
			{
				EntryErrors.Add(FString::Printf(TEXT("key '%s': %s"), *Pair.Key, *KeyError));
				continue;
			}

			const FString ValError = ApplyJsonValueToProperty(PairPtr, MapHelper.GetValueProperty(), Pair.Value);
			if (!ValError.IsEmpty())
			{
				EntryErrors.Add(FString::Printf(TEXT("value for '%s': %s"), *Pair.Key, *ValError));
			}
		}

		// Rehash is REQUIRED after bulk AddDefaultValue_Invalid_NeedsRehash
		// inserts — the hash table is not populated until this runs, and any
		// lookup on an un-rehashed map will crash.
		MapHelper.Rehash();

		if (EntryErrors.Num() > 0)
		{
			return FString::Join(EntryErrors, TEXT("; "));
		}
		return FString();
	}

	// --- Set (TSet<T>) ---
	if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		if (Value->Type != EJson::Array)
		{
			return TEXT("Expected JSON array for set property");
		}

		const TArray<TSharedPtr<FJsonValue>>& JsonArr = Value->AsArray();

		void* SetAddr = SetProp->ContainerPtrToValuePtr<void>(Container);
		FScriptSetHelper SetHelper(SetProp, SetAddr);
		SetHelper.EmptyElements();

		TArray<FString> ElementErrors;
		for (int32 Index = 0; Index < JsonArr.Num(); ++Index)
		{
			const int32 NewIndex = SetHelper.AddDefaultValue_Invalid_NeedsRehash();
			uint8* ElementPtr = SetHelper.GetElementPtr(NewIndex);
			const FString SubError = ApplyJsonValueToProperty(ElementPtr, SetHelper.GetElementProperty(), JsonArr[Index]);
			if (!SubError.IsEmpty())
			{
				ElementErrors.Add(FString::Printf(TEXT("[%d]: %s"), Index, *SubError));
			}
		}

		// Sets share the map's rehash requirement — TSet uses the same hash
		// table machinery, and default-constructed elements collide until
		// Rehash assigns them their real bucket positions.
		SetHelper.Rehash();

		if (ElementErrors.Num() > 0)
		{
			return FString::Join(ElementErrors, TEXT("; "));
		}
		return FString();
	}

	// Everything the marshaler knows about has been handled above. Return an
	// explicit reason so add_pcg_node's "skipped_properties" list tells clients
	// exactly which unsupported type they tripped on.
	return FString::Printf(
		TEXT("Unsupported property type %s"),
		*Property->GetClass()->GetName());
}

//==============================================================================
// SerializePropertyToJson (Task 16)
//==============================================================================
//
// Mirror-image of ApplyJsonValueToProperty. Every type handled on the input side
// must have a matching case here so set/get round-trip through the full schema.
// Where ApplyJsonValueToProperty accepts multiple input shapes for one type
// (e.g. structs accept string literal OR object), this function emits the form
// preferred for re-ingestion: convenience literals for the common math structs,
// JSON objects for everything else.

TSharedPtr<FJsonValue> FUnrealMCPPCGPropertyMarshaler::SerializePropertyToJson(
	const void* Container,
	FProperty* Property)
{
	if (!Container || !Property)
	{
		return MakeShared<FJsonValueNull>();
	}

	// --- Bool ---
	if (FBoolProperty* BoolProp = CastField<FBoolProperty>(Property))
	{
		return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue_InContainer(Container));
	}

	// --- Enum (emits the value name, not the integer, so round-trip keeps the
	// human-readable form the user passed in). ---
	if (FEnumProperty* EnumProp = CastField<FEnumProperty>(Property))
	{
		const void* PropPtr = EnumProp->ContainerPtrToValuePtr<void>(Container);
		const int64 Value = EnumProp->GetUnderlyingProperty()->GetSignedIntPropertyValue(PropPtr);
		if (UEnum* Enum = EnumProp->GetEnum())
		{
			return MakeShared<FJsonValueString>(Enum->GetNameStringByValue(Value));
		}
		return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
	}

	// --- Byte-backed enum or plain uint8 ---
	if (FByteProperty* ByteProp = CastField<FByteProperty>(Property))
	{
		const uint8 Value = ByteProp->GetPropertyValue_InContainer(Container);
		if (ByteProp->Enum)
		{
			return MakeShared<FJsonValueString>(ByteProp->Enum->GetNameStringByValue(static_cast<int64>(Value)));
		}
		return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
	}

	// --- Numeric (int*/float/double) ---
	if (FNumericProperty* NumericProp = CastField<FNumericProperty>(Property))
	{
		const void* PropPtr = NumericProp->ContainerPtrToValuePtr<void>(Container);
		double Number = 0.0;
		if (NumericProp->IsFloatingPoint())
		{
			Number = NumericProp->GetFloatingPointPropertyValue(PropPtr);
		}
		else
		{
			Number = static_cast<double>(NumericProp->GetSignedIntPropertyValue(PropPtr));
		}
		return MakeShared<FJsonValueNumber>(Number);
	}

	// --- String / Name / Text ---
	if (FStrProperty* StrProp = CastField<FStrProperty>(Property))
	{
		return MakeShared<FJsonValueString>(StrProp->GetPropertyValue_InContainer(Container));
	}
	if (FNameProperty* NameProp = CastField<FNameProperty>(Property))
	{
		return MakeShared<FJsonValueString>(NameProp->GetPropertyValue_InContainer(Container).ToString());
	}
	if (FTextProperty* TextProp = CastField<FTextProperty>(Property))
	{
		// FText serializes to its display string. Localization metadata is lost
		// in the round-trip — we accept this because PCG graph authoring does
		// not typically set localized node settings.
		return MakeShared<FJsonValueString>(TextProp->GetPropertyValue_InContainer(Container).ToString());
	}

	// --- Class reference (must precede FObjectProperty; see Apply side note). ---
	if (FClassProperty* ClassProp = CastField<FClassProperty>(Property))
	{
		UObject* Value = ClassProp->GetObjectPropertyValue_InContainer(Container);
		if (!Value)
		{
			return MakeShared<FJsonValueNull>();
		}
		return MakeShared<FJsonValueString>(Value->GetPathName());
	}

	// --- Soft class reference (must precede FSoftObjectProperty). ---
	if (FSoftClassProperty* SoftClassProp = CastField<FSoftClassProperty>(Property))
	{
		const void* PropPtr = SoftClassProp->ContainerPtrToValuePtr<void>(Container);
		const FSoftObjectPtr& SoftPtr = *static_cast<const FSoftObjectPtr*>(PropPtr);
		return MakeShared<FJsonValueString>(SoftPtr.ToString());
	}

	// --- Object reference ---
	if (FObjectProperty* ObjProp = CastField<FObjectProperty>(Property))
	{
		UObject* Value = ObjProp->GetObjectPropertyValue_InContainer(Container);
		if (!Value)
		{
			return MakeShared<FJsonValueNull>();
		}
		return MakeShared<FJsonValueString>(Value->GetPathName());
	}

	// --- Soft object reference ---
	if (FSoftObjectProperty* SoftObjProp = CastField<FSoftObjectProperty>(Property))
	{
		const void* PropPtr = SoftObjProp->ContainerPtrToValuePtr<void>(Container);
		const FSoftObjectPtr& SoftPtr = *static_cast<const FSoftObjectPtr*>(PropPtr);
		return MakeShared<FJsonValueString>(SoftPtr.ToString());
	}

	// --- Struct ---
	if (FStructProperty* StructProp = CastField<FStructProperty>(Property))
	{
		const void* StructAddr = StructProp->ContainerPtrToValuePtr<void>(Container);
		UScriptStruct* ScriptStruct = StructProp->Struct;
		if (!StructAddr || !ScriptStruct)
		{
			return MakeShared<FJsonValueNull>();
		}

		const FName StructName = ScriptStruct->GetFName();

		// Convenience-literal emission for the common math structs. Matches the
		// input form ApplyJsonValueToProperty accepts so clients can pipe set
		// output directly back into a set call.
		//
		// Precision: %.17g is the minimum that guarantees IEEE 754 double
		// round-trip, %.9g for float. Using the default %g (6 significant
		// digits) silently drops precision on FVector/Rotator/Transform.
		if (StructName == NAME_Vector)
		{
			const FVector& V = *static_cast<const FVector*>(StructAddr);
			return MakeShared<FJsonValueString>(FString::Printf(TEXT("%.17g,%.17g,%.17g"), V.X, V.Y, V.Z));
		}
		if (StructName == NAME_Vector2D)
		{
			const FVector2D& V = *static_cast<const FVector2D*>(StructAddr);
			return MakeShared<FJsonValueString>(FString::Printf(TEXT("%.17g,%.17g"), V.X, V.Y));
		}
		if (StructName == NAME_Vector4)
		{
			const FVector4& V = *static_cast<const FVector4*>(StructAddr);
			return MakeShared<FJsonValueString>(FString::Printf(TEXT("%.17g,%.17g,%.17g,%.17g"), V.X, V.Y, V.Z, V.W));
		}
		if (StructName == FName(TEXT("Vector3f")))
		{
			const FVector3f& V = *static_cast<const FVector3f*>(StructAddr);
			return MakeShared<FJsonValueString>(FString::Printf(TEXT("%.9g,%.9g,%.9g"), V.X, V.Y, V.Z));
		}
		if (StructName == NAME_Rotator)
		{
			const FRotator& R = *static_cast<const FRotator*>(StructAddr);
			return MakeShared<FJsonValueString>(FString::Printf(TEXT("%.17g,%.17g,%.17g"), R.Pitch, R.Yaw, R.Roll));
		}
		if (StructName == NAME_LinearColor)
		{
			// Float form preserves HDR range; hex would clip. Callers that want
			// hex can read the float triplet and convert themselves.
			const FLinearColor& C = *static_cast<const FLinearColor*>(StructAddr);
			return MakeShared<FJsonValueString>(FString::Printf(TEXT("%.9g,%.9g,%.9g,%.9g"), C.R, C.G, C.B, C.A));
		}
		if (StructName == NAME_Transform)
		{
			const FTransform& T = *static_cast<const FTransform*>(StructAddr);
			const FVector Loc = T.GetLocation();
			const FRotator Rot = T.Rotator();
			const FVector Scale = T.GetScale3D();
			return MakeShared<FJsonValueString>(FString::Printf(
				TEXT("%.17g,%.17g,%.17g|%.17g,%.17g,%.17g|%.17g,%.17g,%.17g"),
				Loc.X, Loc.Y, Loc.Z,
				Rot.Pitch, Rot.Yaw, Rot.Roll,
				Scale.X, Scale.Y, Scale.Z));
		}

		// PCG attribute selectors emit their own mini-language form via ToString.
		if (IsPCGAttributePropertySelector(ScriptStruct))
		{
			const FPCGAttributePropertySelector* Selector =
				static_cast<const FPCGAttributePropertySelector*>(StructAddr);
#if ENGINE_MINOR_VERSION >= 5
			return MakeShared<FJsonValueString>(Selector->ToString());
#else
			return MakeShared<FJsonValueString>(Selector->GetName().ToString());
#endif
		}

		// Fallback: object with each sub-property serialized recursively. Iterates
		// in declaration order so the emitted JSON key order is deterministic.
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();
		for (TFieldIterator<FProperty> It(ScriptStruct); It; ++It)
		{
			FProperty* SubProperty = *It;
			if (!SubProperty)
			{
				continue;
			}
			Obj->SetField(SubProperty->GetName(), SerializePropertyToJson(StructAddr, SubProperty));
		}
		return MakeShared<FJsonValueObject>(Obj);
	}

	// --- Array ---
	if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(Property))
	{
		const void* ArrayAddr = ArrayProp->ContainerPtrToValuePtr<void>(Container);
		// FScriptArrayHelper's non-const API operates on mutable raw memory but
		// read-only iteration does not mutate it — const_cast is needed because
		// the constructor signature takes void* even though we only read.
		FScriptArrayHelper ArrayHelper(ArrayProp, const_cast<void*>(ArrayAddr));
		TArray<TSharedPtr<FJsonValue>> Out;
		Out.Reserve(ArrayHelper.Num());
		for (int32 Index = 0; Index < ArrayHelper.Num(); ++Index)
		{
			const uint8* ElementPtr = ArrayHelper.GetRawPtr(Index);
			Out.Add(SerializePropertyToJson(ElementPtr, ArrayProp->Inner));
		}
		return MakeShared<FJsonValueArray>(Out);
	}

	// --- Map ---
	if (FMapProperty* MapProp = CastField<FMapProperty>(Property))
	{
		const void* MapAddr = MapProp->ContainerPtrToValuePtr<void>(Container);
		FScriptMapHelper MapHelper(MapProp, const_cast<void*>(MapAddr));
		TSharedPtr<FJsonObject> Obj = MakeShared<FJsonObject>();

		// Serialize keys as strings via ExportTextItem_Direct so the JSON object
		// can actually hold them as field names (JSON requires string keys). The
		// Apply path parses keys back through the scalar marshaler which covers
		// the same types ExportTextItem emits.
		for (FScriptMapHelper::FIterator It = MapHelper.CreateIterator(); It; ++It)
		{
			// PairPtr is the start of the (key, value) pair. KeyProp->Offset_Internal
			// is 0 so ExportTextItem_Direct can read the key from PairPtr directly,
			// and ValueProp->Offset_Internal already equals MapLayout.ValueOffset so
			// SerializePropertyToJson's ContainerPtrToValuePtr call on PairPtr
			// resolves to the value storage exactly once. Using GetValuePtr would
			// double-offset past the value into the next pair.

#if ENGINE_MINOR_VERSION >= 5
			const uint8* PairPtr = MapHelper.GetKeyPtr(It);
#else
			const uint8* PairPtr = MapHelper.GetKeyPtr(*It);
#endif

			FString KeyString;
			MapHelper.GetKeyProperty()->ExportTextItem_Direct(KeyString, PairPtr, nullptr, nullptr, PPF_None);

			Obj->SetField(KeyString, SerializePropertyToJson(PairPtr, MapHelper.GetValueProperty()));
		}
		return MakeShared<FJsonValueObject>(Obj);
	}

	// --- Set (emits as array; order matches insertion order from the iterator) ---
	if (FSetProperty* SetProp = CastField<FSetProperty>(Property))
	{
		const void* SetAddr = SetProp->ContainerPtrToValuePtr<void>(Container);
		FScriptSetHelper SetHelper(SetProp, const_cast<void*>(SetAddr));
		TArray<TSharedPtr<FJsonValue>> Out;
		Out.Reserve(SetHelper.Num());
		for (FScriptSetHelper::FIterator It = SetHelper.CreateIterator(); It; ++It)
		{

#if ENGINE_MINOR_VERSION >= 5
			const uint8* ElementPtr = SetHelper.GetElementPtr(It);
#else
			const uint8* ElementPtr = SetHelper.GetElementPtr(*It);
#endif
			Out.Add(SerializePropertyToJson(ElementPtr, SetHelper.GetElementProperty()));
		}
		return MakeShared<FJsonValueArray>(Out);
	}

	// Unsupported type — return null rather than throwing so the caller's
	// larger JSON tree stays well-formed even if one field fell off the happy
	// path. This mirrors the Apply side returning an error string for the same
	// case (both are survivable).
	return MakeShared<FJsonValueNull>();
}

//==============================================================================
// FPCGPropertyPathResolver (Task 15)
//==============================================================================
//
// Parses a dotted/bracketed path expression into an ordered list of steps and
// walks it against a live FProperty tree. Shared by set_pcg_node_property,
// get_pcg_node_property, and the array-op commands so they all speak the same
// path syntax with the same error reporting.

namespace
{
	// Returns true only for strings that represent a non-negative integer
	// with no sign prefix, no decimal point, and no leading/trailing whitespace.
	// FString::IsNumeric accepts "1.5" and "-3" which would both be wrong for
	// an array index, so we need this stricter check. Used by the path parser
	// to decide whether to pre-compute ArrayIndex from a subscript.
	bool IsCleanNonNegativeInteger(const FString& Candidate)
	{
		if (Candidate.IsEmpty())
		{
			return false;
		}
		for (int32 i = 0; i < Candidate.Len(); ++i)
		{
			const TCHAR Ch = Candidate[i];
			if (Ch < TEXT('0') || Ch > TEXT('9'))
			{
				return false;
			}
		}
		return true;
	}
}

bool FPCGPropertyPathResolver::Parse(
	const FString& Path,
	TArray<FPCGPropertyPathStep>& OutSteps,
	FString& OutError)
{
	OutSteps.Reset();
	OutError.Reset();

	if (Path.IsEmpty())
	{
		OutError = TEXT("Empty property path");
		return false;
	}

	// Manual scanner keeps us in control of error messages and avoids the
	// allocation overhead of a regex split that would fire on every call.
	// Grammar:
	//   path    := segment ('.' segment)*
	//   segment := identifier ('[' subscript ']')?
	//   subscript := any bracketed text, trimmed (no quoting needed)
	// The parser does NOT decide here whether a subscript is an array index
	// or a map key — that requires knowing the target property type, which
	// only the resolver has. We store both RawSubscript (always) and
	// ArrayIndex (only when the subscript is a clean non-negative integer)
	// and let the resolver dispatch.
	const int32 Len = Path.Len();
	int32 Pos = 0;
	while (Pos < Len)
	{
		FPCGPropertyPathStep Step;

		// Read field name: letters, digits, underscore until '.' or '['.
		const int32 NameStart = Pos;
		while (Pos < Len && Path[Pos] != TEXT('.') && Path[Pos] != TEXT('['))
		{
			++Pos;
		}
		if (Pos == NameStart)
		{
			OutError = FString::Printf(
				TEXT("Property path '%s' has empty segment at position %d"), *Path, NameStart);
			return false;
		}
		Step.FieldName = Path.Mid(NameStart, Pos - NameStart);

		// Optional subscript. Only one subscript per segment — chained
		// subscripts like Foo[0][1] would have to be written Foo[0].<field>[1]
		// since nested arrays require an intermediate field name (which is how
		// they are modeled in UE property trees anyway).
		if (Pos < Len && Path[Pos] == TEXT('['))
		{
			++Pos;
			const int32 SubStart = Pos;
			while (Pos < Len && Path[Pos] != TEXT(']'))
			{
				++Pos;
			}
			if (Pos >= Len)
			{
				OutError = FString::Printf(
					TEXT("Property path '%s' has unterminated subscript for field '%s'"),
					*Path, *Step.FieldName);
				return false;
			}
			const FString Subscript = Path.Mid(SubStart, Pos - SubStart).TrimStartAndEnd();
			++Pos; // consume ']'

			if (Subscript.IsEmpty())
			{
				OutError = FString::Printf(
					TEXT("Property path '%s' has empty subscript for field '%s'"),
					*Path, *Step.FieldName);
				return false;
			}

			Step.RawSubscript = Subscript;
			// Only pre-populate ArrayIndex when the subscript is unambiguously
			// a non-negative integer. Anything with a decimal, sign, or trailing
			// text stays as RawSubscript only and is interpreted by the resolver
			// against the target property's key type (for maps) or rejected as
			// non-numeric (for arrays).
			if (IsCleanNonNegativeInteger(Subscript))
			{
				Step.ArrayIndex = FCString::Atoi(*Subscript);
			}
		}

		OutSteps.Add(Step);

		// Dot separator between segments; EOS or non-dot is a parse error.
		if (Pos < Len)
		{
			if (Path[Pos] != TEXT('.'))
			{
				OutError = FString::Printf(
					TEXT("Property path '%s' has unexpected character '%c' at position %d"),
					*Path, Path[Pos], Pos);
				return false;
			}
			++Pos;
			if (Pos >= Len)
			{
				OutError = FString::Printf(
					TEXT("Property path '%s' ends with a trailing '.'"), *Path);
				return false;
			}
		}
	}

	return true;
}

bool FPCGPropertyPathResolver::Resolve(
	void* RootContainer,
	UStruct* RootStruct,
	const TArray<FPCGPropertyPathStep>& Steps,
	bool bStopAtArrayLeaf,
	void*& OutFinalContainer,
	FProperty*& OutFinalProperty,
	FString& OutError)
{
	OutFinalContainer = nullptr;
	OutFinalProperty = nullptr;
	OutError.Reset();

	if (!RootContainer || !RootStruct)
	{
		OutError = TEXT("Null root container or struct");
		return false;
	}
	if (Steps.Num() == 0)
	{
		OutError = TEXT("Empty property path steps");
		return false;
	}

	// Walk state: the struct describing the current container, and the raw
	// pointer to the current container memory. These get re-homed as we descend
	// through structs, array elements, and map values.
	void* CurContainer = RootContainer;
	UStruct* CurStruct = RootStruct;

	for (int32 StepIndex = 0; StepIndex < Steps.Num(); ++StepIndex)
	{
		const FPCGPropertyPathStep& Step = Steps[StepIndex];
		const bool bIsLastStep = (StepIndex == Steps.Num() - 1);

		FProperty* FieldProp = CurStruct ? CurStruct->FindPropertyByName(FName(*Step.FieldName)) : nullptr;
		if (!FieldProp)
		{
			OutError = FString::Printf(
				TEXT("Property path failed at '%s' - field not found on %s"),
				*Step.FieldName, CurStruct ? *CurStruct->GetName() : TEXT("<null>"));
			return false;
		}

		// Subscripted step: dispatch on the target property type. The parser
		// did NOT commit to "this is an array index" vs "this is a map key" at
		// parse time because a subscript like "42" is ambiguous (could be an
		// int-keyed map key, could be an array index). We decide here.
		if (!Step.RawSubscript.IsEmpty())
		{
			if (FArrayProperty* ArrayProp = CastField<FArrayProperty>(FieldProp))
			{
				// Arrays require integer subscripts only. Step.ArrayIndex is
				// INDEX_NONE if the parser saw anything non-integer (decimal,
				// sign, trailing text) — reject those explicitly rather than
				// silently truncating like Atoi would.
				if (Step.ArrayIndex == INDEX_NONE)
				{
					OutError = FString::Printf(
						TEXT("Property path failed at '%s[%s]' - array subscript must be a non-negative integer"),
						*Step.FieldName, *Step.RawSubscript);
					return false;
				}

				void* ArrayAddr = ArrayProp->ContainerPtrToValuePtr<void>(CurContainer);
				FScriptArrayHelper ArrayHelper(ArrayProp, ArrayAddr);
				if (!ArrayHelper.IsValidIndex(Step.ArrayIndex))
				{
					OutError = FString::Printf(
						TEXT("Property path failed at '%s[%d]' - index out of range (array size %d)"),
						*Step.FieldName, Step.ArrayIndex, ArrayHelper.Num());
					return false;
				}

				uint8* ElementPtr = ArrayHelper.GetRawPtr(Step.ArrayIndex);

				if (bIsLastStep)
				{
					// The leaf IS the element itself. Container is the element
					// buffer, property is the inner property (since the caller
					// wants to address the element, not the array).
					OutFinalContainer = ElementPtr;
					OutFinalProperty = ArrayProp->Inner;
					return true;
				}

				// Intermediate array element: to descend further the element
				// must be a struct (we cannot have "Arr[0].Field" where Arr[0]
				// is a scalar).
				FStructProperty* ElemStructProp = CastField<FStructProperty>(ArrayProp->Inner);
				if (!ElemStructProp)
				{
					OutError = FString::Printf(
						TEXT("Property path failed at '%s[%d]' - element type %s cannot be descended into"),
						*Step.FieldName, Step.ArrayIndex, *ArrayProp->Inner->GetClass()->GetName());
					return false;
				}
				CurContainer = ElementPtr;
				CurStruct = ElemStructProp->Struct;
				continue;
			}

			if (FMapProperty* MapProp = CastField<FMapProperty>(FieldProp))
			{
				void* MapAddr = MapProp->ContainerPtrToValuePtr<void>(CurContainer);
				FScriptMapHelper MapHelper(MapProp, MapAddr);

				// Build a temporary key matching MapHelper's key property and
				// ask the marshaler to populate it from the raw subscript text.
				// This reuses the scalar/enum/name path so any string-convertible
				// key type (including FIntProperty for "42"-style keys on
				// int-keyed maps) just works.
				FProperty* KeyProperty = MapHelper.GetKeyProperty();
				if (!KeyProperty)
				{
					OutError = FString::Printf(
						TEXT("Property path failed at '%s[%s]' - map has no key property"),
						*Step.FieldName, *Step.RawSubscript);
					return false;
				}

				// Scratch buffer sized to KeyProperty's storage requirements.
				TArray<uint8> KeyBuffer;
				KeyBuffer.SetNumZeroed(KeyProperty->GetSize());
				KeyProperty->InitializeValue(KeyBuffer.GetData());
				const TSharedPtr<FJsonValue> KeyAsJson = MakeShared<FJsonValueString>(Step.RawSubscript);
				const FString KeyError = FUnrealMCPPCGPropertyMarshaler::ApplyJsonValueToProperty(
					KeyBuffer.GetData(), KeyProperty, KeyAsJson);
				if (!KeyError.IsEmpty())
				{
					KeyProperty->DestroyValue(KeyBuffer.GetData());
					OutError = FString::Printf(
						TEXT("Property path failed at '%s[%s]' - could not marshal key: %s"),
						*Step.FieldName, *Step.RawSubscript, *KeyError);
					return false;
				}

				// FindMapIndexWithKey wants a pointer to a pair (key, value);
				// since only the key is compared we can pass the key buffer
				// directly — UE internally treats the pair pointer as "key
				// pointer" for Identical. (See FScriptMapHelper::FindMapIndexWithKey
				// in UnrealType.h line 5222.)
				const int32 Index = MapHelper.FindMapIndexWithKey(KeyBuffer.GetData());
				KeyProperty->DestroyValue(KeyBuffer.GetData());

				if (Index == INDEX_NONE)
				{
					OutError = FString::Printf(
						TEXT("Property path failed at '%s[%s]' - key not found in map"),
						*Step.FieldName, *Step.RawSubscript);
					return false;
				}

				// CRITICAL: pair pointer, NOT GetValuePtr(Index). ValueProp's
				// Offset_Internal is set by FMapProperty::LinkInternal to equal
				// MapLayout.ValueOffset, and GetValuePtr already adds that same
				// offset. Using GetValuePtr as the container would make
				// ContainerPtrToValuePtr add the offset a SECOND time, reading
				// past the value storage into the next pair or OOB memory.
				// GetKeyPtr(Index) returns the pair start (KeyProp offset = 0),
				// which is the correct container for ValueProp access.
				uint8* PairPtr = MapHelper.GetKeyPtr(Index);

				if (bIsLastStep)
				{
					// Leaf: final container is the pair pointer. The caller
					// passes (PairPtr, ValueProperty) to the marshaler and
					// ValueProperty->ContainerPtrToValuePtr(PairPtr) resolves
					// to the value storage exactly once.
					OutFinalContainer = PairPtr;
					OutFinalProperty = MapHelper.GetValueProperty();
					return true;
				}

				// Intermediate descent into a struct-typed map value: apply
				// the value property's offset exactly once via
				// ContainerPtrToValuePtr, then continue with the struct body
				// as the new container.
				FStructProperty* ValStructProp = CastField<FStructProperty>(MapHelper.GetValueProperty());
				if (!ValStructProp)
				{
					OutError = FString::Printf(
						TEXT("Property path failed at '%s[%s]' - value type %s cannot be descended into"),
						*Step.FieldName, *Step.RawSubscript, *MapHelper.GetValueProperty()->GetClass()->GetName());
					return false;
				}
				CurContainer = ValStructProp->ContainerPtrToValuePtr<void>(PairPtr);
				CurStruct = ValStructProp->Struct;
				continue;
			}

			// Subscript on something that isn't an array or map.
			OutError = FString::Printf(
				TEXT("Property path failed at '%s[%s]' - subscript on non-container property %s"),
				*Step.FieldName, *Step.RawSubscript, *FieldProp->GetClass()->GetName());
			return false;
		}

		// Plain field (no subscript). Leaf case returns the field directly;
		// intermediate case requires the field to be a struct so we can
		// continue descending.
		if (bIsLastStep)
		{
			// Array leaves are a special case for the array-op commands: they
			// want the FArrayProperty itself rather than an element, because
			// AddValue/RemoveAt operate on the array container.
			if (bStopAtArrayLeaf && FieldProp->IsA<FArrayProperty>())
			{
				OutFinalContainer = CurContainer;
				OutFinalProperty = FieldProp;
				return true;
			}

			OutFinalContainer = CurContainer;
			OutFinalProperty = FieldProp;
			return true;
		}

		if (FStructProperty* StructProp = CastField<FStructProperty>(FieldProp))
		{
			CurContainer = StructProp->ContainerPtrToValuePtr<void>(CurContainer);
			CurStruct = StructProp->Struct;
			continue;
		}

		// Instanced subobjects (e.g. UPCGStaticMeshSpawnerSettings::MeshSelectorParameters
		// pointing at a UPCGMeshSelectorWeighted instance). Chase the live pointer
		// and use the runtime class — this is what makes paths like
		// "MeshSelectorParameters.MeshEntries" work even though the intermediate
		// field is an ObjectProperty rather than a struct.
		if (FObjectProperty* ObjProp = CastField<FObjectProperty>(FieldProp))
		{
			UObject* Inner = ObjProp->GetObjectPropertyValue_InContainer(CurContainer);
			if (!Inner)
			{
				OutError = FString::Printf(
					TEXT("Property path failed at '%s' - intermediate object reference is null; assign the subobject before descending"),
					*Step.FieldName);
				return false;
			}
			CurContainer = Inner;
			CurStruct = Inner->GetClass();
			continue;
		}

		OutError = FString::Printf(
			TEXT("Property path failed at '%s' - intermediate field is %s, expected struct or object reference to descend into"),
			*Step.FieldName, *FieldProp->GetClass()->GetName());
		return false;
	}

	// Should be unreachable because every branch above returns on the last step.
	OutError = TEXT("Property path resolver reached end without a leaf (internal error)");
	return false;
}
