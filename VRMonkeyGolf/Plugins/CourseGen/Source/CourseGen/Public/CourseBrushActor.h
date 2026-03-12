#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/CourseBrushComponent.h"

#include "CourseBrushActor.generated.h"

class UPCGComponent;

/**
 * ACourseBrushActor
 *
 * Thin host actor for UCourseBrushComponent.
 * All data lives on the component — this actor exists solely to be
 * placeable in the level and discoverable by PCG world queries.
 *
 * Place in the level near a CourseHoleActor. The hole's PCG graph
 * queries all ACourseBrushActors within the hole's bounding box,
 * reads their UCourseBrushComponent metadata, and applies the
 * Polygon2D boolean ops in Priority order.
 */
UCLASS(HideCategories=(Replication, Networking, Input, Rendering, HLOD, Physics))
class COURSEGEN_API ACourseBrushActor : public AActor
{
	GENERATED_BODY()

public:

	ACourseBrushActor();

	/** All brush data — target, mode, priority. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="CourseBrush")
	TObjectPtr<UCourseBrushComponent> CourseBrushComponent;

	/** PCG component — allows the hole graph to reach this actor via world queries. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="CourseBrush", AdvancedDisplay)
	TObjectPtr<UPCGComponent> PCGComponent;



    virtual void OnConstruction(const FTransform& Transform) override;

    private:

    void UpdateActorTags();
};