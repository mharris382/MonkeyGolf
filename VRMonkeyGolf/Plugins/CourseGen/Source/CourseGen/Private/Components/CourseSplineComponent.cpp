#include "Components/CourseSplineComponent.h"

UCourseSplineComponent::UCourseSplineComponent()
{
	// Closed loop — course outlines are always closed polygons
	SetClosedLoop(true, /*bUpdateSpline*/true);

	// Default spline color to green for viewport clarity
#if WITH_EDITORONLY_DATA
	EditorUnselectedSplineSegmentColor = FLinearColor(0.1f, 0.8f, 0.1f, 1.f);
	EditorSelectedSplineSegmentColor   = FLinearColor(1.0f, 0.9f, 0.1f, 1.f);
#endif
}