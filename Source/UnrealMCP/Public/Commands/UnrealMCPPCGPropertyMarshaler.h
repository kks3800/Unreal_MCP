// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonValue.h"

/**
 * Reflection-based JSON <-> FProperty marshaler for PCG node settings.
 *
 * EDITOR ONLY. Walks FProperty trees recursively to apply JSON values to
 * arbitrary UPROPERTY fields on UPCGSettings subclasses. Enables add_pcg_node
 * and set_pcg_node_property to work uniformly across every PCG node type
 * without per-type C++ code.
 *
 * Task 8 ships the scalar-only stub (bool/int/float/string/name/enum).
 * Task 12 adds struct handling with convenience literals and FPCGAttributePropertySelector.
 * Tasks 13-14 add object/class/soft-ref + array/map/set handling.
 * Task 15 adds the property path resolver for nested access.
 * Task 16 adds the inverse (FProperty -> JSON) serializer.
 */
class UNREALMCP_API FUnrealMCPPCGPropertyMarshaler
{
public:
	/**
	 * Apply a JSON value to a single FProperty in a container.
	 * @param Container  Pointer to the UObject or struct containing the property.
	 * @param Property   The FProperty describing the field to assign.
	 * @param Value      JSON value to marshal into the property.
	 * @return Empty string on success, human-readable error on failure.
	 *
	 * Atomicity note: for scalar properties (bool/numeric/string/enum/object/
	 * soft-ref/struct), a non-empty return leaves the container unmodified.
	 * For container properties (FArrayProperty, FMapProperty, FSetProperty)
	 * the container is cleared before elements are marshaled, so a partial
	 * failure mid-element can leave the container in a partially-populated
	 * state. Callers needing all-or-nothing semantics should marshal into a
	 * scratch buffer and swap on success.
	 */
	static FString ApplyJsonValueToProperty(
		void* Container,
		FProperty* Property,
		const TSharedPtr<FJsonValue>& Value);

	/**
	 * Serialize a single FProperty value at Container back to a JSON value.
	 * Inverse of ApplyJsonValueToProperty. The pair forms a round-trip contract
	 * used by get_pcg_node_property, get_pcg_node_info, and export handlers.
	 *
	 * Returns FJsonValueNull on null inputs or unsupported types so callers can
	 * freely compose the result into larger JSON trees without per-field checks.
	 */
	static TSharedPtr<FJsonValue> SerializePropertyToJson(
		const void* Container,
		FProperty* Property);
};

/**
 * One segment of a dotted property path such as
 * "MeshEntries[2].Descriptor.StaticMesh".
 *
 * The parser cannot always decide at parse time whether a subscript is an
 * array index or a map key — "MyMap[42]" is legitimate for int-keyed maps.
 * So Parse stores BOTH the numeric interpretation (ArrayIndex, INDEX_NONE
 * if the subscript is not a non-negative integer) AND the raw subscript
 * text so the resolver can reinterpret once it knows the target property
 * type.
 *
 *  - Plain field (no subscript): RawSubscript is empty, ArrayIndex = INDEX_NONE.
 *  - Subscripted: RawSubscript is the bracketed text (trimmed, quotes NOT
 *    stripped); ArrayIndex is set only when the subscript parses cleanly
 *    as a non-negative integer with no decimal point or sign prefix.
 */
struct UNREALMCP_API FPCGPropertyPathStep
{
	FString FieldName;
	int32 ArrayIndex = INDEX_NONE;
	FString RawSubscript;
};

/**
 * Parses and walks dotted/bracketed property paths targeting fields on a
 * UPCGSettings instance (or any UStruct tree). Used by set/get/array-op
 * commands to reach nested FProperty locations without per-node C++ wiring.
 */
class UNREALMCP_API FPCGPropertyPathResolver
{
public:
	/**
	 * Tokenize a path like "Mesh.Entries[2].Descriptor" into ordered steps.
	 * Every bracketed subscript populates RawSubscript; ArrayIndex is also
	 * populated when the subscript is a clean non-negative integer. The
	 * resolver reinterprets the subscript against the target property type
	 * (FArrayProperty uses ArrayIndex, FMapProperty uses RawSubscript as a
	 * key).
	 * @return true on success. On failure, OutError names the failing segment.
	 */
	static bool Parse(const FString& Path, TArray<FPCGPropertyPathStep>& OutSteps, FString& OutError);

	/**
	 * Walk a parsed path against an in-memory root and return the leaf container
	 * + leaf FProperty so callers can marshal JSON into (or out of) the location.
	 *
	 * @param RootContainer     The UObject or struct value the path is relative to.
	 * @param RootStruct        The UStruct describing RootContainer's layout.
	 * @param Steps             Output of Parse().
	 * @param bStopAtArrayLeaf  When true, if the final step resolves to an
	 *                          FArrayProperty without a subscript, leave the
	 *                          array property itself as the leaf instead of
	 *                          descending. Used by array manipulation commands.
	 * @param OutFinalContainer Set to the memory owning the final property.
	 * @param OutFinalProperty  Set to the resolved leaf FProperty.
	 * @param OutError          Error message naming the failing segment on failure.
	 */
	static bool Resolve(
		void* RootContainer,
		UStruct* RootStruct,
		const TArray<FPCGPropertyPathStep>& Steps,
		bool bStopAtArrayLeaf,
		void*& OutFinalContainer,
		FProperty*& OutFinalProperty,
		FString& OutError);
};
