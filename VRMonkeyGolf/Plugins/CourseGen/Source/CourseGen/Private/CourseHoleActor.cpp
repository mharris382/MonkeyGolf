#include "CourseHoleActor.h"

#include "CourseGenTags.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "PCGComponent.h"
#include "GameFramework/Actor.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Engine/StaticMesh.h"

#if WITH_EDITOR
#include "Components/DynamicMeshComponent.h"
#include "UDynamicMesh.h"
#include "Data/PCGDynamicMeshData.h"
#include "GeometryScript/MeshAssetFunctions.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#endif

#define LOCTEXT_NAMESPACE "CourseGen"



// =============================================================================
// #region Constructor
// =============================================================================

ACourseHoleActor::ACourseHoleActor()
{
	PrimaryActorTick.bCanEverTick = false;

	GreenMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(
		TEXT("GreenMesh"));
	SetRootComponent(GreenMeshComponent);

	PatchISM = CreateDefaultSubobject<UInstancedStaticMeshComponent>(
		TEXT("PatchISM"));
	PatchISM->SetupAttachment(GreenMeshComponent);

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(
		TEXT("PCGComponent"));

#if WITH_EDITORONLY_DATA
	PreviewMeshComponent = CreateDefaultSubobject<UDynamicMeshComponent>(
		TEXT("PreviewMesh"));
	PreviewMeshComponent->SetupAttachment(GreenMeshComponent);
	PreviewMeshComponent->SetCastShadow(false);
	PreviewMeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PreviewMeshComponent->SetVisibility(false);
#endif
}

// #endregion

// =============================================================================
// #region BeginPlay
// =============================================================================

void ACourseHoleActor::BeginPlay()
{
	Super::BeginPlay();

#if WITH_EDITORONLY_DATA
	if (PreviewMeshComponent)
	{
		PreviewMeshComponent->SetVisibility(false);
	}
#endif

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

	// Scale to zero to hide — keeps ISM indices stable
	if (PatchISM && PatchISM->GetInstanceCount() > Index)
	{
		FTransform HiddenTransform = Slot->SlotTransform;
		HiddenTransform.SetScale3D(FVector::ZeroVector);
		PatchISM->UpdateInstanceTransform(Index, HiddenTransform,
			/*bWorldSpace*/true, /*bMarkRenderStateDirty*/true);
	}

	ActiveCandidateIndex = Index;

	UE_LOG(LogTemp, Log,
		TEXT("CourseHoleActor [Hole %d]: Activated candidate %d."),
		HoleIndex, Index);

	OnCandidateActivated(Index, Slot->SlotTransform);
}

void ACourseHoleActor::ResetActivation()
{
	if (ActiveCandidateIndex == INDEX_NONE)
	{
		return;
	}

	const FProceduralSlot* Slot = SlotCollection.FindCandidate(ActiveCandidateIndex);
	if (Slot && PatchISM && PatchISM->GetInstanceCount() > ActiveCandidateIndex)
	{
		PatchISM->UpdateInstanceTransform(ActiveCandidateIndex, Slot->SlotTransform,
			/*bWorldSpace*/true, /*bMarkRenderStateDirty*/true);
	}

	ActiveCandidateIndex = INDEX_NONE;
}

// #endregion

// =============================================================================
// #region Private Runtime
// =============================================================================

void ACourseHoleActor::InitialiseFromDataAsset()
{
	if (!HoleData)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: No HoleData assigned."), HoleIndex);
		OnContractValidationFailed(CourseGenTags::SurfaceGreen);
		return;
	}

	if (!HoleData->IsValid())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: HoleData is invalid or incomplete."), HoleIndex);
		OnContractValidationFailed(CourseGenTags::HoleCandidate);
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

	BuildPatchISM();
}

void ACourseHoleActor::BuildPatchISM()
{
	if (SlotCollection.CupCandidates.IsEmpty())
	{
		return;
	}

	UStaticMesh* PatchMesh = nullptr;
	for (const FProceduralSlot& Slot : SlotCollection.CupCandidates)
	{
		if (!Slot.PatchMesh.IsNull())
		{
			PatchMesh = Slot.PatchMesh.LoadSynchronous();
			break;
		}
	}

	if (!PatchMesh)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: No PatchMesh found on any candidate slot."),
			HoleIndex);
		return;
	}

	PatchISM->SetStaticMesh(PatchMesh);
	PatchISM->ClearInstances();

	for (const FProceduralSlot& Slot : SlotCollection.CupCandidates)
	{
		PatchISM->AddInstance(Slot.SlotTransform, /*bWorldSpace*/true);
	}
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
// #region Editor
// =============================================================================

#if WITH_EDITOR

void ACourseHoleActor::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	if (bAutoUpdatePreview)
	{
		BindPCGDelegate();
	}
}

void ACourseHoleActor::BeginDestroy()
{
	UnbindPCGDelegate();
	Super::BeginDestroy();
}

