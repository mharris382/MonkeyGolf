#include "CourseBrushActor.h"
#include "CourseGenTags.h"
#include "PCGComponent.h"

ACourseBrushActor::ACourseBrushActor()
{
	PrimaryActorTick.bCanEverTick = false;

	CourseBrushComponent = CreateDefaultSubobject<UCourseBrushComponent>(TEXT("CourseBrushComponent"));
	SetRootComponent(CourseBrushComponent);

	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCGComponent"));
}


void ACourseBrushActor::OnConstruction(const FTransform& Transform)
{
    Super::OnConstruction(Transform);
	UpdateActorTags();
}


void ACourseBrushActor::UpdateActorTags()
{
    if(!CourseBrushComponent)
        return;
    //TODO: get set of tags from BrushComponent that can be used to identify target type and operation mode in PCG (PCG reads actor tags directly, all we need to do is make sure they are correct/up-to-date)
}