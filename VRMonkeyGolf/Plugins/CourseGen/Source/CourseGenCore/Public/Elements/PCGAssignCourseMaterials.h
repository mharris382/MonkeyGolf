#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGContext.h"
#include "Elements/PCGDynamicMeshBaseElement.h"
#include "Data/CourseGenDataAsset.h"

#include "PCGAssignCourseMaterials.generated.h"

/**
 * PCGAssignCourseMaterials
 *
 * Assigns material slots to a dynamic mesh based on per-triangle face normals.
 *
 * Faces within SlopeThresholdDegrees of world up (0,0,1) → slot 0 (FlatGreenMaterial)
 * All other faces                                        → slot 1 (SlopedGreenMaterial)
 *
 * Material definitions sourced from a UCourseGenDataAsset assigned in settings.
 * Run after boolean cuts, before bake — output mesh has correct material IDs
 * for both visual rendering and physics material assignment.
 *
 * Lives in CourseGenCore. Uses CourseGen.Surface.Green pin convention.
 */

 // ── Context ───────────────────────────────────────────────────────────────────

struct FPCGAssignCourseMaterialsContext : public FPCGContext
{
	// No async asset loading. Exists to match BaseElement pattern.
};

// ── Settings ──────────────────────────────────────────────────────────────────

UCLASS(BlueprintType, ClassGroup = (Procedural))
class COURSEGENCORE_API UPCGAssignCourseMaterialsSettings : public UPCGDynamicMeshBaseSettings
{
	GENERATED_BODY()

public:

#if WITH_EDITOR
	virtual FName GetDefaultNodeName() const override
	{
		return FName(TEXT("AssignCourseMaterials"));
	}

	virtual FText GetDefaultNodeTitle() const override
	{
		return NSLOCTEXT("CourseGenCore", "AssignCourseMaterials_Title", "Assign Course Materials");
	}

	virtual FText GetNodeTooltipText() const override
	{
		return NSLOCTEXT("CourseGenCore", "AssignCourseMaterials_Tooltip",
			"Assigns flat/sloped material slots by face normal.\n"
			"Faces within SlopeThresholdDegrees of world up → slot 0 (flat).\n"
			"All other faces → slot 1 (sloped).\n"
			"Run after boolean cuts, before bake.");
	}
#endif

	virtual EPCGDataType GetCurrentPinTypes(const UPCGPin* InPin) const override
	{
		return EPCGDataType::DynamicMesh;
	}

	virtual TArray<FPCGPinProperties> InputPinProperties() const override;
	virtual TArray<FPCGPinProperties> OutputPinProperties() const override;

protected:
	virtual FPCGElementPtr CreateElement() const override;

public:

	/**
	 * Data asset providing FlatGreenMaterial and SlopedGreenMaterial.
	 * Named CourseGenData to match ACourseHoleActor::CourseGenData —
	 * PCG overrides match by UPROPERTY name, so GetActorProperty feeds this directly.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Materials", meta = (PCG_Overridable))
	TSoftObjectPtr<UCourseGenDataAsset> CourseGenData;

	/**
	 * Faces within this angle of world up are classified as flat (slot 0).
	 * Default is intentionally tiny — only near-perfectly horizontal faces get flat material.
	 * Increase if ramps are being misclassified as sloped.
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Materials",
		meta = (PCG_Overridable, ClampMin = "0.0", ClampMax = "89.0", UIMin = "0.0", UIMax = "45.0"))
	float SlopeThresholdDegrees = 0.5f;
};

// ── Element ───────────────────────────────────────────────────────────────────

class COURSEGENCORE_API FPCGAssignCourseMaterialsElement : public IPCGDynamicMeshBaseElement
{
public:
	virtual bool CanExecuteOnlyOnMainThread(FPCGContext* Context) const override;

protected:
	virtual FPCGContext* CreateContext() override;
	virtual bool PrepareDataInternal(FPCGContext* InContext) const override;
	virtual bool ExecuteInternal(FPCGContext* InContext) const override;
};