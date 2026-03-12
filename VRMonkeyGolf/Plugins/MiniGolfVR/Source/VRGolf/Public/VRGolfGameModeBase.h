// VRGolfGameModeBase.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "VRGolfGameModeBase.generated.h"

/**
 * VR Golf Game Mode Base
 *
 * All game modes inherit from this — menu, practice, competitive, future modes.
 * Owns everything that is true for ALL modes:
 *   - Hole loading and management
 *   - Ball spawning and tracking
 *   - Ghost ball system
 *   - Teleport system
 *   - Network session handling (PostLogin, disconnect, host migration)
 *   - Player tracking (ActivePlayerStates, ControllerToState)
 *   - Profile identity wiring
 *
 * Does NOT own:
 *   - Scoring
 *   - Turn management
 *   - Win conditions
 *
 * Extend with AVRGolfGameMode for competitive play with scoring.
 * Use directly for menu, practice, or free play modes.
 */
UCLASS()
class VRGOLF_API AVRGolfGameModeBase : public AGameMode
{
    GENERATED_BODY()

public:
    AVRGolfGameModeBase();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // === NETWORK / SESSION ===

    /**
     * Called by UE when a new player controller connects (host only).
     * Registers player in tracking maps and wires profile identity.
     */
    virtual void PostLogin(APlayerController* NewPlayer) override;

    /**
     * Called by UE when a player disconnects (host only).
     * Cleans up tracking and notifies online subsystem.
     */
    virtual void NotifyPlayerDisconnected(APlayerController* ExitingPlayer);

    /**
     * Called when EOS promotes this machine to host after original host drops.
     * Notifies online subsystem so UI can respond.
     */
    UFUNCTION(BlueprintCallable, Category = "Golf|Network")
    virtual void HandleHostMigration();

    /**
     * Remove a player from active tracking and clean up their ball.
     * Safe to call from any derived game mode.
     */
    UFUNCTION(BlueprintCallable, Category = "Golf|Network")
    virtual void RemovePlayerFromSession(class AVRGolfPlayerState* LeavingPlayerState);

    /**
     * Lock or unlock late joining. Call SetAllowLateJoin(false) when round starts.
     */
    UFUNCTION(BlueprintCallable, Category = "Golf|Network")
    void SetAllowLateJoin(bool bAllow);

    // === HOLE MANAGEMENT ===

    UFUNCTION(BlueprintCallable, Category = "Golf")
    virtual void LoadHole(int32 HoleNumber);

    UFUNCTION(BlueprintPure, Category = "Golf")
    class AVRGolfHole* GetCurrentHole() const { return CurrentHole; }

    // === BALL MANAGEMENT ===

    UFUNCTION(BlueprintCallable, Category = "Golf")
    virtual class AVRGolfBall* SpawnBallForPlayer(APlayerController* PlayerController);

    UFUNCTION(BlueprintPure, Category = "Golf")
    class AVRGolfBall* GetPlayerBall(APlayerController* PlayerController) const;

    UFUNCTION(BlueprintCallable, Category = "Golf")
    virtual void OnBallCompletedHole(class AVRGolfBall* Ball);

    // === GHOST BALL SYSTEM ===

    UFUNCTION(BlueprintPure, Category = "Golf|Ghost")
    virtual bool CanPlayerUseGhostBall(APlayerController* PlayerController) const;

    UFUNCTION(BlueprintCallable, Category = "Golf|Ghost")
    virtual class AVRGolfGhostBall* SpawnGhostBall(class AVRGolfBall* SourceBall);

    UFUNCTION(BlueprintCallable, Category = "Golf|Ghost")
    virtual void OnPlayerUseGhostBall(APlayerController* PlayerController);

    // === TELEPORT SYSTEM ===

    UFUNCTION(BlueprintCallable, Category = "Golf|Teleport")
    virtual void TeleportPlayerToBall(APlayerController* PlayerController);

    // === CONFIGURATION ===

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Ball")
    TSubclassOf<class AVRGolfBall> BallClass;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Ghost")
    TSubclassOf<class AVRGolfGhostBall> GhostBallClass;

protected:
    // === PLAYER TRACKING ===
    // Lives in base so all game modes (menu, practice, competitive) share it.

    UPROPERTY()
    TArray<class AVRGolfPlayerState*> ActivePlayerStates;

    UPROPERTY()
    TMap<APlayerController*, class AVRGolfPlayerState*> ControllerToState;

    // === HOLE / BALL STATE ===

    UPROPERTY()
    class AVRGolfHole* CurrentHole;

    UPROPERTY()
    TMap<APlayerController*, class AVRGolfBall*> PlayerBalls;

    TMap<APlayerController*, int32> GhostBallUsageThisHole;

    // === SESSION STATE ===

    /** True once a round has started — gates late-join ball spawning. */
    bool bRoundStarted = false;

    // === HELPERS ===

    class AVRGolfHole* FindHoleInLevel(int32 HoleNumber) const;

    /** Helper used by derived classes to look up PlayerState from controller. */
    class AVRGolfPlayerState* GetPlayerStateForController(APlayerController* Controller) const;

    FTransform CalculateTeleportTransform(class AVRGolfBall* Ball, const FVector& TargetLocation) const;
    FVector GetTargetLocationForBall(class AVRGolfBall* Ball) const;

    const class UVRGolfSettings* GetGolfSettings() const;
};