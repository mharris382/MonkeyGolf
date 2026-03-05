// VRGolfGameMode.cpp

#include "VRGolfGameMode.h"
#include "VRGolfPlayerState.h"
#include "VRGolfGameState.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "VRGolfBall.h"
#include "VRGolfHole.h"
#include "VRGolfSettings.h"
#include "Logging/LogMacros.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogVRGolf);

AVRGolfGameMode::AVRGolfGameMode()
{
    // Set default classes for competitive play
    PlayerStateClass = AVRGolfPlayerState::StaticClass();
    GameStateClass = AVRGolfGameState::StaticClass();

    CurrentTurnIndex = 0;
    CurrentHoleNumber = 1;
    CurrentPhase = EGamePhase::WaitingToStart;
    CurrentTurnPlayerState = nullptr;
}

void AVRGolfGameMode::BeginPlay()
{
    // Don't call Super::BeginPlay() - we'll manually control hole loading
    // Base class would auto-load hole 1, but we want explicit StartRound call

    UE_LOG(LogTemp, Log, TEXT("VRGolfGameMode initialized - waiting for StartRound"));
}

void AVRGolfGameMode::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Monitor game state
    if (CurrentPhase == EGamePhase::InProgress && bAutoAdvanceTurn)
    {
        CheckIfHoleComplete();
    }
}

void AVRGolfGameMode::StartRound()
{
    if (CurrentPhase != EGamePhase::WaitingToStart)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot start round - game already in progress"));
        return;
    }

    const UVRGolfSettings* Settings = GetGolfSettings();
    int32 TotalHoles = Settings ? Settings->DefaultTotalHoles : 18;

    UE_LOG(LogTemp, Log, TEXT("Starting golf round with %d holes"), TotalHoles);

    InitializePlayers();
    LoadHole(1);

    CurrentPhase = EGamePhase::InProgress;
}

void AVRGolfGameMode::InitializePlayers()
{
    ActivePlayerStates.Empty();
    ControllerToState.Empty();

    // Gather all player states
    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* PC = It->Get();
        if (PC)
        {
            AVRGolfPlayerState* PS = Cast<AVRGolfPlayerState>(PC->PlayerState);
            if (PS)
            {
                ActivePlayerStates.Add(PS);
                ControllerToState.Add(PC, PS);
                UE_LOG(LogTemp, Log, TEXT("Registered player: %s"), *PS->GetPlayerName());
            }
        }
    }

    CurrentTurnIndex = 0;
}

void AVRGolfGameMode::LoadHole(int32 HoleNumber)
{
    UE_LOG(LogTemp, Log, TEXT("Loading Hole %d (Competitive Mode)"), HoleNumber);

    // Call base class to load hole
    Super::LoadHole(HoleNumber);

    if (!CurrentHole)
        return;

    CurrentHoleNumber = HoleNumber;
    PlayersCompletedHole.Empty();

    // Spawn balls for all players
    SpawnPlayerBalls();

    // Update game state
    AVRGolfGameState* GS = GetGameState<AVRGolfGameState>();
    if (GS)
    {
        GS->CurrentHoleNumber = CurrentHoleNumber;
        const UVRGolfSettings* Settings = GetGolfSettings();
        GS->TotalHoles = Settings ? Settings->DefaultTotalHoles : 18;
        GS->OnRep_CurrentHole();
    }

    // Determine first player
    DetermineNextPlayer();
}

