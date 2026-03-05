// VRGolfHole.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "VRGolfHole.generated.h"

/**
 * Configuration data for a golf hole
 */
USTRUCT(BlueprintType)
struct FGolfHoleConfig
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    int32 Par = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    FVector TeePosition = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    FRotator TeeRotation = FRotator::ZeroRotator;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    FString HoleName = TEXT("Hole 1");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    int32 HoleNumber = 1;
};

/**
 * Stage target for multi-stage holes
 * Allows different aim targets based on which surface the ball is on
 */
USTRUCT(BlueprintType)
struct FGolfStageTarget
{
    GENERATED_BODY()

    // The primitive component that defines this stage (ball must be on this surface)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    UPrimitiveComponent* StageSurface = nullptr;

    // The target to aim for from this stage
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    AActor* TargetActor = nullptr;

    // Or direct transform if no actor
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    FTransform TargetTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    bool bUseTransform = false;
};

/**
 * VR Golf Hole - Defines a single hole in the course
 * 
 * Responsibilities:
 * - Hole configuration (par, tee position, name)
 * - Trigger volumes (hole completion, out of bounds, hazards)
 * - Surface validation (which surfaces are puttable)
 * - Multi-stage targets (for complex holes with platforms)
 * - PCG integration (surfaces can be procedurally generated)
 */
UCLASS()
class VRGOLF_API AVRGolfHole : public AActor
{
    GENERATED_BODY()

public:
    AVRGolfHole();

    // Hole configuration
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf")
    FGolfHoleConfig HoleConfig;

    // === TRIGGER VOLUMES ===
    
    /**
     * Hole completion trigger (ball enters here = hole complete)
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf|Triggers")
    class UBoxComponent* HoleTrigger;

    /**
     * Out of bounds triggers (ball resets when touching these)
     * Can have multiple for complex hole shapes
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf|Triggers")
    TArray<class UBoxComponent*> OutOfBoundsTriggers;

    /**
     * Destroy/hazard triggers (water, lava, etc.)
     * Ball resets with possible penalty
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf|Triggers")
    TArray<class UBoxComponent*> DestroyTriggers;

    // === SURFACE DEFINITIONS ===
    
    /**
     * Actors/surfaces that are valid for putting
     * Ball can stop on these surfaces
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf|Surfaces")
    TArray<AActor*> PuttableSurfaces;

    // === MULTI-STAGE SUPPORT ===
    
    /**
     * Stage targets for multi-stage holes
     * Example: Tee → Landing Platform → Green → Hole
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Golf|Stages")
    TArray<FGolfStageTarget> StageTargets;

    // === QUERIES ===

    /**
     * Check if a surface component is valid for putting
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    bool IsPuttableSurface(UPrimitiveComponent* Surface) const;

    /**
     * Get tee position for ball spawn
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    FVector GetTeePosition() const { return HoleConfig.TeePosition; }

    /**
     * Get tee rotation for ball spawn orientation
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    FRotator GetTeeRotation() const { return HoleConfig.TeeRotation; }

    /**
     * Get par for this hole
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    int32 GetPar() const { return HoleConfig.Par; }

    /**
     * Get target location based on which surface the ball is on
     * Used for teleport aim alignment
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    FVector GetTargetForSurface(UPrimitiveComponent* Surface) const;

protected:
    virtual void BeginPlay() override;

    // Cached surface components for fast lookup
    TSet<UPrimitiveComponent*> PuttableSurfaceComponents;

    /**
     * Cache all puttable surface components at begin play
     * Avoids searching actor hierarchy every frame
     */
    void CachePuttableSurfaces();
};
