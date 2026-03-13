// VRGolfGameModeBase.cpp

#include "VRGolfGameModeBase.h"
#include "VRGolfBall.h"
#include "VRGolfGhostBall.h"
#include "VRGolfHole.h"
#include "VRGolfSettings.h"
#include "VRGolfPlayerState.h"
#include "VRGolfOnlineSubsystem.h"
#include "MGProfileSubsystem.h"
#include "MGProfileSaveGame.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AVRGolfGameModeBase::AVRGolfGameModeBase()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentHole = nullptr;
}

void AVRGolfGameModeBase::BeginPlay()
{
    Super::BeginPlay();
    LoadHole(1);
}

void AVRGolfGameModeBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// ---------------------------------------------------------------------------
// Network / Session
// ---------------------------------------------------------------------------

void AVRGolfGameModeBase::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!NewPlayer) return;

    AVRGolfPlayerState* PS = Cast<AVRGolfPlayerState>(NewPlayer->PlayerState);
    if (!PS) return;

    // Register in tracking maps
    ControllerToState.Add(NewPlayer, PS);
    if (!ActivePlayerStates.Contains(PS))
    {
        ActivePlayerStates.Add(PS);
    }

    // Wire profile identity from save subsystem into PlayerState
    // so scoreboard and room save system have a stable identifier
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UMGProfileSubsystem* SaveSys = GI->GetSubsystem<UMGProfileSubsystem>())
        {
            if (UMGProfileSaveGame* Profile = SaveSys->GetActiveProfile())
            {
                PS->SetProfileIdentity(Profile->ProfileName, Profile->ProfileID);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("VRGolfGameModeBase: Player joined — %s"), *PS->GetPlayerName());

    // Notify online subsystem so UI player list updates
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UVRGolfOnlineSubsystem* Online = GI->GetSubsystem<UVRGolfOnlineSubsystem>())
        {
            Online->OnPlayerJoined.Broadcast(PS->GetPlayerName());
        }
    }

    // If round is already underway, spawn a ball for late joiners
    // (only reachable if SetAllowLateJoin(true) was called — off by default once round starts)
    if (bRoundStarted && CurrentHole)
    {
        SpawnBallForPlayer(NewPlayer);
    }
}

void AVRGolfGameModeBase::NotifyPlayerDisconnected(APlayerController* ExitingPlayer)
{
    if (!ExitingPlayer) return;

    AVRGolfPlayerState* PS = GetPlayerStateForController(ExitingPlayer);
    if (!PS) return;

    UE_LOG(LogTemp, Log, TEXT("VRGolfGameModeBase: Player disconnected — %s"), *PS->GetPlayerName());

    // Notify online subsystem for UI
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UVRGolfOnlineSubsystem* Online = GI->GetSubsystem<UVRGolfOnlineSubsystem>())
        {
            Online->OnPlayerLeft.Broadcast(PS->GetPlayerName());
        }
    }

    RemovePlayerFromSession(PS);
}

void AVRGolfGameModeBase::RemovePlayerFromSession(AVRGolfPlayerState* LeavingPlayerState)
{
    if (!LeavingPlayerState) return;

    // Find and clean up their controller references
    APlayerController* LeavingPC = nullptr;
    for (auto& Pair : ControllerToState)
    {
        if (Pair.Value == LeavingPlayerState)
        {
            LeavingPC = Pair.Key;
            break;
        }
    }

    if (LeavingPC)
    {
        // Destroy their ball
        if (AVRGolfBall** Ball = PlayerBalls.Find(LeavingPC))
        {
            if (*Ball && IsValid(*Ball))
            {
                (*Ball)->Destroy();
            }
        }
        PlayerBalls.Remove(LeavingPC);
        ControllerToState.Remove(LeavingPC);
    }

    ActivePlayerStates.Remove(LeavingPlayerState);

    // Derived classes (AVRGolfGameMode) override this to also handle
    // turn advancement and hole completion checks
}

void AVRGolfGameModeBase::HandleHostMigration()
{
    UE_LOG(LogTemp, Log, TEXT("VRGolfGameModeBase: Host migration complete — this machine is now host."));

    if (UGameInstance* GI = GetGameInstance())
    {
        if (UVRGolfOnlineSubsystem* Online = GI->GetSubsystem<UVRGolfOnlineSubsystem>())
        {
            Online->OnHostMigrated.Broadcast();
        }
    }
}