void ACourseHoleActor::PostEditChangeProperty(
	FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName PropertyName = PropertyChangedEvent.GetPropertyName();

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ACourseHoleActor, bAutoUpdatePreview))
	{
		bAutoUpdatePreview ? BindPCGDelegate() : UnbindPCGDelegate();
	}
}

UCourseHoleDataAsset* ACourseHoleActor::GetOrCreateHoleDataAsset()
{
	if (HoleData)
	{
		return HoleData;
	}

	const FString AssetPath = FString::Printf(TEXT("%s/DA_Hole_%02d"),
		*BakeOutputPath.Path, HoleIndex);

	UPackage* Package = CreatePackage(*AssetPath);
	UCourseHoleDataAsset* NewAsset = NewObject<UCourseHoleDataAsset>(
		Package,
		UCourseHoleDataAsset::StaticClass(),
		*FPackageName::GetShortName(AssetPath),
		RF_Public | RF_Standalone);

	FAssetRegistryModule::AssetCreated(NewAsset);
	HoleData = NewAsset;
	MarkPackageDirty();

	return HoleData;
}

void ACourseHoleActor::UpdatePreview()
{
	RebuildPreviewFromPCG();
}

void ACourseHoleActor::ClearPreview()
{
	if (PreviewMeshComponent && PreviewMeshComponent->GetDynamicMesh())
	{
		PreviewMeshComponent->GetDynamicMesh()->Reset();
		PreviewMeshComponent->SetVisibility(false);
	}
}

void ACourseHoleActor::BakeToStaticMesh()
{
	if (BakedMeshName.IsEmpty() || BakeOutputPath.Path.IsEmpty())
	{
		UE_LOG(LogTemp, Error,
			TEXT("CourseHoleActor [Hole %d]: BakedMeshName or BakeOutputPath not set."),
			HoleIndex);
		return;
	}

	if (!PreviewMeshComponent || !PreviewMeshComponent->GetDynamicMesh())
	{
		UE_LOG(LogTemp, Error,
			TEXT("CourseHoleActor [Hole %d]: No preview mesh. Run UpdatePreview first."),
			HoleIndex);
		return;
	}

	// ── Create or find target static mesh asset ───────────────────────────────
	// CopyMeshToStaticMesh requires an existing UStaticMesh* — it cannot create one

	const FString MeshAssetPath = FString::Printf(TEXT("%s/%s"),
		*BakeOutputPath.Path, *BakedMeshName);

	UStaticMesh* BakedMesh = CreateOrFindStaticMeshAsset(MeshAssetPath);
	if (!BakedMesh)
	{
		UE_LOG(LogTemp, Error,
			TEXT("CourseHoleActor [Hole %d]: Failed to create or find static mesh asset at %s."),
			HoleIndex, *MeshAssetPath);
		return;
	}

	// ── Bake options ──────────────────────────────────────────────────────────

	FGeometryScriptCopyMeshToAssetOptions BakeOptions;
	BakeOptions.bEnableRecomputeNormals = true;
	BakeOptions.bEnableRecomputeTangents = true;
	BakeOptions.GenerateLightmapUVs =
		EGeometryScriptGenerateLightmapUVOptions::GenerateLightmapUVs;
	BakeOptions.bReplaceMaterials = true; // write material slots from mesh

	FGeometryScriptMeshWriteLOD TargetLOD; // defaults to LOD 0, SourceModel
	EGeometryScriptOutcomePins Outcome = EGeometryScriptOutcomePins::Failure;

	UGeometryScriptLibrary_StaticMeshFunctions::CopyMeshToStaticMesh(
		PreviewMeshComponent->GetDynamicMesh(),
		BakedMesh,
		BakeOptions,
		TargetLOD,
		Outcome,
		/*bUseSectionMaterials*/true,
		/*Debug*/nullptr);

	if (Outcome == EGeometryScriptOutcomePins::Failure)
	{
		UE_LOG(LogTemp, Error,
			TEXT("CourseHoleActor [Hole %d]: CopyMeshToStaticMesh failed."), HoleIndex);
		return;
	}

	FCourseHoleSlotCollection BakedCollection;
	ExtractSlotCollection(BakedCollection);
	CommitBakeToDataAsset(BakedMesh, BakedCollection);

	UE_LOG(LogTemp, Log,
		TEXT("CourseHoleActor [Hole %d]: Bake complete -> %s."),
		HoleIndex, *MeshAssetPath);
}

UStaticMesh* ACourseHoleActor::CreateOrFindStaticMeshAsset(const FString& AssetPath)
{
	// Check if asset already exists at this path
	if (UStaticMesh* Existing = LoadObject<UStaticMesh>(nullptr, *AssetPath))
	{
		return Existing;
	}

	// Create a new empty static mesh asset
	UPackage* Package = CreatePackage(*AssetPath);
	if (!Package)
	{
		return nullptr;
	}

	UStaticMesh* NewMesh = NewObject<UStaticMesh>(
		Package,
		UStaticMesh::StaticClass(),
		*FPackageName::GetShortName(AssetPath),
		RF_Public | RF_Standalone);

	if (!NewMesh)
	{
		return nullptr;
	}

	// Required before CopyMeshToStaticMesh can write LOD data
	NewMesh->InitResources();

	FAssetRegistryModule::AssetCreated(NewMesh);
	NewMesh->MarkPackageDirty();

	return NewMesh;
}

