// VRGolfGameState.cpp

#include "VRGolfGameState.h"
#include "VRGolfPlayerState.h"
#include "Net/UnrealNetwork.h"

AVRGolfGameState::AVRGolfGameState()
{
    CurrentHoleNumber = 0;
    TotalHoles = 18;
    CurrentTurnPlayer = nullptr;
}

void AVRGolfGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AVRGolfGameState, CurrentHoleNumber);
    DOREPLIFETIME(AVRGolfGameState, TotalHoles);
    DOREPLIFETIME(AVRGolfGameState, CurrentTurnPlayer);
}

TArray<AVRGolfPlayerState*> AVRGolfGameState::GetSortedPlayersByScore() const
{
    TArray<AVRGolfPlayerState*> SortedPlayers;

    // Gather all golf player states
    for (APlayerState* PS : PlayerArray)
    {
        AVRGolfPlayerState* GolfPS = Cast<AVRGolfPlayerState>(PS);
        if (GolfPS)
        {
            SortedPlayers.Add(GolfPS);
        }
    }

    // Sort by total score (lowest score first = best player)
    SortedPlayers.Sort([](const AVRGolfPlayerState& A, const AVRGolfPlayerState& B)
    {
        return A.GetTotalScore() < B.GetTotalScore();
    });

    return SortedPlayers;
}

void AVRGolfGameState::OnRep_CurrentHole()
{
    UE_LOG(LogTemp, Log, TEXT("Game State: Now on hole %d of %d"), CurrentHoleNumber, TotalHoles);
    
    // This is where you'd update UI to show current hole
    // Broadcast event for hole change
}

void AVRGolfGameState::OnRep_CurrentTurn()
{
    if (CurrentTurnPlayer)
    {
        UE_LOG(LogTemp, Log, TEXT("Game State: Current turn - %s"), *CurrentTurnPlayer->GetPlayerName());
    }
    
    // Update turn indicator UI
}
