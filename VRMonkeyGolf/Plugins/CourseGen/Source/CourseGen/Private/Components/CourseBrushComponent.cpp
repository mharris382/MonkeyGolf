#include "Components/CourseBrushComponent.h"

UCourseBrushComponent::UCourseBrushComponent()
{
	// No tick, no collision — pure authoring shape
	PrimaryComponentTick.bCanEverTick = false;
	SetCollisionEnabled(ECollisionEnabled::NoCollision);
}