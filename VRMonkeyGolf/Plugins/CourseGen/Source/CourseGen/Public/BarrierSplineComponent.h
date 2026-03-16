// Copyright ProceduralArchitect. All Rights Reserved.

#pragma once

#include "BarrierSplineMetadata.h"
#include "Components/PCGSplineComponent.h"   // PCGUtils plugin
#include "BarrierSplineMetadata.h"
#include "BarrierSplineComponent.generated.h"



// ─────────────────────────────────────────────────────────────────────────────
// Editor change notification  (analogous to FOnWaterSplineDataChanged)
// ─────────────────────────────────────────────────────────────────────────────

#if WITH_EDITOR
struct FOnBarrierSplineDataChangedParams
{
	FOnBarrierSplineDataChangedParams(
		const FPropertyChangedEvent& InPropertyChangedEvent = FPropertyChangedEvent(/*InProperty=*/nullptr))
		: PropertyChangedEvent(InPropertyChangedEvent)
	{}

	FPropertyChangedEvent PropertyChangedEvent;
	bool bUserTriggered = false;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBarrierSplineDataChanged,
                                    const FOnBarrierSplineDataChangedParams&);
#endif // WITH_EDITOR

// ─────────────────────────────────────────────────────────────────────────────
// Component
// ─────────────────────────────────────────────────────────────────────────────

/**
 * UBarrierSplineComponent
 *
 * Extends UPCGSplineComponent with per-point barrier metadata (height, width).
 * Follows the same defaults-propagation pattern as UWaterSplineComponent.
 *
 * Metadata is owned by the ACourseHoleActor that contains this component;
 * GetSplinePointsMetadata() routes through the actor exactly as the Water
 * plugin routes through AWaterBody.
 *
 * Lives in the CourseGen module.
 */
UCLASS(ClassGroup = "CourseGen",
       meta = (BlueprintSpawnableComponent),
       BlueprintType, Blueprintable)
class COURSEGEN_API UBarrierSplineComponent : public UPCGSplineComponent
{
	GENERATED_UCLASS_BODY()

public:

	// ── Defaults ─────────────────────────────────────────────────────────────

	/**
	 * Default barrier height/width propagated to spline points on this component.
	 * Changing these values in the details panel will update any point that still
	 * sits at the previous default (same behaviour as water spline width/depth).
	 */
	UPROPERTY(Category = "Barrier", EditDefaultsOnly, BlueprintReadOnly)
	FBarrierSplineCurveDefaults BarrierSplineDefaults;

	/**
	 * Snapshot of the last defaults that were propagated; used to detect which
	 * points the user has intentionally overridden vs. which are still at default.
	 */
	UPROPERTY()
	FBarrierSplineCurveDefaults PreviousBarrierSplineDefaults;

	
	UPROPERTY(VisibleAnywhere, Category = "Barrier")
	TObjectPtr<UBarrierSplineMetadata> Metadata;
	
	// ── USplineComponent interface ────────────────────────────────────────────

	 virtual USplineMetadata*       GetSplinePointsMetadata()       override;
	 virtual const USplineMetadata* GetSplinePointsMetadata() const override;

	// Scale editing is not meaningful for barrier splines
	virtual bool AllowsSplinePointScaleEditing() const override { return false; }

	// ── Public API ────────────────────────────────────────────────────────────

	/**
	 * Call after programmatically adding points via USplineComponent::AddPoint(s)
	 * to synchronise metadata and notify listeners.
	 * (Mirrors UWaterSplineComponent::K2_SynchronizeAndBroadcastDataChange)
	 */
	UFUNCTION(BlueprintCallable, Category = "Barrier",
	          DisplayName = "Synchronize And Broadcast Data Change")
	 void K2_SynchronizeAndBroadcastDataChange();

	/**
	 * Convenience: sample barrier height at an arbitrary distance along the spline.
	 * Uses the same input-key convention as USplineComponent::GetFloatPropertyAtSplineInputKey.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Barrier")
	 float GetBarrierHeightAtDistanceAlongSpline(float Distance) const;

	/**
	 * Convenience: sample barrier width at an arbitrary distance along the spline.
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Barrier")
	 float GetBarrierWidthAtDistanceAlongSpline(float Distance) const;

	// ── Editor delegate accessor ──────────────────────────────────────────────

#if WITH_EDITOR
	FOnBarrierSplineDataChanged& OnBarrierSplineDataChanged()
	{
		return BarrierSplineDataChangedEvent;
	}

	 virtual void PostEditUndo() override;
	 virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	 virtual void PostEditImport() override;

	/**
	 * Reset the spline to the given world-space positions and re-synchronise.
	 * Used by course generation tools.
	 */
	void ResetSpline(const TArray<FVector>& Points);

	/**
	 * Pull component defaults down into per-point metadata values.
	 * Returns true if anything changed (so callers know to call UpdateSpline).
	 */
	bool SynchronizeBarrierProperties();
#endif // WITH_EDITOR

	// ── UObject / UActorComponent overrides ──────────────────────────────────

	 virtual void PostLoad()                           override;
	 virtual void PostDuplicate(bool bDuplicateForPie) override;

private:

#if WITH_EDITOR
	FOnBarrierSplineDataChanged BarrierSplineDataChangedEvent;
#endif
};