void AVRGolfGameModeBase::SetAllowLateJoin(bool bAllow)
{
    IOnlineSubsystem* OSS = IOnlineSubsystem::Get(TEXT("EOS"));
    if (!OSS) return;

    IOnlineSessionPtr Sessions = OSS->GetSessionInterface();
    if (!Sessions.IsValid()) return;

    FOnlineSessionSettings UpdatedSettings;
    UpdatedSettings.bAllowJoinInProgress = bAllow;
    Sessions->UpdateSession(NAME_GameSession, UpdatedSettings, true);

    UE_LOG(LogTemp, Log, TEXT("VRGolfGameModeBase: Late join %s."),
        bAllow ? TEXT("enabled") : TEXT("disabled"));
}

// ---------------------------------------------------------------------------
// Hole management
// ---------------------------------------------------------------------------

void AVRGolfGameModeBase::LoadHole(int32 HoleNumber)
{
    UE_LOG(LogTemp, Log, TEXT("Loading hole %d"), HoleNumber);

    CurrentHole = FindHoleInLevel(HoleNumber);

    if (!CurrentHole)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find hole %d in level!"), HoleNumber);
        return;
    }

    GhostBallUsageThisHole.Empty();

    UE_LOG(LogTemp, Log, TEXT("Loaded hole: %s (Par %d)"),
        *CurrentHole->HoleConfig.HoleName, CurrentHole->GetPar());
}

AVRGolfHole* AVRGolfGameModeBase::FindHoleInLevel(int32 HoleNumber) const
{
    FString HoleTag = FString::Printf(TEXT("Hole%d"), HoleNumber);

    TArray<AActor*> FoundHoles;
    UGameplayStatics::GetAllActorsOfClassWithTag(
        GetWorld(), AVRGolfHole::StaticClass(), FName(*HoleTag), FoundHoles);

    if (FoundHoles.Num() > 0)
    {
        return Cast<AVRGolfHole>(FoundHoles[0]);
    }

    // Fallback: match by hole number property
    TArray<AActor*> AllHoles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVRGolfHole::StaticClass(), AllHoles);

    for (AActor* Actor : AllHoles)
    {
        AVRGolfHole* Hole = Cast<AVRGolfHole>(Actor);
        if (Hole && Hole->HoleConfig.HoleNumber == HoleNumber)
        {
            return Hole;
        }
    }

    return nullptr;
}

// ---------------------------------------------------------------------------
// Ball management
// ---------------------------------------------------------------------------

AVRGolfBall* AVRGolfGameModeBase::SpawnBallForPlayer(APlayerController* PlayerController)
{
    if (!PlayerController || !CurrentHole || !BallClass)
        return nullptr;

    if (AVRGolfBall** ExistingBall = PlayerBalls.Find(PlayerController))
    {
        if (*ExistingBall && IsValid(*ExistingBall))
        {
            UE_LOG(LogTemp, Warning, TEXT("Player already has a ball"));
            return *ExistingBall;
        }
    }

    FVector TeePosition = CurrentHole->GetTeePosition();
    FRotator TeeRotation = CurrentHole->GetTeeRotation();

    AVRGolfBall* Ball = GetWorld()->SpawnActor<AVRGolfBall>(BallClass, TeePosition, TeeRotation);
    if (Ball)
    {
        PlayerBalls.Add(PlayerController, Ball);
        UE_LOG(LogTemp, Log, TEXT("Spawned ball for player at %s"), *TeePosition.ToString());
    }

    return Ball;
}

AVRGolfBall* AVRGolfGameModeBase::GetPlayerBall(APlayerController* PlayerController) const
{
    if (!PlayerController) return nullptr;
    AVRGolfBall* const* BallPtr = PlayerBalls.Find(PlayerController);
    return BallPtr ? *BallPtr : nullptr;
}

void AVRGolfGameModeBase::OnBallCompletedHole(AVRGolfBall* Ball)
{
    if (!Ball) return;
    UE_LOG(LogTemp, Log, TEXT("Ball completed hole in %d strokes"), Ball->GetStrokeCount());
    // Derived class (AVRGolfGameMode) handles scoring
}

// ---------------------------------------------------------------------------
// Ghost ball system
// ---------------------------------------------------------------------------

