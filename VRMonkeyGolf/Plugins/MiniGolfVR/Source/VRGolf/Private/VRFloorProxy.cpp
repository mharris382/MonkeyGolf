// VRFloorProxy.cpp
#include "VRFloorProxy.h"

AVRFloorProxy::AVRFloorProxy()
{
    PrimaryActorTick.bCanEverTick = false; // Ball drives this, no self-tick needed
    bReplicates = false; // Purely cosmetic/local — no need to replicate

    USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = Root;
}

void AVRFloorProxy::UpdateFromFloorHit(const FVector& FloorPosition, const FHitResult& Hit)
{
    // Position only, never rotation — that's the whole point
    SetActorLocation(FloorPosition);
    LastFloorNormal = Hit.ImpactNormal;
    LastFloorHit = Hit;
    OnBallHit(Hit);
}

void AVRFloorProxy::NotifyAirborne()
{
    if (bIsGrounded)
    {
        bIsGrounded = false;
        OnProxyAirborne.Broadcast(this);
    }
}

void AVRFloorProxy::NotifyLanded(const FHitResult& Hit)
{
    if (!bIsGrounded)
    {
        bIsGrounded = true;
        LastFloorNormal = Hit.ImpactNormal;
        LastFloorHit = Hit;
        OnProxyLanded.Broadcast(this, Hit);
    }
}
