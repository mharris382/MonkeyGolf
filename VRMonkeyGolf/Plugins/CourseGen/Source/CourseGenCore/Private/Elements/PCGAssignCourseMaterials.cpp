#include "Elements/PCGAssignCourseMaterials.h"

#include "PCGPin.h"
#include "Data/PCGDynamicMeshData.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "DynamicMesh/DynamicMeshAttributeSet.h"
#include "UDynamicMesh.h"

#include "CourseGenTags.h"

#define LOCTEXT_NAMESPACE "PCGAssignCourseMaterials"

using namespace UE::Geometry;

// ── Settings ──────────────────────────────────────────────────────────────────

TArray<FPCGPinProperties> UPCGAssignCourseMaterialsSettings::InputPinProperties() const
{
	TArray<FPCGPinProperties> Props;
	Props.Emplace_GetRef(CourseGenTags::SurfaceGreen, EPCGDataType::DynamicMesh, false, false)
		.SetRequiredPin();
	return Props;
}

TArray<FPCGPinProperties> UPCGAssignCourseMaterialsSettings::OutputPinProperties() const
{
	TArray<FPCGPinProperties> Props;
	Props.Emplace(CourseGenTags::SurfaceGreen, EPCGDataType::DynamicMesh, false, false);
	return Props;
}

FPCGElementPtr UPCGAssignCourseMaterialsSettings::CreateElement() const
{
	return MakeShared<FPCGAssignCourseMaterialsElement>();
}

// ── Element ───────────────────────────────────────────────────────────────────

bool FPCGAssignCourseMaterialsElement::CanExecuteOnlyOnMainThread(FPCGContext* Context) const
{
	// No async loading — safe to run off game thread
	return false;
}

FPCGContext* FPCGAssignCourseMaterialsElement::CreateContext()
{
	return new FPCGAssignCourseMaterialsContext();
}

bool FPCGAssignCourseMaterialsElement::PrepareDataInternal(FPCGContext* InContext) const
{
	// No asset loading required
	return true;
}

