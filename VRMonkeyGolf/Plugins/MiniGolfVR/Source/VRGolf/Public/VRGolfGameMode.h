// VRGolfGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "VRGolfGameModeBase.h"
#include "VRGolfGameMode.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVRGolf, Log, All);

UENUM(BlueprintType)
enum class EGolfTurnOrder : uint8
{
    Sequential,     // Player 1, 2, 3... in fixed order
    FurthestFirst   // Furthest from hole goes first (real golf rules)
};

UENUM(BlueprintType)
enum class EGamePhase : uint8
{
    WaitingToStart,
    InProgress,
    HoleComplete,
    RoundComplete
};

/**
 * VR Golf Game Mode — Competitive play with scoring and turns.
 *
 * Extends AVRGolfGameModeBase with:
 *   - Player scoring via AVRGolfPlayerState
 *   - Turn management (sequential or furthest-first)
 *   - Round progression (multiple holes)
 *   - Win conditions
 *
 * Network session handling, player tracking, and profile identity
 * are all inherited from AVRGolfGameModeBase — do not duplicate here.
 */
UCLASS()
class VRGOLF_API AVRGolfGameMode : public AVRGolfGameModeBase
{
    GENERATED_BODY()

public:
    AVRGolfGameMode();

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    // === GAME FLOW ===

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void StartRound();

    virtual void LoadHole(int32 HoleNumber) override;

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void OnPlayerStroke(class AVRGolfPlayerState* PlayerState, class AVRGolfBall* Ball);

    virtual void OnBallCompletedHole(class AVRGolfBall* Ball) override;

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void SkipToNextHole();

    // === TURN MANAGEMENT ===

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void AdvanceTurn();

    UFUNCTION(BlueprintPure, Category = "Golf")
    class AVRGolfPlayerState* GetCurrentTurnPlayer() const;

    UFUNCTION(BlueprintPure, Category = "Golf")
    bool IsPlayersTurn(class AVRGolfPlayerState* PlayerState) const;

    UFUNCTION(BlueprintPure, Category = "Golf")
    EGamePhase GetGamePhase() const { return CurrentPhase; }

    // === DISCONNECT OVERRIDE ===

    /**
     * Override base disconnect to also handle turn advancement
     * and hole completion when a player drops mid-round.
     */
    virtual void RemovePlayerFromSession(class AVRGolfPlayerState* LeavingPlayerState) override;

    // === CONFIGURATION ===

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Rules")
    EGolfTurnOrder TurnOrder = EGolfTurnOrder::Sequential;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Rules")
    bool bAllowSimultaneousPlay = false;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Rules")
    bool bAutoAdvanceTurn = true;

    virtual bool CanPlayerUseGhostBall(APlayerController* PlayerController) const override;

protected:
    // Turn state
    int32 CurrentTurnIndex = 0;

    UPROPERTY()
    class AVRGolfPlayerState* CurrentTurnPlayerState;

    EGamePhase CurrentPhase = EGamePhase::WaitingToStart;

    UPROPERTY()
    TSet<class AVRGolfPlayerState*> PlayersCompletedHole;

    int32 CurrentHoleNumber = 1;

    // Initialization
    void InitializePlayers();
    void SpawnPlayerBalls();

    // Turn logic
    void DetermineNextPlayer();
    class AVRGolfPlayerState* GetFurthestPlayer();
    void SetCurrentTurnPlayer(class AVRGolfPlayerState* PlayerState);
    void NotifyTurnChange();

    // Hole management
    void CheckIfHoleComplete();
    void OnAllPlayersCompleteHole();
    void CleanupHole();

    // Round management
    void OnRoundComplete();
};