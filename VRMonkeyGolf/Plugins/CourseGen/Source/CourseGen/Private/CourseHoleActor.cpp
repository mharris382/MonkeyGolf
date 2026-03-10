#include "CourseHoleActor.h"

#include "CourseGenTags.h"
#include "Components/StaticMeshComponent.h"
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