// VRGolfHole.cpp

#include "VRGolfHole.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GolfSurfaceInterface.h"

AVRGolfHole::AVRGolfHole()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create root component
    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;

    // Create hole trigger
    HoleTrigger = CreateDefaultSubobject<UBoxComponent>(TEXT("HoleTrigger"));
    HoleTrigger->SetupAttachment(RootComponent);
    HoleTrigger->SetBoxExtent(FVector(5.0f, 5.0f, 5.0f));
    HoleTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HoleTrigger->SetCollisionResponseToAllChannels(ECR_Ignore);
    HoleTrigger->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
    HoleTrigger->ComponentTags.Add(TEXT("HoleTrigger"));
}

void AVRGolfHole::BeginPlay()
{
    Super::BeginPlay();

    CachePuttableSurfaces();

    UE_LOG(LogTemp, Log, TEXT("Hole %d (%s) initialized - Par %d"), 
           HoleConfig.HoleNumber, *HoleConfig.HoleName, HoleConfig.Par);
}

void AVRGolfHole::CachePuttableSurfaces()
{
    PuttableSurfaceComponents.Empty();

    for (AActor* Actor : PuttableSurfaces)
    {
        if (!Actor)
            continue;

        // Get all primitive components from the actor
        TArray<UPrimitiveComponent*> PrimitiveComponents;
        Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

        for (UPrimitiveComponent* Comp : PrimitiveComponents)
        {
            PuttableSurfaceComponents.Add(Comp);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Cached %d puttable surface components for hole %d"), 
           PuttableSurfaceComponents.Num(), HoleConfig.HoleNumber);
}

bool AVRGolfHole::IsPuttableSurface(UPrimitiveComponent* Surface) const
{
    if (!Surface)
        return false;

    // Check cached surfaces first (fastest)
    if (PuttableSurfaceComponents.Contains(Surface))
        return true;

    // Check if component's actor implements golf surface interface
    AActor* pwner = Surface->GetOwner();
    if (pwner && pwner->GetClass()->ImplementsInterface(UGolfSurfaceInterface::StaticClass()))
    {
        return IGolfSurfaceInterface::Execute_IsPuttableSurface(pwner);
    }

    // Check component tags
    if (Surface->ComponentHasTag(TEXT("NonPuttable")))
        return false;

    // Default: assume it's puttable unless explicitly marked otherwise
    return true;
}

FVector AVRGolfHole::GetTargetForSurface(UPrimitiveComponent* Surface) const
{
    // Check if this surface has a custom stage target
    for (const FGolfStageTarget& Stage : StageTargets)
    {
        if (Stage.StageSurface == Surface)
        {
            if (Stage.bUseTransform)
            {
                return Stage.TargetTransform.GetLocation();
            }
            else if (Stage.TargetActor)
            {
                return Stage.TargetActor->GetActorLocation();
            }
        }
    }

    // Default: aim for the hole trigger
    return HoleTrigger ? HoleTrigger->GetComponentLocation() : GetActorLocation();
}