void AVRGolfGameMode::SpawnPlayerBalls()
{
    if (!CurrentHole)
        return;

    FVector TeePosition = CurrentHole->GetTeePosition();
    FRotator TeeRotation = CurrentHole->GetTeeRotation();

    // Spawn balls with slight offset for multiplayer
    float Spacing = 10.0f; // 10cm spacing
    int32 PlayerIndex = 0;

    for (AVRGolfPlayerState* PS : ActivePlayerStates)
    {
        // Find controller for this player state
        APlayerController* PC = nullptr;
        for (auto& Pair : ControllerToState)
        {
            if (Pair.Value == PS)
            {
                PC = Pair.Key;
                break;
            }
        }

        if (!PC)
            continue;

        // Calculate offset position
        FVector Offset = FVector(0, PlayerIndex * Spacing, 0);
        FVector SpawnLocation = TeePosition + TeeRotation.RotateVector(Offset);

        // Use base class spawn (it handles the map)
        AVRGolfBall* Ball = SpawnBallForPlayer(PC);
        if (Ball)
        {
            Ball->SetActorLocation(SpawnLocation);
            UE_LOG(LogTemp, Log, TEXT("Spawned ball for player %s at %s"), 
                   *PS->GetPlayerName(), *SpawnLocation.ToString());
        }

        PlayerIndex++;
    }
}

void AVRGolfGameMode::OnPlayerStroke(AVRGolfPlayerState* PlayerState, AVRGolfBall* Ball)
{
    if (!PlayerState || !Ball)
        return;

    // Check if it's this player's turn (if not simultaneous play)
    if (!bAllowSimultaneousPlay && !IsPlayersTurn(PlayerState))
    {
        UE_LOG(LogTemp, Warning, TEXT("Player %s stroked out of turn!"), *PlayerState->GetPlayerName());
        return;
    }

    // Increment stroke count
    PlayerState->AddStroke(CurrentHoleNumber);

    UE_LOG(LogTemp, Log, TEXT("Player %s stroke #%d on hole %d"), 
           *PlayerState->GetPlayerName(), PlayerState->GetCurrentHoleStrokes(), CurrentHoleNumber);
}

void AVRGolfGameMode::OnBallCompletedHole(AVRGolfBall* Ball)
{
    if (!Ball || !CurrentHole)
        return;

    // Find which player owns this ball
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
        UE_LOG(LogTemp, Warning, TEXT("Ball completed hole but no owning player found"));
        return;
    }

    // Mark player as completed
    PlayersCompletedHole.Add(OwningPlayer);

    // Record score
    int32 Strokes = Ball->GetStrokeCount();
    int32 Par = CurrentHole->GetPar();
    OwningPlayer->CompleteHole(CurrentHoleNumber, Strokes, Par);

    UE_LOG(LogTemp, Log, TEXT("Player %s completed hole %d in %d strokes (Par %d)"), 
           *OwningPlayer->GetPlayerName(), CurrentHoleNumber, Strokes, Par);

    // Check if all players done
    CheckIfHoleComplete();
}

void AVRGolfGameMode::CheckIfHoleComplete()
{
    // Check if all active players have completed
    bool bAllPlayersComplete = true;
    for (AVRGolfPlayerState* PS : ActivePlayerStates)
    {
        if (!PlayersCompletedHole.Contains(PS))
        {
            bAllPlayersComplete = false;
            break;
        }
    }

    if (bAllPlayersComplete)
    {
        OnAllPlayersCompleteHole();
    }
}