void ACourseHoleActor::RebuildPreviewFromPCG()
{
	if (!PCGComponent)
	{
		return;
	}

	TArray<FPCGTaggedData> GreenData =
		PCGComponent->GetGeneratedGraphOutput().GetInputsByPin(CourseGenTags::SurfaceGreen);

	if (GreenData.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: CourseGen.Surface.Green not in PCG output."),
			HoleIndex);
		return;
	}

	const UPCGDynamicMeshData* MeshData =
		Cast<UPCGDynamicMeshData>(GreenData[0].Data);

	if (!MeshData || !MeshData->GetDynamicMesh())
	{
		return;
	}

	PreviewMeshComponent->GetDynamicMesh()->SetMesh(
		MeshData->GetDynamicMesh()->GetMeshRef());
	PreviewMeshComponent->SetVisibility(true);

	UE_LOG(LogTemp, Log,
		TEXT("CourseHoleActor [Hole %d]: Preview updated."), HoleIndex);
}

bool ACourseHoleActor::ExtractSlotCollection(
	FCourseHoleSlotCollection& OutCollection) const
{
	if (!PCGComponent)
	{
		return false;
	}

	const FPCGDataCollection& Output = PCGComponent->GetGeneratedGraphOutput();

	// ── Tee ───────────────────────────────────────────────────────────────────

	TArray<FPCGTaggedData> TeeData = Output.GetInputsByPin(CourseGenTags::HoleTee);
	if (TeeData.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: CourseGen.Hole.Tee missing."), HoleIndex);
		return false;
	}

	const UPCGPointData* TeePoints = Cast<UPCGPointData>(TeeData[0].Data);
	if (TeePoints && !TeePoints->GetPoints().IsEmpty())
	{
		OutCollection.TeeTransform = TeePoints->GetPoints()[0].Transform;
	}

	// ── Candidates ────────────────────────────────────────────────────────────

	TArray<FPCGTaggedData> CandidateData =
		Output.GetInputsByPin(CourseGenTags::HoleCandidate);

	if (CandidateData.IsEmpty())
	{
		UE_LOG(LogTemp, Warning,
			TEXT("CourseHoleActor [Hole %d]: CourseGen.Hole.Candidate missing."), HoleIndex);
		return false;
	}

	const UPCGPointData* CandidatePoints =
		Cast<UPCGPointData>(CandidateData[0].Data);

	if (!CandidatePoints)
	{
		return false;
	}

	const FPCGMetadataAttribute<int32>* IndexAttr =
		CandidatePoints->Metadata
		? CandidatePoints->Metadata->GetConstTypedAttribute<int32>(
			CourseGenTags::Attr_CandidateIndex)
		: nullptr;

	const TArray<FPCGPoint>& Points = CandidatePoints->GetPoints();
	OutCollection.CupCandidates.Reserve(Points.Num());

	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const FPCGPoint& Point = Points[i];

		FProceduralSlot Slot;
		Slot.SlotTransform = Point.Transform;
		Slot.SlotIndex = IndexAttr
			? IndexAttr->GetValueFromItemKey(Point.MetadataEntry)
			: i;

		OutCollection.CupCandidates.Add(Slot);
	}

	return OutCollection.IsValid();
}

void ACourseHoleActor::CommitBakeToDataAsset(
	UStaticMesh* BakedMesh,
	const FCourseHoleSlotCollection& Collection)
{
	UCourseHoleDataAsset* DataAsset = GetOrCreateHoleDataAsset();
	if (!DataAsset)
	{
		return;
	}

	DataAsset->HoleIndex = HoleIndex;
	DataAsset->SlotCollection = Collection;
	DataAsset->BakedGreenMesh = BakedMesh;

	DataAsset->MarkPackageDirty();
	MarkPackageDirty();

	UE_LOG(LogTemp, Log,
		TEXT("CourseHoleActor [Hole %d]: Data asset committed (%d candidates)."),
		HoleIndex, Collection.CupCandidates.Num());
}

void ACourseHoleActor::BindPCGDelegate()
{
	if (PCGComponent)
	{
		PCGComponent->OnPCGGraphGeneratedDelegate.AddUObject(
			this, &ACourseHoleActor::OnPCGGenerationFinished);
	}
}

void ACourseHoleActor::UnbindPCGDelegate()
{
	if (PCGComponent)
	{
		PCGComponent->OnPCGGraphGeneratedDelegate.RemoveAll(this);
	}
}

void ACourseHoleActor::OnPCGGenerationFinished(UPCGComponent* InPCGComponent)
{
	if (bAutoUpdatePreview)
	{
		RebuildPreviewFromPCG();
	}
}

#endif // WITH_EDITOR

// #endregion

#undef LOCTEXT_NAMESPACE