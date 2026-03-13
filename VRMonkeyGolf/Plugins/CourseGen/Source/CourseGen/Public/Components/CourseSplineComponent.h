#pragma once

#include "CoreMinimal.h"
#include "Components/SplineComponent.h"

#include "CourseSplineComponent.generated.h"

/**
 * Role of this spline within the course generation pipeline.
 * PCG graphs filter CourseSplineComponents by role to determine
 * which generation stage each spline participates in.
 */
UENUM(BlueprintType)
enum class ECourseSplineRole : uint8
{
	/** Primary playable surface. Triangulated into the green mesh. */
	Green,

	/**
	 * Additional green island — treated as a separate mesh that is
	 * unioned or kept as a discrete surface depending on PCG graph.
	 */
	Island,

	/**
	 * Defines the terrain boundary/skirt zone surrounding the green.
	 * Drives the PCG conformance and dressing falloff band.
	 */
	Boundary,

	/** Lake, pond, or stream cutout. Subtracted from the green via Polygon2D difference. */
	WaterFeature
};

/**
 * UCourseSplineComponent
 *
 * USplineComponent subclass that carries per-spline metadata for the
 * CourseGen PCG pipeline. Closed loop by default.
 *
 * Analogous to the Water plugin's water body splines — the spline shape
 * drives geometry generation, the metadata properties drive PCG behavior.
 *
 * Add instances as child components on ACourseHoleActor. The actor's
 * OnConstruction unions all CourseSplineComponent bounds automatically.
 *
 * PCG access pattern:
 *   GetActorData / GetComponentsByClass → filter by ECourseSplineRole
 *   → feed into appropriate graph stage (green gen, boundary, water cutout, etc.)
 */
UCLASS(ClassGroup=(CourseGen), meta=(BlueprintSpawnableComponent),
	HideCategories=(Collision, Navigation, AssetUserData, Mobile))
class COURSEGEN_API UCourseSplineComponent : public USplineComponent
{
	GENERATED_BODY()

public:

	UCourseSplineComponent();

	// #region Per-Spline Metadata

	/**
	 * Role of this spline in the course generation pipeline.
	 * Determines which PCG graph stage consumes this spline.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CourseSpline")
	ECourseSplineRole Role = ECourseSplineRole::Green;




	/**
	 * Depth of downward extrusion when building green mesh side walls.
	 * Gives the green a physical thickness so it reads as a solid surface
	 * rather than a flat plane. Only relevant for Green and Island roles.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CourseSpline",
		meta=(ClampMin="0.0", UIMin="0.0", UIMax="200.0", ForceUnits="cm",
			EditCondition="Role==ECourseSplineRole::Green||Role==ECourseSplineRole::Island"))
	float ExtrusionDepth = 15.f;

	/**
	 * Width of the terrain blend/skirt falloff from this spline's edge.
	 * Overrides the course-level default for this specific spline.
	 * Only relevant for Boundary role splines.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="CourseSpline",
		meta=(ClampMin="0.0", UIMin="0.0", UIMax="1000.0", ForceUnits="cm",
			EditCondition="Role==ECourseSplineRole::Boundary"))
	float BoundaryBlendDistance = 200.f;

	// #endregion
};