
#include "Actors/VRGolfHoleGenerator.h"
#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"

void AVRGolfHoleGenerator::BeginPlay()
{
	Super::BeginPlay();
}

void AVRGolfHoleGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
}



#if WITH_EDITOR
void AVRGolfHoleGenerator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}
#endif