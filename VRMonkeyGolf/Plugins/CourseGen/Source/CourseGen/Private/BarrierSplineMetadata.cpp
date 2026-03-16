// Copyright ProceduralArchitect. All Rights Reserved.

#include "BarrierSplineMetadata.h"
#include "BarrierSplineComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(BarrierSplineMetadata)

// ─────────────────────────────────────────────────────────────────────────────
// Helpers (file-scope, mirrors Water plugin's template approach)
// ─────────────────────────────────────────────────────────────────────────────

namespace BarrierSplineMetadataPrivate
{
	/**
	 * Ensures curve InVal keys are sequential integers and that the point count
	 * matches NumPoints, padding with DefaultValue or trimming the tail.
	 */
	template<class T>
	static void FixupCurve(FInterpCurve<T>& Curve, const T& DefaultValue, int32 NumPoints)
	{
		// Repair any drifted InVal keys (can happen after undo/redo or bad serialise)
		for (int32 i = 0; i < Curve.Points.Num(); ++i)
		{
			Curve.Points[i].InVal = static_cast<float>(i);
		}

		// Pad missing points
		while (Curve.Points.Num() < NumPoints)
		{
			const float InVal = Curve.Points.Num() > 0
				? Curve.Points.Last().InVal + 1.f
				: 0.f;
			Curve.Points.Add(FInterpCurvePoint<T>(InVal, DefaultValue));
		}

		// Trim excess points
		if (Curve.Points.Num() > NumPoints)
		{
			Curve.Points.RemoveAt(NumPoints, Curve.Points.Num() - NumPoints);
		}
	}

	/**
	 * If the point value still matches the old default, update it to the new
	 * default.  Returns true when a change was made.
	 */
	template<class T>
	static bool PropagateIfAtDefault(FInterpCurve<T>& Curve,
	                                 const T& PreviousDefault,
	                                 const T& NewDefault,
	                                 int32 PointIndex)
	{
		if (FMath::IsNearlyEqual(Curve.Points[PointIndex].OutVal, PreviousDefault)
			&& !FMath::IsNearlyEqual(PreviousDefault, NewDefault))
		{
			Curve.Points[PointIndex].OutVal = NewDefault;
			return true;
		}
		return false;
	}
} // namespace BarrierSplineMetadataPrivate

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UBarrierSplineMetadata::UBarrierSplineMetadata(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

// ─────────────────────────────────────────────────────────────────────────────
// Editor
// ─────────────────────────────────────────────────────────────────────────────

#if WITH_EDITOR
void UBarrierSplineMetadata::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_EDITORONLY_DATA
	FOnBarrierSplineMetadataChangedParams Params(PropertyChangedEvent);
	Params.BarrierSplineMetadata = this;
	Params.bUserTriggered        = true;
	OnChangeMetadata.Broadcast(Params);
#endif
}
#endif // WITH_EDITOR

// ─────────────────────────────────────────────────────────────────────────────
// USplineMetadata interface
// ─────────────────────────────────────────────────────────────────────────────

void UBarrierSplineMetadata::InsertPoint(int32 Index, float t, bool bClosedLoop)
{
	check(Index >= 0);
	Modify();

	const int32 NumPoints = BarrierHeight.Points.Num();
	const float InputKey  = static_cast<float>(Index);

	if (Index >= NumPoints)
	{
		// Past the end — just append
		AddPoint(InputKey);
		return;
	}

	// Lerp between the previous and current point values
	const int32 PrevIndex    = (bClosedLoop && Index == 0) ? NumPoints - 1 : Index - 1;
	const bool  bHasPrevious = (PrevIndex >= 0 && PrevIndex < NumPoints);

	float NewHeight = BarrierHeight.Points[Index].OutVal;
	float NewWidth  = BarrierWidth.Points[Index].OutVal;

	if (bHasPrevious)
	{
		NewHeight = FMath::LerpStable(BarrierHeight.Points[PrevIndex].OutVal, NewHeight, t);
		NewWidth  = FMath::LerpStable(BarrierWidth.Points[PrevIndex].OutVal,  NewWidth,  t);
	}

	BarrierHeight.Points.Insert(FInterpCurvePoint<float>(InputKey, NewHeight), Index);
	BarrierWidth.Points.Insert( FInterpCurvePoint<float>(InputKey, NewWidth),  Index);

	// Shift InVal keys for all points after the inserted one
	for (int32 i = Index + 1; i < BarrierHeight.Points.Num(); ++i)
	{
		BarrierHeight.Points[i].InVal += 1.f;
		BarrierWidth.Points[i].InVal  += 1.f;
	}
}