bool AVRGolfGameModeBase::CanPlayerUseGhostBall(APlayerController* PlayerController) const
{
    if (!PlayerController) return false;

    const UVRGolfSettings* Settings = GetGolfSettings();
    if (!Settings || !Settings->bAllowGhostBalls) return false;

    if (Settings->MaxGhostBallsPerHole > 0)
    {
        const int32* UsageCount = GhostBallUsageThisHole.Find(PlayerController);
        if (UsageCount && *UsageCount >= Settings->MaxGhostBallsPerHole)
        {
            return false;
        }
    }

    return true;
}

AVRGolfGhostBall* AVRGolfGameModeBase::SpawnGhostBall(AVRGolfBall* SourceBall)
{
    if (!SourceBall || !GhostBallClass) return nullptr;

    AVRGolfGhostBall* GhostBall = GetWorld()->SpawnActor<AVRGolfGhostBall>(
        GhostBallClass, SourceBall->GetActorLocation(), SourceBall->GetActorRotation());

    if (GhostBall)
    {
        const UVRGolfSettings* Settings = GetGolfSettings();
        if (Settings)
        {
            GhostBall->MaxLifetime = Settings->GhostBallMaxLifetime;
            GhostBall->FadeStartTime = Settings->GhostBallFadeStartTime;
        }
    }

    return GhostBall;
}

void AVRGolfGameModeBase::OnPlayerUseGhostBall(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    int32& Usage = GhostBallUsageThisHole.FindOrAdd(PlayerController);
    Usage++;

    const UVRGolfSettings* Settings = GetGolfSettings();
    if (Settings && Settings->MaxGhostBallsPerHole > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Ghost balls remaining: %d"),
            Settings->MaxGhostBallsPerHole - Usage);
    }
}

// ---------------------------------------------------------------------------
// Teleport system
// ---------------------------------------------------------------------------

void AVRGolfGameModeBase::TeleportPlayerToBall(APlayerController* PlayerController)
{
    if (!PlayerController) return;

    AVRGolfBall* Ball = GetPlayerBall(PlayerController);
    if (!Ball) return;

    FVector TargetLocation = GetTargetLocationForBall(Ball);
    FTransform TeleportTransform = CalculateTeleportTransform(Ball, TargetLocation);

    if (APawn* PlayerPawn = PlayerController->GetPawn())
    {
        PlayerPawn->SetActorTransform(TeleportTransform);
    }
}

FVector AVRGolfGameModeBase::GetTargetLocationForBall(AVRGolfBall* Ball) const
{
    if (!Ball || !CurrentHole) return FVector::ZeroVector;

    FVector BallLocation = Ball->GetActorLocation();
    TArray<FHitResult> Hits;

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Ball);

    GetWorld()->LineTraceMultiByChannel(
        Hits,
        BallLocation + FVector(0, 0, 10),
        BallLocation - FVector(0, 0, 50),
        ECC_WorldStatic,
        QueryParams);

    for (const FHitResult& Hit : Hits)
    {
        if (Hit.Component.IsValid())
        {
            FVector Target = CurrentHole->GetTargetForSurface(Hit.Component.Get());
            if (!Target.IsZero()) return Target;
        }
    }

    return CurrentHole->HoleTrigger
        ? CurrentHole->HoleTrigger->GetComponentLocation()
        : CurrentHole->GetActorLocation();
}

FTransform AVRGolfGameModeBase::CalculateTeleportTransform(AVRGolfBall* Ball,
    const FVector& TargetLocation) const
{
    if (!Ball) return FTransform::Identity;

    FVector BallLocation = Ball->GetActorLocation();
    FVector DirectionToTarget = (TargetLocation - BallLocation).GetSafeNormal2D();
    FRotator LookAtRotation = DirectionToTarget.Rotation();

    const UVRGolfSettings* Settings = GetGolfSettings();
    float DistanceBehind = Settings ? Settings->TeleportDistanceBehindBall : 100.0f;

    FVector TeleportLocation = BallLocation - (DirectionToTarget * DistanceBehind);
    TeleportLocation.Z = BallLocation.Z;

    return FTransform(LookAtRotation, TeleportLocation, FVector::OneVector);
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

AVRGolfPlayerState* AVRGolfGameModeBase::GetPlayerStateForController(
    APlayerController* Controller) const
{
    if (!Controller) return nullptr;
    AVRGolfPlayerState* const* StatePtr = ControllerToState.Find(Controller);
    return StatePtr ? *StatePtr : nullptr;
}

const UVRGolfSettings* AVRGolfGameModeBase::GetGolfSettings() const
{
    return GetDefault<UVRGolfSettings>();
}