void AVRGolfGameMode::OnAllPlayersCompleteHole()
{
    UE_LOG(LogTemp, Log, TEXT("All players completed hole %d"), CurrentHoleNumber);

    CurrentPhase = EGamePhase::HoleComplete;

    // Auto-advance after delay
    FTimerHandle TimerHandle;
    GetWorldTimerManager().SetTimer(TimerHandle, [this]()
    {
        SkipToNextHole();
    }, 3.0f, false);
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

void AVRGolfGameMode::OnRoundComplete()
{
    UE_LOG(LogTemp, Log, TEXT("Round complete!"));

    CurrentPhase = EGamePhase::RoundComplete;

    // Show final scoreboard
    AVRGolfGameState* GS = GetGameState<AVRGolfGameState>();
    if (GS)
    {
        TArray<AVRGolfPlayerState*> SortedPlayers = GS->GetSortedPlayersByScore();
        if (SortedPlayers.Num() > 0)
        {
            UE_LOG(LogTemp, Log, TEXT("Winner: %s with score %+d"), 
                   *SortedPlayers[0]->GetPlayerName(), SortedPlayers[0]->GetTotalScore());
        }
    }
}

void AVRGolfGameMode::CleanupHole()
{
    // Destroy all player balls
    for (auto& Pair : PlayerBalls)
    {
        if (Pair.Value)
        {
            Pair.Value->Destroy();
        }
    }
    PlayerBalls.Empty();
}

void AVRGolfGameMode::DetermineNextPlayer()
{
    if (bAllowSimultaneousPlay)
        return; // No turn order in simultaneous play

    switch (TurnOrder)
    {
        case EGolfTurnOrder::Sequential:
        {
            // Simple round-robin
            if (ActivePlayerStates.Num() == 0)
                return;

            CurrentTurnIndex = (CurrentTurnIndex + 1) % ActivePlayerStates.Num();
            SetCurrentTurnPlayer(ActivePlayerStates[CurrentTurnIndex]);
            break;
        }

        case EGolfTurnOrder::FurthestFirst:
        {
            // Player furthest from hole goes first
            AVRGolfPlayerState* Furthest = GetFurthestPlayer();
            SetCurrentTurnPlayer(Furthest);
            break;
        }
    }
}

AVRGolfPlayerState* AVRGolfGameMode::GetFurthestPlayer()
{
    if (!CurrentHole)
        return nullptr;

    FVector HoleLocation = CurrentHole->HoleTrigger ? 
        CurrentHole->HoleTrigger->GetComponentLocation() : 
        CurrentHole->GetActorLocation();

    AVRGolfPlayerState* FurthestPlayer = nullptr;
    float MaxDistance = -1.0f;

    for (AVRGolfPlayerState* PS : ActivePlayerStates)
    {
        // Skip players who've completed the hole
        if (PlayersCompletedHole.Contains(PS))
            continue;

        // Find this player's controller
        APlayerController* PC = nullptr;
        for (auto& Pair : ControllerToState)
        {
            if (Pair.Value == PS)
            {
                PC = Pair.Key;
                break;
            }
        }

        if (!PC)
            continue;

        AVRGolfBall* Ball = GetPlayerBall(PC);
        if (!Ball)
            continue;

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
    if (CurrentTurnPlayerState == PlayerState)
        return;

    // Clear old player's turn
    if (CurrentTurnPlayerState)
    {
        CurrentTurnPlayerState->SetIsMyTurn(false);
    }

    // Set new player's turn
    CurrentTurnPlayerState = PlayerState;
    if (CurrentTurnPlayerState)
    {
        CurrentTurnPlayerState->SetIsMyTurn(true);
    }

    // Update game state
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
    if (!CurrentTurnPlayerState)
        return;

    UE_LOG(LogTemp, Log, TEXT("It's now %s's turn"), *CurrentTurnPlayerState->GetPlayerName());
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
    if (bAllowSimultaneousPlay)
        return true;

    return CurrentTurnPlayerState == PlayerState;
}

bool AVRGolfGameMode::CanPlayerUseGhostBall(APlayerController* PlayerController) const
{
    // Call base class first
    if (!Super::CanPlayerUseGhostBall(PlayerController))
        return false;

    // Check multiplayer restrictions
    const UVRGolfSettings* Settings = GetGolfSettings();
    if (Settings && ActivePlayerStates.Num() > 1 && !Settings->bAllowGhostBallsInMultiplayer)
    {
        return false;
    }

    return true;
}

AVRGolfPlayerState* AVRGolfGameMode::GetPlayerStateForController(APlayerController* Controller) const
{
    if (!Controller)
        return nullptr;

    AVRGolfPlayerState* const* StatePtr = ControllerToState.Find(Controller);
    return StatePtr ? *StatePtr : nullptr;
}
