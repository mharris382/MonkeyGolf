// VRGolfGhostBall.cpp

#include "VRGolfGhostBall.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"

AVRGolfGhostBall::AVRGolfGhostBall()
{
    // Ghost balls are client-side only - no replication
    bReplicates = false;
    SetReplicateMovement(false);

    LifeTime = 0.0f;

    // Visual setup
    if (BallMesh)
    {
        // Ghost balls should be semi-transparent
        BallMesh->SetRenderCustomDepth(false);
        BallMesh->SetCastShadow(false);
    }

    // Reduce physics cost for ghost balls
    if (CollisionSphere)
    {
        // Don't collide with pawns/players
        CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
        // Still collide with world to show trajectory
    }
}

void AVRGolfGhostBall::BeginPlay()
{
    Super::BeginPlay();

    // Create dynamic material for fade effect
    if (BallMesh)
    {
        UMaterialInterface* BaseMaterial = BallMesh->GetMaterial(0);
        if (BaseMaterial)
        {
            GhostMaterial = UMaterialInstanceDynamic::Create(BaseMaterial, this);
            BallMesh->SetMaterial(0, GhostMaterial);
            
            // Initial ghost appearance (50% opacity, glowing blue)
            if (GhostMaterial)
            {
                GhostMaterial->SetScalarParameterValue(TEXT("Opacity"), 0.5f);
                GhostMaterial->SetVectorParameterValue(TEXT("EmissiveColor"), FLinearColor(0.2f, 0.5f, 1.0f));
            }
        }
    }
}

void AVRGolfGhostBall::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    LifeTime += DeltaTime;

    UpdateGhostVisuals(DeltaTime);

    // Destroy after max lifetime
    if (LifeTime >= MaxLifetime)
    {
        DestroyGhostBall();
    }
}

void AVRGolfGhostBall::UpdateGhostVisuals(float DeltaTime)
{
    if (!GhostMaterial)
        return;

    // Fade out as lifetime progresses
    if (LifeTime >= FadeStartTime)
    {
        float FadeProgress = (LifeTime - FadeStartTime) / (MaxLifetime - FadeStartTime);
        float Opacity = FMath::Lerp(0.5f, 0.0f, FadeProgress);
        
        GhostMaterial->SetScalarParameterValue(TEXT("Opacity"), Opacity);
    }
}

void AVRGolfGhostBall::DestroyGhostBall()
{
    // Optional: Spawn disappear VFX here
    UE_LOG(LogTemp, Log, TEXT("Ghost ball destroyed after %.2f seconds"), LifeTime);
    Destroy();
}

void AVRGolfGhostBall::ApplyStroke(const FVector& ImpulseDirection, float ImpulseMagnitude)
{
    // Ghost balls CAN be hit (for chaining practice shots)
    // But don't increment stroke count or call server
    
    if (CollisionSphere)
    {
        FVector Impulse = ImpulseDirection * ImpulseMagnitude;
        CollisionSphere->AddImpulse(Impulse, NAME_None, true);
    }

    CurrentState = EBallState::InMotion;
    // DON'T increment CurrentStrokes for ghost balls
    // DON'T call Server_ApplyStroke - this is client-only

    UE_LOG(LogTemp, Log, TEXT("Ghost ball stroke - not counted"));
}

void AVRGolfGhostBall::OnBallBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
                                           AActor* OtherActor, UPrimitiveComponent* OtherComp, 
                                           int32 OtherBodyIndex, bool bFromSweep, 
                                           const FHitResult& SweepResult)
{
    // Ghost balls don't trigger hole completion or game events
    // They just pass through triggers
    
    if (!OtherActor)
        return;

    // Optional: Destroy ghost ball on certain triggers for visual clarity
    if (OtherActor->ActorHasTag(TEXT("HoleTrigger")) || 
        OtherActor->ActorHasTag(TEXT("OutOfBounds")) ||
        OtherActor->ActorHasTag(TEXT("DestroyTrigger")))
    {
        UE_LOG(LogTemp, Log, TEXT("Ghost ball reached trigger: %s"), *OtherActor->GetName());
        DestroyGhostBall();
    }
}
