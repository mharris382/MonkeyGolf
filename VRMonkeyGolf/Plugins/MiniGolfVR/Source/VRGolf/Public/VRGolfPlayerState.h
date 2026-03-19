// VRGolfPlayerState.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "VRGolfPlayerState.generated.h"


UENUM(BlueprintType)
enum class EGolfScore : uint8
{
    HoleIncomplete = 10,
    HoleComplete    = 8,   // +4 or worse
    TripleBogey     = 7,
    DoubleBogey     = 6,
    Bogey           = 5,
    Par             = 4,
    Birdie          = 3,
    Eagle           = 2,
    Albatross       = 1,
    Condor          = 0,
    HoleInOne       = 9
};


/**
 * Score data for a single hole.
 * GolfScore is cached on CompleteHole (server-side) so BP can read it
 * without calling a struct function, and it replicates correctly.
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

    /** Cached on CompleteHole — safe to read from BP and replicated structs. */
    UPROPERTY(BlueprintReadOnly, Category = "Golf")
    EGolfScore GolfScore = EGolfScore::HoleIncomplete;

    /** Compute score from strokes/par. Call once on the server; store result in GolfScore. */
    EGolfScore GetGolfScore() const
    {
        if (!bCompleted)  return EGolfScore::HoleIncomplete;
        if (Strokes == 1) return EGolfScore::HoleInOne;
        // Maps (Strokes-Par)+4 onto the 0-8 range of the enum.
        // Condor(-4)=0 ... Par(0)=4 ... HoleComplete(+4 or worse)=8
        return static_cast<EGolfScore>(FMath::Clamp((Strokes - Par) + 4, 0, 8));
    }

    int32 GetScoreToPar() const { return Strokes - Par; }
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPlayerCompletedHole, class AVRGolfPlayerState*, PlayerState, int32, HoleNumber, FHoleScore, Score);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnPlayerStartedHole,    class AVRGolfPlayerState*, PlayerState, int32, HoleNumber);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerTurnChanged,     class AVRGolfPlayerState*, PlayerState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerProfileChanged,  class AVRGolfPlayerState*, PlayerState);


/**
 * Player state for VR Golf — tracks scores, turn state, hole progress, and profile identity.
 * Replicated to all clients for scoreboard display and room save identification.
 *
 * Authority rules:
 *   CompleteHole / SetIsMyTurn  — authority only (called by GameMode)
 *   Server_AddStroke            — Server RPC; safe to call from client pawn
 */
UCLASS()
class VRGOLF_API AVRGolfPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    AVRGolfPlayerState();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // === SCORE ===

    /**
     * Server RPC — increments stroke count for the current hole.
     * Call from wherever stroke input is detected (ball delegate, pawn input).
     * In singleplayer the RPC fires immediately on the same machine.
     */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Golf")
    void Server_AddStroke(int32 HoleNumber);

    /**
     * Authority only — called by GameMode when the ball is holed.
     * Snapshots the final score and broadcasts OnPlayerCompletedHole.
     */
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

    /** Authority only — called by GameMode to assign/revoke the turn. */
    UFUNCTION(BlueprintCallable, Category = "Golf")
    void SetIsMyTurn(bool bInTurn);

    // === PROFILE IDENTITY ===

    /** Display name from profile. Pushed from GameModeBase on PostLogin. */
    UPROPERTY(ReplicatedUsing = OnRep_ProfileName, BlueprintReadOnly, Category = "Profile")
    FString ProfileName;

    /** Stable GUID from UVRGolfProfileSaveGame. Never changes for a player. */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Profile")
    FGuid ProfileID;

    /** Set profile identity. Called by AVRGolfGameModeBase::PostLogin. */
    UFUNCTION(BlueprintCallable, Category = "Profile")
    void SetProfileIdentity(const FString& InProfileName, const FGuid& InProfileID);

    // === DELEGATES ===

    UPROPERTY(BlueprintAssignable, Category = "PlayerState|Events")
    FOnPlayerCompletedHole OnPlayerCompletedHole;

    UPROPERTY(BlueprintAssignable, Category = "PlayerState|Events")
    FOnPlayerStartedHole OnPlayerStartedHole;

    UPROPERTY(BlueprintAssignable, Category = "PlayerState|Events")
    FOnPlayerTurnChanged OnPlayerTurnChanged;

    UPROPERTY(BlueprintAssignable, Category = "PlayerState|Events")
    FOnPlayerProfileChanged OnPlayerProfileChanged;

protected:
    UPROPERTY(ReplicatedUsing = OnRep_HoleScores)
    TArray<FHoleScore> HoleScores;

    UPROPERTY(Replicated)
    int32 CurrentHoleStrokes = 0;

    UPROPERTY(ReplicatedUsing = OnRep_IsMyTurn)
    bool bIsMyTurn = false;

private:
    /** Shared implementation called by the Server RPC. */
    void AddStroke_Internal(int32 HoleNumber);

    UFUNCTION()
    void OnRep_HoleScores();

    UFUNCTION()
    void OnRep_IsMyTurn();

    UFUNCTION()
    void OnRep_ProfileName();
};