bool FPCGAssignCourseMaterialsElement::ExecuteInternal(FPCGContext* InContext) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGAssignCourseMaterialsElement::ExecuteInternal);

	FPCGAssignCourseMaterialsContext* Context =
		static_cast<FPCGAssignCourseMaterialsContext*>(InContext);
	check(Context);

	const UPCGAssignCourseMaterialsSettings* Settings =
		InContext->GetInputSettings<UPCGAssignCourseMaterialsSettings>();
	check(Settings);

	// ── 1. Validate data asset ────────────────────────────────────────────────

	if (Settings->CourseGenData.IsNull())
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("MissingDataAsset",
			"AssignCourseMaterials: No CourseGenData assigned."));
		return true;
	}

	UCourseGenDataAsset* DataAsset = Settings->CourseGenData.LoadSynchronous();
	if (!DataAsset)
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("FailedLoad",
			"AssignCourseMaterials: CourseGenData failed to load."));
		return true;
	}

	if (DataAsset->FlatGreenMaterial.IsNull() || DataAsset->SlopedGreenMaterial.IsNull())
	{
		PCGE_LOG(Warning, GraphAndLog, LOCTEXT("MissingMaterials",
			"AssignCourseMaterials: FlatGreenMaterial or SlopedGreenMaterial not set — passing mesh through unchanged."));

		InContext->OutputData.TaggedData.Append(
			InContext->InputData.GetInputsByPin(CourseGenTags::SurfaceGreen));
		return true;
	}

	// ── 2. Get inputs ─────────────────────────────────────────────────────────

	TArray<FPCGTaggedData> Inputs =
		InContext->InputData.GetInputsByPin(CourseGenTags::SurfaceGreen);

	if (Inputs.IsEmpty())
	{
		PCGE_LOG(Error, GraphAndLog, LOCTEXT("NoInput",
			"AssignCourseMaterials: No CourseGen.Surface.Green input found."));
		return true;
	}

	// ── 3. Precompute dot threshold ───────────────────────────────────────────
	//
	// Faces where dot(faceNormal, worldUp) >= DotThreshold are flat (slot 0).
	// All others are sloped (slot 1).
	// Small default threshold means only near-perfectly horizontal faces are flat.

	const double DotThreshold =
		FMath::Cos(FMath::DegreesToRadians(Settings->SlopeThresholdDegrees));

	const FVector3d WorldUp(0.0, 0.0, 1.0);

	// ── 4. Process each mesh ──────────────────────────────────────────────────

	for (FPCGTaggedData& InputTaggedData : Inputs)
	{
		const UPCGDynamicMeshData* InMeshData =
			Cast<const UPCGDynamicMeshData>(InputTaggedData.Data);

		if (!InMeshData)
		{
			PCGE_LOG(Warning, GraphAndLog, LOCTEXT("BadInput",
				"AssignCourseMaterials: Input is not DynamicMesh data, skipping."));
			continue;
		}

		// CopyOrSteal — zero-copy if sole consumer, deep copy otherwise
		UPCGDynamicMeshData* OutMeshData = CopyOrSteal(InputTaggedData, InContext);
		if (!OutMeshData)
		{
			PCGE_LOG(Warning, GraphAndLog, LOCTEXT("CopyFailed",
				"AssignCourseMaterials: CopyOrSteal failed, skipping."));
			continue;
		}

		FDynamicMesh3* RawMesh = OutMeshData->GetMutableDynamicMesh()->GetMeshPtr();
		if (!RawMesh)
		{
			PCGE_LOG(Warning, GraphAndLog, LOCTEXT("NullMesh",
				"AssignCourseMaterials: Raw mesh pointer is null, skipping."));
			continue;
		}

		// ── 5. Ensure material ID attribute exists ────────────────────────────

		if (!RawMesh->HasAttributes())
		{
			RawMesh->EnableAttributes();
		}

		if (!RawMesh->Attributes()->HasMaterialID())
		{
			RawMesh->Attributes()->EnableMaterialID();
		}

		FDynamicMeshMaterialAttribute* MaterialIDs =
			RawMesh->Attributes()->GetMaterialID();

		// ── 6. Classify each triangle by face normal ──────────────────────────

		int32 FlatCount   = 0;
		int32 SlopedCount = 0;

		for (int32 TriID : RawMesh->TriangleIndicesItr())
		{
			const FIndex3i Tri = RawMesh->GetTriangle(TriID);

			const FVector3d V0 = RawMesh->GetVertex(Tri.A);
			const FVector3d V1 = RawMesh->GetVertex(Tri.B);
			const FVector3d V2 = RawMesh->GetVertex(Tri.C);

			FVector3d Normal = (V1 - V0).Cross(V2 - V0);

			if (!Normal.Normalize())
			{
				// Degenerate triangle — classify as sloped to be safe
				MaterialIDs->SetValue(TriID, 1);
				++SlopedCount;
				continue;
			}

			const double Dot = Normal.Dot(WorldUp);

			if (Dot >= DotThreshold)
			{
				MaterialIDs->SetValue(TriID, 0); // flat
				++FlatCount;
			}
			else
			{
				MaterialIDs->SetValue(TriID, 1); // sloped
				++SlopedCount;
			}
		}

		PCGE_LOG(Verbose, GraphAndLog,
			FText::Format(
				LOCTEXT("Result", "AssignCourseMaterials: {0} flat, {1} sloped. Threshold: {2} deg."),
				FText::AsNumber(FlatCount),
				FText::AsNumber(SlopedCount),
				FText::AsNumber(Settings->SlopeThresholdDegrees)));

		// ── 7. Bind material references onto the mesh data ───────────────────
		// UPCGDynamicMeshData::Materials is what SaveCourseMeshToAsset reads via
		// GetMaterials() to populate AssetOptions.NewMaterials before calling
		// CopyMeshToStaticMesh. Without this call that array is empty and no
		// materials are written to the static mesh asset.
		//
		// Slot order must match the material IDs written above:
		//   [0] = FlatGreenMaterial   (faces where dot >= DotThreshold)
		//   [1] = SlopedGreenMaterial (all other faces)
		{
			UMaterialInterface* FlatMat   = DataAsset->FlatGreenMaterial.LoadSynchronous();
			UMaterialInterface* SlopedMat = DataAsset->SlopedGreenMaterial.LoadSynchronous();

			OutMeshData->SetMaterials({ FlatMat, SlopedMat });
		}

		// ── 8. Output ─────────────────────────────────────────────────────────

		FPCGTaggedData& Output =
			InContext->OutputData.TaggedData.Emplace_GetRef(InputTaggedData);
		Output.Data = OutMeshData;
	}

	return true;
}

#undef LOCTEXT_NAMESPACE