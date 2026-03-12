#pragma once

#include "CoreMinimal.h"
#include "Components/ShapeComponent.h"

#include "CourseBrushComponent.generated.h"

/**
 * Which PCG graph stage this brush participates in.
 */
UENUM(BlueprintType)
enum class ECourseBrushTarget : uint8
{
	/** Polygon2D boolean op applied to the playable green surface. */
	Green,

	/** Shapes the terrain boundary/skirt zone around the green. */
	Boundary,

	/** Reserved for future terrain conformance stage. */
	Terrain,
};

/**
 * Boolean operation applied by the PCG graph.
 * Maps directly to the PCG Polygon2D Union / Difference node.
 */
UENUM(BlueprintType)
enum class ECourseBrushMode : uint8
{
	/** Expands or merges into the target shape. */
	Union,

	/** Cuts / subtracts from the target shape. */
	Difference,
};

/**
 * UCourseBrushComponent
 *
 * UBrushComponent subclass that carries PCG pipeline metadata.
 * Shape is authored via the standard UE geometry brush tools.
 * Data is read by the CourseHole PCG graph to perform Polygon2D boolean ops.
 *
 * PCG access pattern:
 *   GetActorsByClass(ACourseBrushActor) within hole bounds
 *   → read UCourseBrushComponent properties
 *   → sort by Priority
 *   → apply Mode as Polygon2D Union / Difference in order
 *
 * Does nothing at runtime — pure authoring data carrier.
 */
UCLASS(ClassGroup = (CourseGen), meta = (BlueprintSpawnableComponent),
	HideCategories = (Collision, Navigation, AssetUserData, Mobile))
	class COURSEGEN_API UCourseBrushComponent : public UShapeComponent
{
	GENERATED_BODY()

public:

	UCourseBrushComponent();

	// #region Brush Metadata

	/** Which PCG graph stage consumes this brush. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseBrush")
	ECourseBrushTarget Target = ECourseBrushTarget::Green;

	/** Boolean operation the PCG graph applies with this brush's shape. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseBrush")
	ECourseBrushMode Mode = ECourseBrushMode::Union;

	/**
	 * Execution order within brushes of the same Target.
	 * Lower value = applied first.
	 * Assign unique values when op order affects the result.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CourseBrush")
	int32 Priority = 0;

	// #endregion
};