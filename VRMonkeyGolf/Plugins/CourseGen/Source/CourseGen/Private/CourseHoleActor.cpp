#include "CourseHoleActor.h"

#include "CourseGenTags.h"
#include "GameFramework/Actor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CourseSplineComponent.h"
#include "PCGComponent.h"

#define LOCTEXT_NAMESPACE "CourseGen"

// =============================================================================
// #region Constructor
// =============================================================================

ACourseHoleActor::ACourseHoleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GreenMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GreenMesh"));
	SetRootComponent(GreenMeshComponent);

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));

	HoleBoundsComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("HoleBounds"));
	HoleBoundsComponent->SetupAttachment(GreenMeshComponent);
	HoleBoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HoleBoundsComponent->SetLineThickness(2.0f);

	// Visible in editor for authoring feedback, hidden at runtime
	HoleBoundsComponent->SetHiddenInGame(true);
}


void ACourseHoleActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	RecomputeHoleBounds();
}
// #endregion

// =============================================================================
// #region BeginPlay
// =============================================================================

void ACourseHoleActor::BeginPlay()
{
	Super::BeginPlay();
	InitialiseFromDataAsset();

	if (SelectionMode != ECandidateSelectionMode::External)
	{
		const int32 ChosenIndex = ResolveAutoIndex();
		if (ChosenIndex != INDEX_NONE)
		{
			ActivateCupCandidate(ChosenIndex);
		}
	}
}

// #endregion
// =============================================================================
// #region Bounds
// =============================================================================

void ACourseHoleActor::RecomputeHoleBounds()
{
	// Union world-space bounds of every child USplineComponent.
	// Works for both default-subobject splines and BP-added splines.

	FBox UnionBox(EForceInit::ForceInit);

	// Query specifically for CourseSplineComponents — ignores any unrelated
	// USplineComponents that might exist on child actors or other systems.
	TArray<UCourseSplineComponent*> Splines;
	GetComponents<UCourseSplineComponent>(Splines);

	for (const USplineComponent* Spline : Splines)
	{
		if (!Spline) { continue; }

		// Sample spline points to build a tight world-space box.
		// SplinePoints are in local space — transform each to world.
		const int32 NumPoints = Spline->GetNumberOfSplinePoints();
		for (int32 i = 0; i < NumPoints; ++i)
		{
			const FVector WorldPos =
				Spline->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::World);
			UnionBox += WorldPos;
		}
	}

	if (!UnionBox.IsValid)
	{
		// No splines yet — set a small default so the box component isn't degenerate
		UnionBox = FBox(GetActorLocation() - FVector(100.f),
			GetActorLocation() + FVector(100.f));
	}

	// Convert world-space AABB to component-local extent + offset.
	// BoxComponent extent is half-size in local space.
	const FVector WorldCenter = UnionBox.GetCenter();
	const FVector HalfExtent = UnionBox.GetExtent() + FVector(BoundsPadding);

	// Position the box component at the AABB center in world space,
	// then set its local extent to match (spline bounds + padding).
	HoleBoundsComponent->SetWorldLocation(WorldCenter);
	HoleBoundsComponent->SetBoxExtent(HalfExtent, /*bUpdateOverlaps*/false);
}

FBox ACourseHoleActor::GetHoleBounds_Implementation() const
{
	// HoleBoundsComponent already includes BoundsPadding from RecomputeHoleBounds.
	// Subclasses can override to return a further expanded or custom box.
	const FVector Center = HoleBoundsComponent->GetComponentLocation();
	const FVector Extent = HoleBoundsComponent->GetScaledBoxExtent();
	return FBox(Center - Extent, Center + Extent);
}

// #endregion
// =============================================================================
// #region Runtime Interface
// =============================================================================


TArray<FTransform> ACourseHoleActor::GetCupCandidates() const
{
	TArray<FTransform> Transforms;
	Transforms.Reserve(SlotCollection.CupCandidates.Num());
	for (const FProceduralSlot& Slot : SlotCollection.CupCandidates)
	{
		Transforms.Add(Slot.SlotTransform);
	}
	return Transforms;
}

FTransform ACourseHoleActor::GetTeeTransform() const
{
	return SlotCollection.TeeTransform;
}

void ACourseHoleActor::ActivateCupCandidate(int32 Index)
{
	if (ActiveCandidateIndex != INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: Already activated. Call ResetActivation first."),
			HoleIndex);
		return;
	}

	const FProceduralSlot* Slot = SlotCollection.FindCandidate(Index);
	if (!Slot)
	{
		UE_LOG(LogTemp, Error,
			TEXT("CourseHoleActor [Hole %d]: Candidate index %d not found."),
			HoleIndex, Index);
		return;
	}

	ActiveCandidateIndex = Index;

	UE_LOG(LogTemp, Log,
		TEXT("CourseHoleActor [Hole %d]: Candidate %d recorded at %s."),
		HoleIndex, Index, *Slot->SlotTransform.GetLocation().ToString());

	OnCandidateActivated(Index, Slot->SlotTransform);
}

void ACourseHoleActor::ResetActivation()
{
	ActiveCandidateIndex = INDEX_NONE;
}

int32 ACourseHoleActor::ResolveAutoIndex() const
{
	if (SlotCollection.CupCandidates.IsEmpty())
	{
		return INDEX_NONE;
	}

	switch (SelectionMode)
	{
	case ECandidateSelectionMode::First:
		return SlotCollection.CupCandidates[0].SlotIndex;

	case ECandidateSelectionMode::Random:
	{
		const int32 Idx = FMath::RandRange(0, SlotCollection.CupCandidates.Num() - 1);
		return SlotCollection.CupCandidates[Idx].SlotIndex;
	}

	case ECandidateSelectionMode::Seeded:
	{
		FRandomStream Stream(ExplicitSeed);
		const int32 Idx = Stream.RandRange(0, SlotCollection.CupCandidates.Num() - 1);
		return SlotCollection.CupCandidates[Idx].SlotIndex;
	}

	case ECandidateSelectionMode::External:
	default:
		return INDEX_NONE;
	}
}

// #endregion

// =============================================================================
// #region Private
// =============================================================================

void ACourseHoleActor::InitialiseFromDataAsset()
{
	if (!HoleData || !HoleData->IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: HoleData missing or invalid."), HoleIndex);
		OnHoleDataMissing();
		return;
	}

	SlotCollection = HoleData->SlotCollection;

	if (!HoleData->BakedGreenMesh.IsNull())
	{
		if (UStaticMesh* GreenMesh = HoleData->BakedGreenMesh.LoadSynchronous())
		{
			GreenMeshComponent->SetStaticMesh(GreenMesh);
		}
	}
}

// #endregion

#undef LOCTEXT_NAMESPACE