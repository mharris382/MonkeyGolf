// VRGolfGameMode.h
#pragma once

#include "CoreMinimal.h"
#include "VRGolfGameModeBase.h"
#include "Delegates/DelegateCombinations.h"
#include "VRGolfGameMode.generated.h"


DECLARE_LOG_CATEGORY_EXTERN(LogVRGolf, Log, All);

UENUM(BlueprintType)
enum class EGolfTurnOrder : uint8
{
    Sequential,     // Player 1, 2, 3... in order
    FurthestFirst   // Furthest from hole goes first (like real golf)
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
 * VR Golf Game Mode - Competitive play with scoring and turns
 * 
 * Extends AVRGolfGameModeBase with:
 * - Player scoring via PlayerState
 * - Turn management (sequential or furthest-first)
 * - Round progression (multiple holes)
 * - Win conditions
 * - Multiplayer support
 * 
 * Use AVRGolfGameModeBase for practice/training modes without scoring
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

    /**
     * Start a full round of golf
     */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    void StartRound();

    /**
     * Override base LoadHole to add scoring setup
     */
    virtual void LoadHole(int32 HoleNumber) override;

    /**
     * Called when a player takes a stroke
     */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    void OnPlayerStroke(class AVRGolfPlayerState* PlayerState, class AVRGolfBall* Ball);

    /**
     * Override base OnBallCompletedHole to add scoring
     */
    virtual void OnBallCompletedHole(class AVRGolfBall* Ball) override;

    /**
     * Manually advance to next hole
     */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    void SkipToNextHole();

    // === TURN MANAGEMENT ===

    /**
     * Advance to next player's turn
     */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    void AdvanceTurn();

    /**
     * Get current turn player
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    class AVRGolfPlayerState* GetCurrentTurnPlayer() const;

    /**
     * Check if it's a specific player's turn
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    bool IsPlayersTurn(class AVRGolfPlayerState* PlayerState) const;

    /**
     * Get current game phase
     */
    UFUNCTION(BlueprintPure, Category = "Golf")
    EGamePhase GetGamePhase() const { return CurrentPhase; }

    // === CONFIGURATION ===

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Rules")
    EGolfTurnOrder TurnOrder = EGolfTurnOrder::Sequential;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Rules")
    bool bAllowSimultaneousPlay = false;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Rules")
    bool bAutoAdvanceTurn = true;

    /**
     * Override ghost ball check for multiplayer
     */
    virtual bool CanPlayerUseGhostBall(APlayerController* PlayerController) const override;

protected:
    // Player tracking
    UPROPERTY()
    TArray<class AVRGolfPlayerState*> ActivePlayerStates;

    // Map players to their states (needed because base class uses PlayerController keys)
    UPROPERTY()
    TMap<APlayerController*, class AVRGolfPlayerState*> ControllerToState;

    // Turn state
    int32 CurrentTurnIndex;

    UPROPERTY()
    class AVRGolfPlayerState* CurrentTurnPlayerState;

    // Hole state
    int32 CurrentHoleNumber;

    UPROPERTY()
    EGamePhase CurrentPhase;

    UPROPERTY()
    TSet<class AVRGolfPlayerState*> PlayersCompletedHole;

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

    // Helper to get player state from controller
    class AVRGolfPlayerState* GetPlayerStateForController(APlayerController* Controller) const;
};
