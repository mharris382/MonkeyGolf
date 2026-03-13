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

    int32 GetScoreToPar() const { return Strokes - Par; }
};

/**
 * Player state for VR Golf - tracks scores, turn state, hole progress, and profile identity.
 * Replicated to all clients for scoreboard display and room save identification.
 */
UCLASS()
class VRGOLF_API AVRGolfPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AVRGolfPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // === SCORE ===

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

    // === TURN ===

    UFUNCTION(BlueprintPure, Category = "Golf")
    bool IsMyTurn() const { return bIsMyTurn; }

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void SetIsMyTurn(bool bInTurn);

    // === PROFILE IDENTITY ===
    // Replicated so all clients can identify players on the scoreboard
    // and the room save system can record per-player scores correctly.

    /** Display name from profile. Pushed from GameModeBase on PostLogin. */
    UPROPERTY(ReplicatedUsing = OnRep_ProfileName, BlueprintReadOnly, Category = "Profile")
    FString ProfileName;

    /** Stable GUID from UMGProfileSaveGame. Never changes for a player. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Profile")
    FGuid ProfileID;

    /** Set profile identity. Called by AVRGolfGameModeBase::PostLogin. */
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void SetProfileIdentity(const FString& InProfileName, const FGuid& InProfileID);

protected:
    UPROPERTY(ReplicatedUsing = OnRep_HoleScores)
    TArray<FHoleScore> HoleScores;

    UPROPERTY(Replicated)
    int32 CurrentHoleStrokes;

    UPROPERTY(ReplicatedUsing = OnRep_IsMyTurn)
    bool bIsMyTurn;

    UFUNCTION()
    void OnRep_HoleScores();

    UFUNCTION()
    void OnRep_IsMyTurn();

    UFUNCTION()
    void OnRep_ProfileName();
};