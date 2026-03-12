#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "CourseActor.generated.h"

class UPCGComponent;
class ACourseHoleActor;
class UBoxComponent;
/**
 * ACourseActor
 *
 * Course-level PCG actor. Runs once over the entire course (or a subset).
 * Drives systems that need awareness of multiple holes simultaneously —
 * e.g. course-wide dressing passes, global lighting hints, cross-hole PCG.
 *
 * Current state: stub. Owns the hole list and global/explicit mode toggle.
 *
 * Global mode:   HoleActors list is ignored. At BeginPlay, all
 *                ACourseHoleActors found in the level are collected automatically.
 *
 * Explicit mode: HoleActors list is used as-is. Allows targeting a specific
 *                subset of holes — useful for per-group PCG passes or
 *                multi-course levels in the future.
 */
UCLASS(HideCategories = (Replication, Networking, Input))
class COURSEGEN_API ACourseActor : public AActor
{
	GENERATED_BODY()

public:

	ACourseActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface

	// #region Configuration

	/**
	 * If true, HoleActors is populated at BeginPlay by collecting every
	 * ACourseHoleActor in the level. The list is hidden in the Details panel.
	 *
	 * If false, HoleActors is populated manually in the editor.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Course")
	bool bGlobalSpawner = true;


	/**
	 * Reference floor height for this course. PCG terrain generation reads
	 * CourseFloorZ.Z as the base elevation for terrain conformance and skirt depth.
	 * X and Y are ignored — drag the widget vertically to set the floor level.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course", meta = (MakeEditWidget = true))
	FVector CourseFloorZ = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable, Category = "Course")
	float GetCourseFloorZWS(){ return GetActorTransform().TransformPosition(CourseCeilingZ).Z;	}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course", meta = (MakeEditWidget = true))
	FVector CourseCeilingZ = FVector(0,0,1000);

	UFUNCTION(BlueprintCallable, Category = "Course")
	float GetCourseCeilingZWS() { return GetActorTransform().TransformPosition(CourseCeilingZ).Z; }


	/**
	 * Explicit list of holes this actor operates over.
	 * Only used when bGlobalSpawner is false.
	 * Populated automatically at BeginPlay when bGlobalSpawner is true.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Course",
		meta = (EditCondition = "!bGlobalSpawner"))
	TArray<TObjectPtr<ACourseHoleActor>> HoleActors;

	// #endregion

	// #region Interface

	/** Returns the resolved hole list — always valid after BeginPlay. */
	UFUNCTION(BlueprintCallable, Category = "Course")
	const TArray<ACourseHoleActor*> GetHoleActors() const;

	// #endregion

	// #region Components

	/** PCG component. Graph runs once over the full hole list. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Course|Components")
	TObjectPtr<UPCGComponent> PCGComponent;

	// #endregion


#pragma region BOUNDS


/**
 * World-space AABB enclosing all child spline components.
 * Recomputed every OnConstruction — stays live as splines are edited.
 * PCG graphs read this to spatially restrict world actor queries.
 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CourseHole|Bounds")
	TObjectPtr<UBoxComponent> HoleBoundsComponent;

	/**
	 * Uniform padding applied to the spline-derived AABB on all sides.
	 * Default 500cm — enough to capture nearby WorldBrushActors and boundary zone.
	 * Increase if PCG queries are missing actors just outside the green edge.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseHole|Bounds",
		meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2000.0", ForceUnits = "cm"))
	float BoundsPadding = 500.f;

	/**
	 * Returns the world-space AABB of this hole, derived from all child splines
	 * expanded by BoundsPadding on all sides.
	 * BP interface hook — override in subclass to provide a custom bounds box.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CourseHole|Bounds")
	FBox GetHoleBounds() const;
	virtual FBox GetHoleBounds_Implementation() const;

#pragma endregion


private:

	void CollectHoleActorsFromLevel();
	void RecomputeHoleBounds();


	
};