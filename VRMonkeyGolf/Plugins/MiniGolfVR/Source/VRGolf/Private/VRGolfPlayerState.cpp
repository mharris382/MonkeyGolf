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
}

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
    NewScore.Strokes = FinalStrokes;
    NewScore.Par = Par;
    NewScore.bCompleted = true;

    HoleScores.Add(NewScore);
    
    // Reset for next hole
    CurrentHoleStrokes = 0;

    UE_LOG(LogTemp, Log, TEXT("Player %s completed hole %d: %d strokes (Par %d, %+d)"), 
           *GetPlayerName(), HoleNumber, FinalStrokes, Par, NewScore.GetScoreToPar());

    // Notify clients
    OnRep_HoleScores();
}

int32 AVRGolfPlayerState::GetTotalStrokes() const
{
    int32 Total = 0;
    for (const FHoleScore& sc : HoleScores)
    {
        Total += sc.Strokes;
    }
    return Total;
}

int32 AVRGolfPlayerState::GetTotalScore() const
{
    int32 SCR = 0;
    for (const FHoleScore& HoleScore : HoleScores)
    {
        SCR += HoleScore.GetScoreToPar();
    }
    return SCR;
}

void AVRGolfPlayerState::SetIsMyTurn(bool bInTurn)
{
    bIsMyTurn = bInTurn;
    OnRep_IsMyTurn();
}

void AVRGolfPlayerState::OnRep_HoleScores()
{
    // Notify UI to update scoreboard
    // This is where you'd broadcast a delegate or event for UI updates
}

void AVRGolfPlayerState::OnRep_IsMyTurn()
{
    // Notify UI to show/hide turn indicator
    if (bIsMyTurn)
    {
        UE_LOG(LogTemp, Log, TEXT("It's my turn! (Player: %s)"), *GetPlayerName());
        // This is where you'd show UI feedback like "YOUR TURN" overlay
    }
}
