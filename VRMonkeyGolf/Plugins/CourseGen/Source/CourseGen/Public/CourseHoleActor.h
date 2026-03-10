#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralSlot.h"
#include "Data/CourseHoleDataAsset.h"
#include "Data/CourseGenDataAsset.h"

#include "CourseHoleActor.generated.h"

class UPCGComponent;
class UStaticMeshComponent;

UENUM(BlueprintType)
enum class ECandidateSelectionMode : uint8
{
	First,    // Index 0 — deterministic, good for editor preview
	Random,   // Random from available candidates
	Seeded,   // Uses ExplicitSeed value
	External  // Caller must invoke ActivateCupCandidate manually
};

/**
 * ACourseHoleActor
 *
 * Data boundary class for one mini golf hole.
 * Owns all data the pipeline needs — does not execute any of it.
 *
 * At cook time: PCG reads CourseGenData and writes results back via HoleData.
 * At runtime:   MonkeyGolfCore reads the exposed API to drive gameplay.
 *
 * This class does not bake, preview, spawn, or manage any components actively.
 * PCG owns the cook pipeline. MonkeyGolfCore owns the runtime behavior.
 */
UCLASS(HideCategories = (Replication, Networking, Input))
class COURSEGEN_API ACourseHoleActor : public AActor
{
	GENERATED_BODY()

public:

	ACourseHoleActor();

	//~ Begin AActor interface
	virtual void BeginPlay() override;
	//~ End AActor interface

	// #region Data

	/** Visual + physics configuration shared across all holes on this course. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseHole")
	TObjectPtr<UCourseGenDataAsset> CourseGenData;

	/**
	 * Output data asset produced by the PCG bake pipeline.
	 * Contains baked green mesh ref, tee transform, and cup candidate slots.
	 * Read at BeginPlay to populate runtime state.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseHole")
	TObjectPtr<UCourseHoleDataAsset> HoleData;

	/** Index of this hole within the course. Used by PCG and runtime systems. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseHole")
	int32 HoleIndex = 0;

	// #endregion

	// #region Candidate Selection

	/**
	 * How the active cup candidate is chosen at BeginPlay.
	 * External = MonkeyGolfCore calls ActivateCupCandidate directly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseHole|Selection")
	ECandidateSelectionMode SelectionMode = ECandidateSelectionMode::Random;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseHole|Selection",
		meta = (EditCondition = "SelectionMode==ECandidateSelectionMode::Seeded"))
	int32 ExplicitSeed = 0;

	// #endregion

	// #region Runtime Interface
	// Read-only accessors into HoleData. MonkeyGolfCore calls these.
	// All return empty/identity if HoleData is not assigned.

	/** All available cup candidate transforms for this hole. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	TArray<FTransform> GetCupCandidates() const;

	/** Tee spawn transform. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	FTransform GetTeeTransform() const;

	/** Currently active candidate index. INDEX_NONE if not yet activated. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	int32 GetActiveCandidateIndex() const { return ActiveCandidateIndex; }

	/** True if a candidate has been activated. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	bool IsActivated() const { return ActiveCandidateIndex != INDEX_NONE; }

	/**
	 * Records the chosen candidate index.
	 * Does not spawn anything — notifies via OnCandidateActivated for
	 * MonkeyGolfCore to act on.
	 */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	void ActivateCupCandidate(int32 Index);

	/** Clears activation state. Allows re-selection. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	void ResetActivation();

	/**
	 * Resolves which candidate index to use based on SelectionMode.
	 * Returns INDEX_NONE if External or no candidates available.
	 * MonkeyGolfCore can call this to get the recommended index,
	 * then pass it to ActivateCupCandidate.
	 */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	int32 ResolveAutoIndex() const;

	// #endregion

	// #region BlueprintNativeEvent Hooks

	/**
	 * Called when ActivateCupCandidate records a valid selection.
	 * MonkeyGolfCore BP subclass overrides to spawn cup actor,
	 * hide ISM patch, set up ball trigger, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "CourseHole")
	void OnCandidateActivated(int32 Index, FTransform CupTransform);
	virtual void OnCandidateActivated_Implementation(int32 Index, FTransform CupTransform) {}

	/**
	 * Called at BeginPlay if HoleData is missing or invalid.
	 * Override to handle gracefully — show placeholder, log, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "CourseHole")
	void OnHoleDataMissing();
	virtual void OnHoleDataMissing_Implementation() {}

	// #endregion

	// #region Components
	// Present but passive — MonkeyGolfCore or PCG drives them, not this class.

	/** Displays the baked static mesh green. Assigned by PCG bake pipeline. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CourseHole|Components")
	TObjectPtr<UStaticMeshComponent> GreenMeshComponent;

	/** PCG component. Graph authored externally — this class does not trigger it. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CourseHole|Components")
	TObjectPtr<UPCGComponent> PCGComponent;

	// #endregion

private:

	// #region Runtime State

	int32 ActiveCandidateIndex = INDEX_NONE;

	/** Cached slot collection read from HoleData at BeginPlay. */
	UPROPERTY()
	FCourseHoleSlotCollection SlotCollection;

	void InitialiseFromDataAsset();

	// #endregion
};