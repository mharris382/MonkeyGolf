// VRGolfGameModeBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "VRGolfGameModeBase.generated.h"

/**
 * VR Golf Game Mode Base - Core mechanics WITHOUT scoring
 * 
 * Use this directly for:
 * - Practice mode (no scores, no turns)
 * - Training levels
 * - Trick shot challenges
 * - Free play
 * 
 * Provides:
 * - Hole loading and management
 * - Ball spawning and tracking
 * - Ghost ball permissions
 * - Teleport system
 * - Reset handling
 * 
 * Does NOT provide:
 * - Scoring
 * - Turn management
 * - Win conditions
 * - PlayerState integration
 * 
 * Extend with AVRGolfGameMode for competitive play with scoring
 */
UCLASS()
class VRGOLF_API AVRGolfGameModeBase : public AGameMode
{
    GENERATED_BODY()

public:
    AVRGolfGameModeBase();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // === HOLE MANAGEMENT ===

    /**
     * Load and activate a specific hole
     */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    virtual void LoadHole(int32 HoleNumber);

    /**
     * Get the currently active hole
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    class AVRGolfHole* GetCurrentHole() const { return CurrentHole; }

    // === BALL MANAGEMENT ===

    /**
     * Spawn a ball for a player at the tee
     */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    virtual class AVRGolfBall* SpawnBallForPlayer(APlayerController* PlayerController);

    /**
     * Get ball for a specific player
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    class AVRGolfBall* GetPlayerBall(APlayerController* PlayerController) const;

    /**
     * Called when a ball completes the hole
     */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    virtual void OnBallCompletedHole(class AVRGolfBall* Ball);

    // === GHOST BALL SYSTEM ===

    /**
     * Can this player spawn a ghost ball?
     */
    UFUNCTION(BlueprintPure, Category = "Golf|Ghost")
    virtual bool CanPlayerUseGhostBall(APlayerController* PlayerController) const;

    /**
     * Spawn a ghost ball for practice
     */
    UFUNCTION(BlueprintCallable, Category = "Golf|Ghost")
    virtual class AVRGolfGhostBall* SpawnGhostBall(class AVRGolfBall* SourceBall);

    /**
     * Track ghost ball usage (for limits)
     */
    UFUNCTION(BlueprintCallable, Category = "Golf|Ghost")
    virtual void OnPlayerUseGhostBall(APlayerController* PlayerController);

    // === TELEPORT SYSTEM ===

    /**
     * Teleport player to their ball with aim alignment
     */
    UFUNCTION(BlueprintCallable, Category = "Golf|Teleport")
    virtual void TeleportPlayerToBall(APlayerController* PlayerController);

    // === CONFIGURATION ===

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Ball")
    TSubclassOf<class AVRGolfBall> BallClass;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Ghost")
    TSubclassOf<class AVRGolfGhostBall> GhostBallClass;

protected:
    // Current active hole
    UPROPERTY()
    class AVRGolfHole* CurrentHole;

    // Track balls per player
    UPROPERTY()
    TMap<APlayerController*, class AVRGolfBall*> PlayerBalls;

    // Track ghost ball usage per player per hole
    TMap<APlayerController*, int32> GhostBallUsageThisHole;

    // === HOLE LOADING ===

    /**
     * Find hole actor in the level by hole number
     */
    class AVRGolfHole* FindHoleInLevel(int32 HoleNumber) const;

    // === TELEPORT HELPERS ===

    /**
     * Calculate where to teleport player based on ball position and target
     */
    FTransform CalculateTeleportTransform(class AVRGolfBall* Ball, const FVector& TargetLocation) const;

    /**
     * Get the target location the ball should aim for
     * (accounts for multi-stage holes)
     */
    FVector GetTargetLocationForBall(class AVRGolfBall* Ball) const;

    // === SETTINGS ACCESS ===

    const class UVRGolfSettings* GetGolfSettings() const;
};
