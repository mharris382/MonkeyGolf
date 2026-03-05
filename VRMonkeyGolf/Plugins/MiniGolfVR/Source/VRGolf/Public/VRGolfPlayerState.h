// VRGolfPlayerState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "VRGolfPlayerState.generated.h"

/**
 * Score data for a single hole
 */
USTRUCT(BlueprintType)
struct FHoleScore
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Golf")
    int32 HoleNumber = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Golf")
    int32 Strokes = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Golf")
    int32 Par = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Golf")
    bool bCompleted = false;

    // Helper to calculate score relative to par (negative = under par, positive = over par)
    int32 GetScoreToPar() const { return Strokes - Par; }
};

/**
 * Player state for VR Golf - tracks scores, turn state, and hole progress
 * Replicated to all clients for scoreboard display
 */
UCLASS()
class VRGOLF_API AVRGolfPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AVRGolfPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Score management
    UFUNCTION(BlueprintCallable, Category = "Golf")
    void AddStroke(int32 HoleNumber);

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void CompleteHole(int32 HoleNumber, int32 FinalStrokes, int32 Par);

    UFUNCTION(BlueprintPure, Category = "Golf")
    int32 GetCurrentHoleStrokes() const { return CurrentHoleStrokes; }

    UFUNCTION(BlueprintPure, Category = "Golf")
    int32 GetTotalStrokes() const;

    UFUNCTION(BlueprintPure, Category = "Golf")
    int32 GetTotalScore() const;

    UFUNCTION(BlueprintPure, Category = "Golf")
    const TArray<FHoleScore>& GetHoleScores() const { return HoleScores; }

    // Turn state
    UFUNCTION(BlueprintPure, Category = "Golf")
    bool IsMyTurn() const { return bIsMyTurn; }

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void SetIsMyTurn(bool bInTurn);

protected:
    // Array of completed hole scores
    UPROPERTY(ReplicatedUsing = OnRep_HoleScores)
    TArray<FHoleScore> HoleScores;

    // Current hole stroke count (resets each hole)
    UPROPERTY(Replicated)
    int32 CurrentHoleStrokes;

    // Is it this player's turn?
    UPROPERTY(ReplicatedUsing = OnRep_IsMyTurn)
    bool bIsMyTurn;

    // Replication callbacks
    UFUNCTION()
    void OnRep_HoleScores();

    UFUNCTION()
    void OnRep_IsMyTurn();
};
