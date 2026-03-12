#include "CourseActor.h"

#include "CourseHoleActor.h"
#include "PCGComponent.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"

// =============================================================================
// #region Constructor
// =============================================================================

ACourseActor::ACourseActor()
{
	PrimaryActorTick.bCanEverTick = false;
	HoleBoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("HoleBounds"));
	SetRootComponent(HoleBoundsComponent);
	HoleBoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoleBoundsComponent->SetLineThickness(2.0f);

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));

}

void ACourseActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (bGlobalSpawner)
	{
		CollectHoleActorsFromLevel();
	}

	if (HoleActors.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("CourseActor: No holes found. Please add ACourseHoleActors to the level."));
		return;
	}

	RecomputeHoleBounds();
}

// #endregion
// =============================================================================
// #region Bounds
// =============================================================================

void ACourseActor::RecomputeHoleBounds()
{
	// Union world-space bounds of every child USplineComponent.
	// Works for both default-subobject splines and BP-added splines.

	FBox UnionBox(EForceInit::ForceInit);

	if (!UnionBox.IsValid)
	{
		// No splines yet — set a small default so the box component isn't degenerate
		UnionBox = FBox(GetActorLocation() - FVector(100.f),
			GetActorLocation() + FVector(100.f));
	}

	for (ACourseHoleActor* Hole : GetHoleActors())
	{
		if (!Hole) { continue; }
		const FBox HoleBox = Hole->GetHoleBounds();
		if (HoleBox.IsValid)
		{
			UnionBox += HoleBox;
		}
	}

	UnionBox.Max.Z = FMath::Max(UnionBox.Max.Z, GetCourseCeilingZWS());
	UnionBox.Min.Z = FMath::Min(UnionBox.Min.Z, GetCourseFloorZWS());

	// Convert world-space AABB to component-local extent + offset.
	// BoxComponent extent is half-size in local space.
	const FVector WorldCenter = UnionBox.GetCenter();
	FVector HalfExtent = UnionBox.GetExtent() + FVector(BoundsPadding);
	const float floorz = GetCourseFloorZWS();
	

	// Position the box component at the AABB center in world space,
	// then set its local extent to match (spline bounds + padding).
	HoleBoundsComponent->SetWorldLocation(WorldCenter);
	HoleBoundsComponent->SetBoxExtent(HalfExtent, /*bUpdateOverlaps*/false);
}

FBox ACourseActor::GetHoleBounds_Implementation() const
{
	// HoleBoundsComponent already includes BoundsPadding from RecomputeHoleBounds.
	// Subclasses can override to return a further expanded or custom box.
	const FVector Center = HoleBoundsComponent->GetComponentLocation();
	const FVector Extent = HoleBoundsComponent->GetScaledBoxExtent();
	return FBox(Center - Extent, Center + Extent);
}

// #endregion

// =============================================================================
// #region AActor Interface
// =============================================================================

void ACourseActor::BeginPlay()
{
	Super::BeginPlay();

	if (bGlobalSpawner)
	{
		CollectHoleActorsFromLevel();
	}
}

// #endregion

// =============================================================================
// #region Interface
// =============================================================================

const TArray<ACourseHoleActor*> ACourseActor::GetHoleActors() const
{
	TArray<ACourseHoleActor*> Result;
	Result.Reserve(HoleActors.Num());
	for (const TObjectPtr<ACourseHoleActor>& Hole : HoleActors)
	{
		if (Hole) { Result.Add(Hole); }
	}
	return Result;
}

// #endregion

// =============================================================================
// #region Private
// =============================================================================

void ACourseActor::CollectHoleActorsFromLevel()
{
	HoleActors.Reset();

	for (TActorIterator<ACourseHoleActor> It(GetWorld()); It; ++It)
	{
		HoleActors.Add(*It);
	}

}

// #endregion