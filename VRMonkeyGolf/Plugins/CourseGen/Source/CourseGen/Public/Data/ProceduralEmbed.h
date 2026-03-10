#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"

#include "ProceduralEmbed.generated.h"

/**
 * FProceduralSlot
 *
 * A self-contained bundle describing one selectable procedural element.
 * Represents a single candidate position with all meshes needed to:
 *   - Display it visually
 *   - Simulate it physically
 *   - Cut it into a dynamic mesh (cook time)
 *   - Patch over the cut when inactive (runtime)
 *
 * The cup hole is the canonical first use case, but this struct is
 * intentionally generic — obstacles, tunnels, ramps, or any runtime-
 * selectable element can use the same pattern.
 *
 * Reconstructed from PCG tagged data by CourseGenRuntime at BeginPlay.
 * CourseGenCore never references this struct directly.
 */
USTRUCT(BlueprintType)
struct COURSEGEN_API FProceduralEmbed
{
	GENERATED_BODY()

	// ── Meshes ────────────────────────────────────────────────────────────────

	/**
	 * The visible representation of this element when active.
	 * For a cup: the hole ring/lip mesh placed at runtime.
	 * Null is valid — some slots are purely physical with no visual.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Embed")
	TSoftObjectPtr<UStaticMesh> VisualMesh;

	/**
	 * Mesh instanced by ISM to fill the cut when this slot is inactive.
	 * Must be flush with the green surface and match its material.
	 * Hidden at runtime when this slot is the active selection.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Embed")
	TSoftObjectPtr<UStaticMesh> PatchMesh;


    
	/**
	 * Mesh used to boolean-subtract from the green surface at cook time.
	 * Consumed by PCGSubtractMeshPoints during CourseGenCore bake.
	 * Not needed at runtime — null after bake is complete.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Embed")
	TSoftObjectPtr<UStaticMesh> CutterMesh;

	// ── Helpers ───────────────────────────────────────────────────────────────

	/** Returns true if this slot has the minimum data needed for runtime use. */
	bool IsValid() const
	{
		return !VisualMesh.IsNull();
	}

	/** Returns true if this slot can participate in a boolean cut at cook time. */
	bool CanCut() const
	{
		return !CutterMesh.IsNull();
		// CutterMesh null is acceptable — PCGSubtractMeshPoints falls back to cylinder
	}

	/** Returns true if this slot has a patch to hide at runtime. */
	bool HasPatch() const
	{
		return IsValid() && !PatchMesh.IsNull();
	}
};
