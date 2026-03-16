// Copyright ProceduralArchitect. All Rights Reserved.

#pragma once

#include "Components/SplineComponent.h"
#include "BarrierSplineMetadata.generated.h"

#define UE_API COURSEGEN_API

// ─────────────────────────────────────────────────────────────────────────────
// Editor-only change notification
// ─────────────────────────────────────────────────────────────────────────────

#if WITH_EDITOR

class UBarrierSplineMetadata;

struct FOnBarrierSplineMetadataChangedParams
{
	FOnBarrierSplineMetadataChangedParams(
		const FPropertyChangedEvent& InPropertyChangedEvent = FPropertyChangedEvent(/*InProperty=*/nullptr))
		: PropertyChangedEvent(InPropertyChangedEvent)
	{}

	FPropertyChangedEvent PropertyChangedEvent;
	UBarrierSplineMetadata* BarrierSplineMetadata = nullptr;
	bool bUserTriggered = false;
};

#endif // WITH_EDITOR

// ─────────────────────────────────────────────────────────────────────────────
// Per-component default values  (analogous to FWaterSplineCurveDefaults)
// These are propagated to new/existing spline points the same way Water does it.
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FBarrierSplineCurveDefaults
{
	GENERATED_BODY()

	FBarrierSplineCurveDefaults()
		: DefaultBarrierHeight(100.f)
		, DefaultBarrierWidth(10.f)
	{}

	/** Default height of the barrier wall at each spline point (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|Defaults")
	float DefaultBarrierHeight;

	/** Default thickness of the barrier wall at each spline point (cm) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrier|Defaults")
	float DefaultBarrierWidth;
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-point metadata stored alongside the spline curve
// ─────────────────────────────────────────────────────────────────────────────

UCLASS(MinimalAPI)
class UBarrierSplineMetadata : public USplineMetadata
{
	GENERATED_UCLASS_BODY()

public:

	// ── USplineMetadata interface ─────────────────────────────────────────────

	/** Insert a new point before Index, lerping between neighbours */
	UE_API virtual void InsertPoint(int32 Index, float t, bool bClosedLoop) override;

	/** Update point at Index by lerping between neighbours */
	UE_API virtual void UpdatePoint(int32 Index, float t, bool bClosedLoop) override;

	/** Append a point at the end, copying values from the current last point */
	UE_API virtual void AddPoint(float InputKey) override;

	UE_API virtual void RemovePoint(int32 Index) override;
	UE_API virtual void DuplicatePoint(int32 Index) override;
	UE_API virtual void CopyPoint(const USplineMetadata* FromSplineMetadata, int32 FromIndex, int32 ToIndex) override;
	UE_API virtual void Reset(int32 NumPoints) override;

	/** Re-synchronise curve point counts with the spline; reads defaults from the owning component */
	UE_API virtual void Fixup(int32 NumPoints, USplineComponent* SplineComp) override;

	/**
	 * If a point currently sits at PreviousDefaults values, update it to NewDefaults.
	 * Returns true if anything changed (triggers UpdateSpline on the component).
	 */
	UE_API bool PropagateDefaultValue(int32 PointIndex,
	                                  const FBarrierSplineCurveDefaults& PreviousDefaults,
	                                  const FBarrierSplineCurveDefaults& NewDefaults);

	// ── Per-point curves (exist in both editor and runtime) ───────────────────

	/** Height of the barrier wall at each spline point (cm) */
	UPROPERTY(EditAnywhere, Category = "Barrier")
	FInterpCurveFloat BarrierHeight;

	/** Thickness of the barrier wall at each spline point (cm) */
	UPROPERTY(EditAnywhere, Category = "Barrier")
	FInterpCurveFloat BarrierWidth;

	// ── Editor visualisation toggles ──────────────────────────────────────────

#if WITH_EDITORONLY_DATA
	UPROPERTY(EditAnywhere, Category = "Barrier", meta = (InlineEditConditionToggle))
	bool bShouldVisualizeBarrierHeight = false;

	UPROPERTY(EditAnywhere, Category = "Barrier", meta = (InlineEditConditionToggle))
	bool bShouldVisualizeBarrierWidth = false;

	/** Fired after any per-point metadata property changes in the editor */
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnBarrierSplineMetadataChanged,
	                                    const FOnBarrierSplineMetadataChangedParams&);
	FOnBarrierSplineMetadataChanged OnChangeMetadata;
#endif // WITH_EDITORONLY_DATA

#if WITH_EDITOR
	UE_API virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};

#undef UE_API
