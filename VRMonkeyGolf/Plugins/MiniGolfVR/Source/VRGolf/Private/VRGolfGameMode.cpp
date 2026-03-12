// VRGolfGameMode.cpp

#include "VRGolfGameMode.h"
#include "VRGolfPlayerState.h"
#include "VRGolfGameState.h"
#include "VRGolfBall.h"
#include "VRGolfHole.h"
#include "VRGolfSettings.h"
#include "Logging/LogMacros.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogVRGolf);

AVRGolfGameMode::AVRGolfGameMode()
{
    PlayerStateClass = AVRGolfPlayerState::StaticClass();
    GameStateClass = AVRGolfGameState::StaticClass();

    CurrentTurnPlayerState = nullptr;
}

void AVRGolfGameMode::BeginPlay()
{
    // Intentionally skip Super::BeginPlay() — base auto-loads hole 1,
    // but competitive mode waits for explicit StartRound() call.
    UE_LOG(LogVRGolf, Log, TEXT("VRGolfGameMode: Waiting for StartRound."));
}

void AVRGolfGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (CurrentPhase == EGamePhase::InProgress && bAutoAdvanceTurn)
    {
        CheckIfHoleComplete();
    }
}

// ---------------------------------------------------------------------------
// Game flow
// ---------------------------------------------------------------------------

void AVRGolfGameMode::StartRound()
{
    if (CurrentPhase != EGamePhase::WaitingToStart)
    {
        UE_LOG(LogVRGolf, Warning, TEXT("Cannot start round — already in progress."));
        return;
    }

    const UVRGolfSettings* Settings = GetGolfSettings();
    int32 TotalHoles = Settings ? Settings->DefaultTotalHoles : 18;

    UE_LOG(LogVRGolf, Log, TEXT("Starting round with %d holes."), TotalHoles);

    InitializePlayers();
    LoadHole(1);

    CurrentPhase = EGamePhase::InProgress;
    bRoundStarted = true;           // Inherited from base — gates late-join ball spawning
    SetAllowLateJoin(false);        // Lock the session once the round starts
}

void AVRGolfGameMode::InitializePlayers()
{
    // ActivePlayerStates and ControllerToState are owned by base.
    // They're already populated by PostLogin — just ensure turn index is reset.
    CurrentTurnIndex = 0;
}

void AVRGolfGameMode::LoadHole(int32 HoleNumber)
{
    UE_LOG(LogVRGolf, Log, TEXT("Loading hole %d (Competitive Mode)."), HoleNumber);

    Super::LoadHole(HoleNumber);
    if (!CurrentHole) return;

    CurrentHoleNumber = HoleNumber;
    PlayersCompletedHole.Empty();

    SpawnPlayerBalls();

    AVRGolfGameState* GS = GetGameState<AVRGolfGameState>();
    if (GS)
    {
        GS->CurrentHoleNumber = CurrentHoleNumber;
        const UVRGolfSettings* Settings = GetGolfSettings();
        GS->TotalHoles = Settings ? Settings->DefaultTotalHoles : 18;
        GS->OnRep_CurrentHole();
    }

    DetermineNextPlayer();
}

void AVRGolfGameMode::SpawnPlayerBalls()
{
    if (!CurrentHole) return;

    FVector  TeePosition = CurrentHole->GetTeePosition();
    FRotator TeeRotation = CurrentHole->GetTeeRotation();
    float    Spacing = 10.0f;
    int32    Index = 0;

    for (AVRGolfPlayerState* PS : ActivePlayerStates)
    {
        APlayerController* PC = nullptr;
        for (auto& Pair : ControllerToState)
        {
            if (Pair.Value == PS) { PC = Pair.Key; break; }
        }
        if (!PC) continue;

        FVector Offset = FVector(0, Index * Spacing, 0);
        FVector SpawnLocation = TeePosition + TeeRotation.RotateVector(Offset);

        AVRGolfBall* Ball = SpawnBallForPlayer(PC);
        if (Ball)
        {
            Ball->SetActorLocation(SpawnLocation);
        }
        Index++;
    }
}