void UBarrierSplineMetadata::UpdatePoint(int32 Index, float t, bool bClosedLoop)
{
	const int32 NumPoints = BarrierHeight.Points.Num();
	check(Index >= 0 && Index < NumPoints);
	Modify();

	const int32 PrevIndex = (bClosedLoop && Index == 0)           ? NumPoints - 1 : Index - 1;
	const int32 NextIndex = (bClosedLoop && Index + 1 >= NumPoints) ? 0             : Index + 1;

	const bool bHasPrev = (PrevIndex >= 0 && PrevIndex < NumPoints);
	const bool bHasNext = (NextIndex >= 0 && NextIndex < NumPoints);

	if (bHasPrev && bHasNext)
	{
		BarrierHeight.Points[Index].OutVal = FMath::LerpStable(
			BarrierHeight.Points[PrevIndex].OutVal,
			BarrierHeight.Points[NextIndex].OutVal, t);

		BarrierWidth.Points[Index].OutVal = FMath::LerpStable(
			BarrierWidth.Points[PrevIndex].OutVal,
			BarrierWidth.Points[NextIndex].OutVal, t);
	}
}

void UBarrierSplineMetadata::RemovePoint(int32 Index)
{
	check(Index < BarrierHeight.Points.Num());
	Modify();

	BarrierHeight.Points.RemoveAt(Index);
	BarrierWidth.Points.RemoveAt(Index);

	// Repair InVal keys for the tail
	for (int32 i = Index; i < BarrierHeight.Points.Num(); ++i)
	{
		BarrierHeight.Points[i].InVal -= 1.f;
		BarrierWidth.Points[i].InVal  -= 1.f;
	}
}

void UBarrierSplineMetadata::DuplicatePoint(int32 Index)
{
	check(Index < BarrierHeight.Points.Num());
	Modify();

	BarrierHeight.Points.Insert(FInterpCurvePoint<float>(BarrierHeight.Points[Index]), Index);
	BarrierWidth.Points.Insert( FInterpCurvePoint<float>(BarrierWidth.Points[Index]),  Index);

	for (int32 i = Index + 1; i < BarrierHeight.Points.Num(); ++i)
	{
		BarrierHeight.Points[i].InVal += 1.f;
		BarrierWidth.Points[i].InVal  += 1.f;
	}
}
void UBarrierSplineMetadata::AddPoint(float InputKey)
{
	Modify();

	float DefaultHeight = 100.f;
	float DefaultWidth  = 10.f;

	// Component is always the direct owner now — no outer actor needed
	if (UBarrierSplineComponent* OwnerComp = 
		GetTypedOuter<UBarrierSplineComponent>())
	{
		DefaultHeight = OwnerComp->BarrierSplineDefaults.DefaultBarrierHeight;
		DefaultWidth  = OwnerComp->BarrierSplineDefaults.DefaultBarrierWidth;
	}

	if (BarrierHeight.Points.Num() > 0)
	{
		DefaultHeight = BarrierHeight.Points.Last().OutVal;
		DefaultWidth  = BarrierWidth.Points.Last().OutVal;
	}

	const float NewInputKey = static_cast<float>(BarrierHeight.Points.Num());
	BarrierHeight.Points.Emplace(NewInputKey, DefaultHeight);
	BarrierWidth.Points.Emplace(NewInputKey, DefaultWidth);
}
void UBarrierSplineMetadata::CopyPoint(const USplineMetadata* FromSplineMetadata,
                                        int32 FromIndex, int32 ToIndex)
{
	check(FromSplineMetadata != nullptr);

	if (const UBarrierSplineMetadata* From = Cast<UBarrierSplineMetadata>(FromSplineMetadata))
	{
		check(ToIndex   < BarrierHeight.Points.Num());
		check(FromIndex < From->BarrierHeight.Points.Num());
		Modify();

		BarrierHeight.Points[ToIndex].OutVal = From->BarrierHeight.Points[FromIndex].OutVal;
		BarrierWidth.Points[ToIndex].OutVal  = From->BarrierWidth.Points[FromIndex].OutVal;
	}
}

void UBarrierSplineMetadata::Reset(int32 NumPoints)
{
	Modify();
	BarrierHeight.Points.Reset(NumPoints);
	BarrierWidth.Points.Reset(NumPoints);
}

void UBarrierSplineMetadata::Fixup(int32 NumPoints, USplineComponent* SplineComp)
{
	// SplineComp is guaranteed to be our UBarrierSplineComponent
	const UBarrierSplineComponent* BarrierComp = CastChecked<UBarrierSplineComponent>(SplineComp);
	const FBarrierSplineCurveDefaults& Defaults = BarrierComp->BarrierSplineDefaults;

	BarrierSplineMetadataPrivate::FixupCurve(BarrierHeight, Defaults.DefaultBarrierHeight, NumPoints);
	BarrierSplineMetadataPrivate::FixupCurve(BarrierWidth,  Defaults.DefaultBarrierWidth,  NumPoints);
}

bool UBarrierSplineMetadata::PropagateDefaultValue(int32 PointIndex,
                                                    const FBarrierSplineCurveDefaults& PreviousDefaults,
                                                    const FBarrierSplineCurveDefaults& NewDefaults)
{
	bool bChanged = false;

	bChanged |= BarrierSplineMetadataPrivate::PropagateIfAtDefault(
		BarrierHeight, PreviousDefaults.DefaultBarrierHeight, NewDefaults.DefaultBarrierHeight, PointIndex);

	bChanged |= BarrierSplineMetadataPrivate::PropagateIfAtDefault(
		BarrierWidth, PreviousDefaults.DefaultBarrierWidth, NewDefaults.DefaultBarrierWidth, PointIndex);

	return bChanged;
}
