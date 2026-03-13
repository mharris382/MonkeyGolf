// VRGolfPlayerState.cpp

#include "VRGolfPlayerState.h"
#include "Net/UnrealNetwork.h"

AVRGolfPlayerState::AVRGolfPlayerState()
{
    CurrentHoleStrokes = 0;
    bIsMyTurn = false;
}

void AVRGolfPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AVRGolfPlayerState, HoleScores);
    DOREPLIFETIME(AVRGolfPlayerState, CurrentHoleStrokes);
    DOREPLIFETIME(AVRGolfPlayerState, bIsMyTurn);
    DOREPLIFETIME(AVRGolfPlayerState, ProfileName);
    DOREPLIFETIME(AVRGolfPlayerState, ProfileID);
}

// ---------------------------------------------------------------------------
// Score
// ---------------------------------------------------------------------------

void AVRGolfPlayerState::AddStroke(int32 HoleNumber)
{
    CurrentHoleStrokes++;

    UE_LOG(LogTemp, Log, TEXT("Player %s: Stroke #%d on hole %d"),
           *GetPlayerName(), CurrentHoleStrokes, HoleNumber);
}

void AVRGolfPlayerState::CompleteHole(int32 HoleNumber, int32 FinalStrokes, int32 Par)
{
    FHoleScore NewScore;
    NewScore.HoleNumber = HoleNumber;
    NewScore.Strokes    = FinalStrokes;
    NewScore.Par        = Par;
    NewScore.bCompleted = true;

    HoleScores.Add(NewScore);
    CurrentHoleStrokes = 0;

    UE_LOG(LogTemp, Log, TEXT("Player %s completed hole %d: %d strokes (Par %d, %+d)"),
           *GetPlayerName(), HoleNumber, FinalStrokes, Par, NewScore.GetScoreToPar());

    OnRep_HoleScores();
}

int32 AVRGolfPlayerState::GetTotalStrokes() const
{
    int32 Total = 0;
    for (const FHoleScore& S : HoleScores)
    {
        Total += S.Strokes;
    }
    return Total;
}

int32 AVRGolfPlayerState::GetTotalScore() const
{
    int32 Total = 0;
    for (const FHoleScore& S : HoleScores)
    {
        Total += S.GetScoreToPar();
    }
    return Total;
}

// ---------------------------------------------------------------------------
// Turn
// ---------------------------------------------------------------------------

void AVRGolfPlayerState::SetIsMyTurn(bool bInTurn)
{
    bIsMyTurn = bInTurn;
    OnRep_IsMyTurn();
}

void AVRGolfPlayerState::OnRep_HoleScores()
{
    // Broadcast delegate here when UI scoreboard is wired up
}

void AVRGolfPlayerState::OnRep_IsMyTurn()
{
    if (bIsMyTurn)
    {
        UE_LOG(LogTemp, Log, TEXT("It's my turn! (Player: %s)"), *GetPlayerName());
    }
    // Broadcast delegate here for turn indicator UI
}

// ---------------------------------------------------------------------------
// Profile identity
// ---------------------------------------------------------------------------

void AVRGolfPlayerState::SetProfileIdentity(const FString& InProfileName, const FGuid& InProfileID)
{
    ProfileName = InProfileName;
    ProfileID   = InProfileID;

    // Keep UE's built-in PlayerName in sync so VRE and engine systems
    // display the correct name without additional wiring
    SetPlayerName(InProfileName);
}

void AVRGolfPlayerState::OnRep_ProfileName()
{
    // Keep engine PlayerName in sync on clients
    SetPlayerName(ProfileName);
    // Broadcast delegate here for scoreboard name update
}