void AVRGolfGameMode::OnPlayerStroke(AVRGolfPlayerState* PlayerState, AVRGolfBall* Ball)
{
    if (!PlayerState || !Ball) return;

    if (!bAllowSimultaneousPlay && !IsPlayersTurn(PlayerState))
    {
        UE_LOG(LogVRGolf, Warning, TEXT("Player %s stroked out of turn."), *PlayerState->GetPlayerName());
        return;
    }

    PlayerState->AddStroke(CurrentHoleNumber);
}

void AVRGolfGameMode::OnBallCompletedHole(AVRGolfBall* Ball)
{
    if (!Ball || !CurrentHole) return;

    // Find owning player via base class PlayerBalls map
    AVRGolfPlayerState* OwningPlayer = nullptr;
    for (const auto& Pair : PlayerBalls)
    {
        if (Pair.Value == Ball)
        {
            OwningPlayer = GetPlayerStateForController(Pair.Key);
            break;
        }
    }

    if (!OwningPlayer)
    {
        UE_LOG(LogVRGolf, Warning, TEXT("Ball completed hole but no owning player found."));
        return;
    }

    PlayersCompletedHole.Add(OwningPlayer);
    OwningPlayer->CompleteHole(CurrentHoleNumber, Ball->GetStrokeCount(), CurrentHole->GetPar());

    CheckIfHoleComplete();
}

void AVRGolfGameMode::SkipToNextHole()
{
    const UVRGolfSettings* Settings = GetGolfSettings();
    int32 TotalHoles = Settings ? Settings->DefaultTotalHoles : 18;

    if (CurrentHoleNumber >= TotalHoles)
    {
        OnRoundComplete();
    }
    else
    {
        CurrentPhase = EGamePhase::InProgress;
        CleanupHole();
        LoadHole(CurrentHoleNumber + 1);
    }
}

void AVRGolfGameMode::CleanupHole()
{
    for (auto& Pair : PlayerBalls)
    {
        if (Pair.Value) Pair.Value->Destroy();
    }
    PlayerBalls.Empty();
}

void AVRGolfGameMode::CheckIfHoleComplete()
{
    for (AVRGolfPlayerState* PS : ActivePlayerStates)
    {
        if (!PlayersCompletedHole.Contains(PS)) return;
    }
    OnAllPlayersCompleteHole();
}

void AVRGolfGameMode::OnAllPlayersCompleteHole()
{
    UE_LOG(LogVRGolf, Log, TEXT("All players completed hole %d."), CurrentHoleNumber);
    CurrentPhase = EGamePhase::HoleComplete;

    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
        {
            SkipToNextHole();
        }, 3.0f, false);
}

void AVRGolfGameMode::OnRoundComplete()
{
    UE_LOG(LogVRGolf, Log, TEXT("Round complete."));
    CurrentPhase = EGamePhase::RoundComplete;

    AVRGolfGameState* GS = GetGameState<AVRGolfGameState>();
    if (GS)
    {
        TArray<AVRGolfPlayerState*> Sorted = GS->GetSortedPlayersByScore();
        if (Sorted.Num() > 0)
        {
            UE_LOG(LogVRGolf, Log, TEXT("Winner: %s (%+d)"),
                *Sorted[0]->GetPlayerName(), Sorted[0]->GetTotalScore());
        }
    }
}

// ---------------------------------------------------------------------------
// Turn management
// ---------------------------------------------------------------------------

void AVRGolfGameMode::DetermineNextPlayer()
{
    if (bAllowSimultaneousPlay || ActivePlayerStates.Num() == 0) return;

    switch (TurnOrder)
    {
    case EGolfTurnOrder::Sequential:
        CurrentTurnIndex = (CurrentTurnIndex + 1) % ActivePlayerStates.Num();
        SetCurrentTurnPlayer(ActivePlayerStates[CurrentTurnIndex]);
        break;

    case EGolfTurnOrder::FurthestFirst:
        SetCurrentTurnPlayer(GetFurthestPlayer());
        break;
    }
}

