// VRGolfPlayerState.cpp

#include "VRGolfPlayerState.h"
#include "Net/UnrealNetwork.h"

AVRGolfPlayerState::AVRGolfPlayerState()
{
    // Defaults set in-line on declarations; nothing extra needed here.
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

void AVRGolfPlayerState::Server_AddStroke_Implementation(int32 HoleNumber)
{
    AddStroke_Internal(HoleNumber);
}

void AVRGolfPlayerState::AddStroke_Internal(int32 HoleNumber)
{
    CurrentHoleStrokes++;
    UE_LOG(LogTemp, Log, TEXT("[GolfPS] %s — Stroke #%d on hole %d"),
           *GetPlayerName(), CurrentHoleStrokes, HoleNumber);
}

void AVRGolfPlayerState::CompleteHole(int32 HoleNumber, int32 FinalStrokes, int32 Par)
{
    // Authority-only: this is always called from GameMode.
    if (!HasAuthority()) return;

    FHoleScore NewScore;
    NewScore.HoleNumber = HoleNumber;
    NewScore.Strokes    = FinalStrokes;
    NewScore.Par        = Par;
    NewScore.bCompleted = true;
    NewScore.GolfScore  = NewScore.GetGolfScore();   // cache for BP/replication

    HoleScores.Add(NewScore);
    CurrentHoleStrokes = 0;

    UE_LOG(LogTemp, Log, TEXT("[GolfPS] %s completed hole %d: %d strokes (Par %d, %+d)"),
           *GetPlayerName(), HoleNumber, FinalStrokes, Par, NewScore.GetScoreToPar());

    // Manual call: OnRep_ doesn't auto-fire on the server after a replicated
    // property mutation — clients receive it via replication automatically.
    OnRep_HoleScores();
}

int32 AVRGolfPlayerState::GetTotalStrokes() const
{
    int32 Total = 0;
    for (const FHoleScore& S : HoleScores)
        Total += S.Strokes;
    return Total;
}

int32 AVRGolfPlayerState::GetTotalScore() const
{
    int32 Total = 0;
    for (const FHoleScore& S : HoleScores)
        Total += S.GetScoreToPar();
    return Total;
}

// ---------------------------------------------------------------------------
// Turn
// ---------------------------------------------------------------------------

void AVRGolfPlayerState::SetIsMyTurn(bool bInTurn)
{
    // Authority-only: always driven by GameMode.
    if (!HasAuthority()) return;

    bIsMyTurn = bInTurn;
    OnRep_IsMyTurn();   // manual call for server; clients get it via replication
}

// ---------------------------------------------------------------------------
// Rep notifies
// ---------------------------------------------------------------------------

void AVRGolfPlayerState::OnRep_HoleScores()
{
    if (HoleScores.IsEmpty()) return;

    const FHoleScore& Last = HoleScores.Last();
    OnPlayerCompletedHole.Broadcast(this, Last.HoleNumber, Last);
}

void AVRGolfPlayerState::OnRep_IsMyTurn()
{
    if (bIsMyTurn)
    {
        UE_LOG(LogTemp, Log, TEXT("[GolfPS] It's %s's turn"), *GetPlayerName());
        OnPlayerTurnChanged.Broadcast(this);
    }
}

// ---------------------------------------------------------------------------
// Profile identity
// ---------------------------------------------------------------------------

void AVRGolfPlayerState::SetProfileIdentity(const FString& InProfileName, const FGuid& InProfileID)
{
    ProfileName = InProfileName;
    ProfileID   = InProfileID;
    SetPlayerName(InProfileName);   // keep engine PlayerName in sync
}

void AVRGolfPlayerState::OnRep_ProfileName()
{
    SetPlayerName(ProfileName);     // keep engine PlayerName in sync on clients
    OnPlayerProfileChanged.Broadcast(this);
}