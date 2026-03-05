// VRGolfGhostBall.h
#pragma once

#include "CoreMinimal.h"
#include "VRGolfBall.h"
#include "VRGolfGhostBall.generated.h"

/**
 * Ghost Ball - Practice/preview ball that doesn't count for scoring
 * 
 * Features:
 * - Client-side only (no replication)
 * - Fades out after max lifetime
 * - Doesn't trigger hole completion
 * - Doesn't count strokes
 * - Perfect for testing trick shots without penalty
 */
UCLASS()
class VRGOLF_API AVRGolfGhostBall : public AVRGolfBall
{
    GENERATED_BODY()

public:
    AVRGolfGhostBall();

    virtual void Tick(float DeltaTime) override;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost")
    float MaxLifetime = 5.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Ghost")
    float FadeStartTime = 3.0f;

protected:
    virtual void BeginPlay() override;

    float LifeTime;
    
    UPROPERTY()
    UMaterialInstanceDynamic* GhostMaterial;

    void UpdateGhostVisuals(float DeltaTime);
    void DestroyGhostBall();

    // Override ball behaviors - ghost balls don't affect game state
    virtual void ApplyStroke(const FVector& ImpulseDirection, float ImpulseMagnitude) override;
    
    // Ghost balls pass through triggers
    virtual void OnBallBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult) override;

    
};