AVRGolfPlayerState* AVRGolfGameMode::GetFurthestPlayer()
{
    if (!CurrentHole) return nullptr;

    FVector HoleLocation = CurrentHole->HoleTrigger
        ? CurrentHole->HoleTrigger->GetComponentLocation()
        : CurrentHole->GetActorLocation();

    AVRGolfPlayerState* FurthestPlayer = nullptr;
    float MaxDistance = -1.0f;

    for (AVRGolfPlayerState* PS : ActivePlayerStates)
    {
        if (PlayersCompletedHole.Contains(PS)) continue;

        APlayerController* PC = nullptr;
        for (auto& Pair : ControllerToState)
        {
            if (Pair.Value == PS) { PC = Pair.Key; break; }
        }
        if (!PC) continue;

        AVRGolfBall* Ball = GetPlayerBall(PC);
        if (!Ball) continue;

        float Distance = FVector::Dist(Ball->GetActorLocation(), HoleLocation);
        if (Distance > MaxDistance)
        {
            MaxDistance = Distance;
            FurthestPlayer = PS;
        }
    }

    return FurthestPlayer;
}

void AVRGolfGameMode::SetCurrentTurnPlayer(AVRGolfPlayerState* PlayerState)
{
    if (CurrentTurnPlayerState == PlayerState) return;

    if (CurrentTurnPlayerState)
    {
        CurrentTurnPlayerState->SetIsMyTurn(false);
    }

    CurrentTurnPlayerState = PlayerState;

    if (CurrentTurnPlayerState)
    {
        CurrentTurnPlayerState->SetIsMyTurn(true);
    }

    AVRGolfGameState* GS = GetGameState<AVRGolfGameState>();
    if (GS)
    {
        GS->CurrentTurnPlayer = CurrentTurnPlayerState;
        GS->OnRep_CurrentTurn();
    }

    NotifyTurnChange();
}

void AVRGolfGameMode::NotifyTurnChange()
{
    if (CurrentTurnPlayerState)
    {
        UE_LOG(LogVRGolf, Log, TEXT("It's now %s's turn."), *CurrentTurnPlayerState->GetPlayerName());
    }
}

void AVRGolfGameMode::AdvanceTurn()
{
    DetermineNextPlayer();
}

AVRGolfPlayerState* AVRGolfGameMode::GetCurrentTurnPlayer() const
{
    return CurrentTurnPlayerState;
}

bool AVRGolfGameMode::IsPlayersTurn(AVRGolfPlayerState* PlayerState) const
{
    if (bAllowSimultaneousPlay) return true;
    return CurrentTurnPlayerState == PlayerState;
}

// ---------------------------------------------------------------------------
// Disconnect override — adds turn + hole completion handling
// ---------------------------------------------------------------------------

void AVRGolfGameMode::RemovePlayerFromSession(AVRGolfPlayerState* LeavingPlayerState)
{
    if (!LeavingPlayerState) return;

    bool bWasCurrentTurn = (CurrentTurnPlayerState == LeavingPlayerState);

    // Let base clean up ball and tracking maps
    Super::RemovePlayerFromSession(LeavingPlayerState);

    // Also remove from scoring state
    PlayersCompletedHole.Remove(LeavingPlayerState);

    // Clamp turn index
    if (ActivePlayerStates.Num() > 0)
    {
        CurrentTurnIndex = CurrentTurnIndex % ActivePlayerStates.Num();
    }

    // If it was their turn, advance
    if (bWasCurrentTurn)
    {
        CurrentTurnPlayerState = nullptr;
        AdvanceTurn();
    }

    // If only one player remains, end the round
    if (ActivePlayerStates.Num() == 1)
    {
        UE_LOG(LogVRGolf, Log, TEXT("Only one player remains — ending round."));
        OnRoundComplete();
        return;
    }

    CheckIfHoleComplete();
}

// ---------------------------------------------------------------------------
// Ghost ball override
// ---------------------------------------------------------------------------

bool AVRGolfGameMode::CanPlayerUseGhostBall(APlayerController* PlayerController) const
{
    if (!Super::CanPlayerUseGhostBall(PlayerController)) return false;

    const UVRGolfSettings* Settings = GetGolfSettings();
    if (Settings && ActivePlayerStates.Num() > 1 && !Settings->bAllowGhostBallsInMultiplayer)
    {
        return false;
    }

    return true;
}