#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProceduralSlot.h"
#include "Data/CourseHoleDataAsset.h"
#include "Data/CourseGenDataAsset.h"

#include "CourseHoleActor.generated.h"

class UPCGComponent;
class UDynamicMeshComponent;
class UInstancedStaticMeshComponent;
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
 * Single actor representing one mini golf hole in the level.
 *
 * Responsibilities:
 *   Runtime  — reads UCourseHoleDataAsset, manages ISM patch visibility,
 *               exposes cup candidate selection to MonkeyGolfCore
 *   Editor   — maintains dynamic mesh preview from PCG output,
 *               bakes to static mesh asset on demand (WITH_EDITOR only)
 *
 * Placed once per hole in the level. Driven by a PCGComponent on the
 * same actor whose graph fulfills the V0 CourseGen tag contract.
 *
 * MonkeyGolfCore interacts via BlueprintNativeEvent hooks and
 * BlueprintCallable interface. No reverse dependency into MonkeyGolfCore.
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

	// #region Configuration
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseHole")
	TObjectPtr<UCourseGenDataAsset> CourseGenData;

	/**
	 * Data asset written by the bake operation.
	 * Read at BeginPlay to populate runtime state.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseHole")
	TObjectPtr<UCourseHoleDataAsset> HoleData;


	/** Index of this hole within the course. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseHole")
	int32 HoleIndex = 0;

	// #endregion

	// #region Candidate Selection

	/**
	 * How the active cup candidate is chosen at BeginPlay.
	 * Set to External if MonkeyGolfCore will call ActivateCupCandidate directly.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseHole|Selection")
	ECandidateSelectionMode SelectionMode = ECandidateSelectionMode::Random;

	/**
	 * Seed used when SelectionMode == Seeded.
	 * Same seed produces the same candidate selection every session.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseHole|Selection",
		meta = (EditCondition = "SelectionMode==ECandidateSelectionMode::Seeded"))
	int32 ExplicitSeed = 0;

	// #endregion

	// #region Runtime Interface

	/** Returns all available cup candidate transforms for this hole. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	TArray<FTransform> GetCupCandidates() const;

	/** Returns the tee spawn transform. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	FTransform GetTeeTransform() const;

	/**
	 * Activates a specific cup candidate by index.
	 * Hides the corresponding ISM patch, broadcasts OnCandidateActivated.
	 * No-op if already activated — call ResetActivation first to reselect.
	 */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	void ActivateCupCandidate(int32 Index);

	/** Resets activation state — allows re-selection. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	void ResetActivation();

	/** Returns the currently active candidate index. INDEX_NONE if not yet activated. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	int32 GetActiveCandidateIndex() const { return ActiveCandidateIndex; }

	/** Returns true if a candidate has been activated. */
	UFUNCTION(BlueprintCallable, Category = "CourseHole")
	bool IsActivated() const { return ActiveCandidateIndex != INDEX_NONE; }

	// #endregion

	// #region BlueprintNativeEvent Hooks

	/**
	 * Called after a candidate is successfully activated.
	 * MonkeyGolfCore BP subclass overrides this to spawn the cup actor,
	 * place the ball trigger, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "CourseHole")
	void OnCandidateActivated(int32 Index, FTransform CupTransform);
	virtual void OnCandidateActivated_Implementation(
		int32 Index, FTransform CupTransform) {
	}

	/**
	 * Called if required contract data is missing at BeginPlay.
	 * Override to handle gracefully in BP — log, show placeholder, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "CourseHole")
	void OnContractValidationFailed(FName MissingTag);
	virtual void OnContractValidationFailed_Implementation(FName MissingTag) {}

	// #endregion

	// #region Editor Bake Interface
#if WITH_EDITOR

	/**
	 * Returns existing HoleData asset or creates a new one.
	 * Called by the bake pipeline to get a writable data asset target.
	 */
	UCourseHoleDataAsset* GetOrCreateHoleDataAsset();

	/** Bakes current PCG output to a static mesh asset. Expensive — manual trigger. */
	UFUNCTION(CallInEditor, Category = "CourseHole|Bake")
	void BakeToStaticMesh();

	/** Rebuilds dynamic mesh preview from current PCG output. Fast — safe to call freely. */
	UFUNCTION(CallInEditor, Category = "CourseHole|Bake")
	void UpdatePreview();

	/** Clears the dynamic mesh preview. */
	UFUNCTION(CallInEditor, Category = "CourseHole|Bake")
	void ClearPreview();

	//~ Begin AActor interface (editor)
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostRegisterAllComponents() override;
	virtual void BeginDestroy() override;
	//~ End AActor interface (editor)

#endif // WITH_EDITOR
	// #endregion

protected:

	// #region Components

	/** Displays the baked static mesh green at runtime. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CourseHole|Components")
	TObjectPtr<UStaticMeshComponent> GreenMeshComponent;

	/** ISM holding all patch instances. All visible initially; active candidate hidden at runtime. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CourseHole|Components")
	TObjectPtr<UInstancedStaticMeshComponent> PatchISM;

	/**
	 * PCG component driving the course gen graph.
	 * Present in all builds — runtime PCG graph reads its output for patch hiding.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "CourseHole|Components")
	TObjectPtr<UPCGComponent> PCGComponent;

	// #endregion

private:

	// #region Runtime State

	/** Populated from HoleData at BeginPlay. */
	UPROPERTY()
	FCourseHoleSlotCollection SlotCollection;

	int32 ActiveCandidateIndex = INDEX_NONE;

	void InitialiseFromDataAsset();
	void BuildPatchISM();
	int32 ResolveAutoIndex() const;

	// #endregion

	// #region Editor Only Data
#if WITH_EDITORONLY_DATA

	/** Dynamic mesh preview — editor visualization only, not present in cooked builds. */
	UPROPERTY()
	TObjectPtr<UDynamicMeshComponent> PreviewMeshComponent;

	/** Asset name used when baking to static mesh. */
	UPROPERTY(EditAnywhere, Category = "CourseHole|Bake")
	FString BakedMeshName = TEXT("SM_Green_Baked");

	/** Output path for baked static mesh and data assets. */
	UPROPERTY(EditAnywhere, Category = "CourseHole|Bake", meta = (ContentDir))
	FDirectoryPath BakeOutputPath;

	/** If true, preview mesh updates automatically when PCG graph executes. */
	UPROPERTY(EditAnywhere, Category = "CourseHole|Bake")
	bool bAutoUpdatePreview = true;

#endif // WITH_EDITORONLY_DATA
	// #endregion

	// #region Editor Methods
#if WITH_EDITOR

	void RebuildPreviewFromPCG();
	bool ExtractSlotCollection(FCourseHoleSlotCollection& OutCollection) const;
	void CommitBakeToDataAsset(UStaticMesh* BakedMesh,
		const FCourseHoleSlotCollection& Collection);
	void BindPCGDelegate();
	void UnbindPCGDelegate();

	/**
	 * Finds an existing UStaticMesh asset at the given package path,
	 * or creates a new empty one if none exists.
	 * Required before calling CopyMeshToStaticMesh — the asset must exist first.
	 */
	UStaticMesh* CreateOrFindStaticMeshAsset(const FString& AssetPath);

	UFUNCTION()
	void OnPCGGenerationFinished(UPCGComponent* InPCGComponent);

#endif // WITH_EDITOR
	// #endregion
};
