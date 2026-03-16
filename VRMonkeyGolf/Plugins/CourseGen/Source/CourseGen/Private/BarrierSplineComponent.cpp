// Copyright ProceduralArchitect. All Rights Reserved.

#include "BarrierSplineComponent.h"
#include "CourseHoleActor.h"    // ACourseHoleActor — outer actor

#include UE_INLINE_GENERATED_CPP_BY_NAME(BarrierSplineComponent)

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UBarrierSplineComponent::UBarrierSplineComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Metadata = CreateDefaultSubobject<UBarrierSplineMetadata>(TEXT("BarrierSplineMetadata"));
}

// ─────────────────────────────────────────────────────────────────────────────
// UObject overrides
// ─────────────────────────────────────────────────────────────────────────────

void UBarrierSplineComponent::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITOR
	SynchronizeBarrierProperties();
	// NOTE: Do not broadcast here — calling into script during PostLoad is illegal.
#endif
}

void UBarrierSplineComponent::PostDuplicate(bool bDuplicateForPie)
{
	Super::PostDuplicate(bDuplicateForPie);

#if WITH_EDITOR
	if (!bDuplicateForPie)
	{
		SynchronizeBarrierProperties();
		BarrierSplineDataChangedEvent.Broadcast(FOnBarrierSplineDataChangedParams());
	}
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Metadata routing  (mirrors UWaterSplineComponent::GetSplinePointsMetadata)
//
// The metadata object lives on the ACourseHoleActor, not on this component,
// so that it follows the same outer-actor ownership model as the Water plugin.
// ─────────────────────────────────────────────────────────────────────────────

USplineMetadata* UBarrierSplineComponent::GetSplinePointsMetadata()
{
	return Metadata;
}

const USplineMetadata* UBarrierSplineComponent::GetSplinePointsMetadata() const
{
	return Metadata;
}

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void UBarrierSplineComponent::K2_SynchronizeAndBroadcastDataChange()
{
#if WITH_EDITOR
	SynchronizeBarrierProperties();

	FPropertyChangedEvent PropertyChangedEvent(
		FindFProperty<FProperty>(UBarrierSplineComponent::StaticClass(), TEXT("SplineCurves")),
		EPropertyChangeType::ValueSet);

	FOnBarrierSplineDataChangedParams Params(PropertyChangedEvent);
	Params.bUserTriggered = true;
	BarrierSplineDataChangedEvent.Broadcast(Params);
#endif
}

float UBarrierSplineComponent::GetBarrierHeightAtDistanceAlongSpline(float Distance) const
{
	const UBarrierSplineMetadata* Meta = Cast<UBarrierSplineMetadata>(GetSplinePointsMetadata());
	if (!Meta || Meta->BarrierHeight.Points.Num() == 0)
	{
		return BarrierSplineDefaults.DefaultBarrierHeight;
	}

	const float InputKey = GetInputKeyValueAtDistanceAlongSpline(Distance);
	return Meta->BarrierHeight.Eval(InputKey, BarrierSplineDefaults.DefaultBarrierHeight);
}

float UBarrierSplineComponent::GetBarrierWidthAtDistanceAlongSpline(float Distance) const
{
	const UBarrierSplineMetadata* Meta = Cast<UBarrierSplineMetadata>(GetSplinePointsMetadata());
	if (!Meta || Meta->BarrierWidth.Points.Num() == 0)
	{
		return BarrierSplineDefaults.DefaultBarrierWidth;
	}

	const float InputKey = GetInputKeyValueAtDistanceAlongSpline(Distance);
	return Meta->BarrierWidth.Eval(InputKey, BarrierSplineDefaults.DefaultBarrierWidth);
}

// ─────────────────────────────────────────────────────────────────────────────
// Editor
// ─────────────────────────────────────────────────────────────────────────────

#if WITH_EDITOR

void UBarrierSplineComponent::PostEditUndo()
{
	Super::PostEditUndo();
	BarrierSplineDataChangedEvent.Broadcast(FOnBarrierSplineDataChangedParams());
}

void UBarrierSplineComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SynchronizeBarrierProperties();

	FOnBarrierSplineDataChangedParams Params(PropertyChangedEvent);
	Params.bUserTriggered = true;
	BarrierSplineDataChangedEvent.Broadcast(Params);
}

void UBarrierSplineComponent::PostEditImport()
{
	Super::PostEditImport();

	SynchronizeBarrierProperties();
	BarrierSplineDataChangedEvent.Broadcast(FOnBarrierSplineDataChangedParams());
}

void UBarrierSplineComponent::ResetSpline(const TArray<FVector>& Points)
{
	ClearSplinePoints(/*bUpdateSpline=*/false);
	PreviousBarrierSplineDefaults = BarrierSplineDefaults;

	for (const FVector& Pt : Points)
	{
		AddSplinePoint(Pt, ESplineCoordinateSpace::Local, /*bUpdateSpline=*/false);
	}

	UpdateSpline();
	SynchronizeBarrierProperties();
	BarrierSplineDataChangedEvent.Broadcast(FOnBarrierSplineDataChangedParams());
}

bool UBarrierSplineComponent::SynchronizeBarrierProperties()
{
	bool bAnythingChanged = false;

	if (!Metadata)
	{
		return false;
	}

	// Ensure the metadata curve arrays match the current spline point count
	Metadata->Fixup(GetNumberOfSplinePoints(), this);

	// Propagate any changed defaults to points that are still at their old default
	for (int32 Point = 0; Point < GetNumberOfSplinePoints(); ++Point)
	{
		bAnythingChanged |= Metadata->PropagateDefaultValue(
			Point, PreviousBarrierSplineDefaults, BarrierSplineDefaults);
	}

	if (bAnythingChanged)
	{
		UpdateSpline();
	}

	// Snapshot the current defaults so the next call can detect incremental changes
	PreviousBarrierSplineDefaults = BarrierSplineDefaults;

	return bAnythingChanged;
}

#endif // WITH_EDITOR
