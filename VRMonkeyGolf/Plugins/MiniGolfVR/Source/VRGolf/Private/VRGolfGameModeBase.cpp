// VRGolfGameModeBase.cpp

#include "VRGolfGameModeBase.h"
#include "VRGolfBall.h"
#include "VRGolfGhostBall.h"
#include "VRGolfHole.h"
#include "VRGolfSettings.h"
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

    // Load first hole by default
    LoadHole(1);
}

void AVRGolfGameModeBase::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Base class doesn't need tick, but derived classes might
}

void AVRGolfGameModeBase::LoadHole(int32 HoleNumber)
{
    UE_LOG(LogTemp, Log, TEXT("Loading hole %d"), HoleNumber);

    // Find hole in level
    CurrentHole = FindHoleInLevel(HoleNumber);

    if (!CurrentHole)
    {
        UE_LOG(LogTemp, Error, TEXT("Could not find hole %d in level!"), HoleNumber);
        return;
    }

    // Clear ghost ball usage tracking for new hole
    GhostBallUsageThisHole.Empty();

    UE_LOG(LogTemp, Log, TEXT("Loaded hole: %s (Par %d)"), 
           *CurrentHole->HoleConfig.HoleName, CurrentHole->GetPar());
}

AVRGolfHole* AVRGolfGameModeBase::FindHoleInLevel(int32 HoleNumber) const
{
    // Find hole by tag: "Hole1", "Hole2", etc.
    FString HoleTag = FString::Printf(TEXT("Hole%d"), HoleNumber);

    TArray<AActor*> FoundHoles;
    UGameplayStatics::GetAllActorsOfClassWithTag(
        GetWorld(), 
        AVRGolfHole::StaticClass(), 
        FName(*HoleTag), 
        FoundHoles
    );

    if (FoundHoles.Num() > 0)
    {
        return Cast<AVRGolfHole>(FoundHoles[0]);
    }

    // Fallback: find any hole with matching hole number
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

AVRGolfBall* AVRGolfGameModeBase::SpawnBallForPlayer(APlayerController* PlayerController)
{
    if (!PlayerController || !CurrentHole || !BallClass)
        return nullptr;

    // Check if player already has a ball
    if (AVRGolfBall** ExistingBall = PlayerBalls.Find(PlayerController))
    {
        if (*ExistingBall && IsValid(*ExistingBall))
        {
            UE_LOG(LogTemp, Warning, TEXT("Player already has a ball"));
            return *ExistingBall;
        }
    }

    // Spawn ball at tee
    FVector TeePosition = CurrentHole->GetTeePosition();
    FRotator TeeRotation = CurrentHole->GetTeeRotation();

    AVRGolfBall* Ball = GetWorld()->SpawnActor<AVRGolfBall>(
        BallClass,
        TeePosition,
        TeeRotation
    );

    if (Ball)
    {
        PlayerBalls.Add(PlayerController, Ball);
        UE_LOG(LogTemp, Log, TEXT("Spawned ball for player at %s"), *TeePosition.ToString());
    }

    return Ball;
}

AVRGolfBall* AVRGolfGameModeBase::GetPlayerBall(APlayerController* PlayerController) const
{
    if (!PlayerController)
        return nullptr;

    AVRGolfBall* const* BallPtr = PlayerBalls.Find(PlayerController);
    return BallPtr ? *BallPtr : nullptr;
}

void AVRGolfGameModeBase::OnBallCompletedHole(AVRGolfBall* Ball)
{
    if (!Ball)
        return;

    UE_LOG(LogTemp, Log, TEXT("Ball completed hole in %d strokes"), Ball->GetStrokeCount());

    // Base class just logs - derived class (AVRGolfGameMode) handles scoring
}

bool AVRGolfGameModeBase::CanPlayerUseGhostBall(APlayerController* PlayerController) const
{
    if (!PlayerController)
        return false;

    const UVRGolfSettings* Settings = GetGolfSettings();
    if (!Settings)
        return false;

    // Check if ghost balls are enabled
    if (!Settings->bAllowGhostBalls)
        return false;

    // Check usage limit
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
    if (!SourceBall || !GhostBallClass)
        return nullptr;

    // Spawn ghost ball at source ball's position
    FVector SpawnLocation = SourceBall->GetActorLocation();
    FRotator SpawnRotation = SourceBall->GetActorRotation();

    AVRGolfGhostBall* GhostBall = GetWorld()->SpawnActor<AVRGolfGhostBall>(
        GhostBallClass,
        SpawnLocation,
        SpawnRotation
    );

    if (GhostBall)
    {
        // Apply settings from project settings
        const UVRGolfSettings* Settings = GetGolfSettings();
        if (Settings)
        {
            GhostBall->MaxLifetime = Settings->GhostBallMaxLifetime;
            GhostBall->FadeStartTime = Settings->GhostBallFadeStartTime;
        }

        UE_LOG(LogTemp, Log, TEXT("Ghost ball spawned"));
    }

    return GhostBall;
}

void AVRGolfGameModeBase::OnPlayerUseGhostBall(APlayerController* PlayerController)
{
    if (!PlayerController)
        return;

    if (!GhostBallUsageThisHole.Contains(PlayerController))
    {
        GhostBallUsageThisHole.Add(PlayerController, 0);
    }

    GhostBallUsageThisHole[PlayerController]++;

    const UVRGolfSettings* Settings = GetGolfSettings();
    if (Settings && Settings->MaxGhostBallsPerHole > 0)
    {
        int32 Remaining = Settings->MaxGhostBallsPerHole - GhostBallUsageThisHole[PlayerController];
        UE_LOG(LogTemp, Log, TEXT("Ghost balls remaining: %d"), Remaining);
    }
}

void AVRGolfGameModeBase::TeleportPlayerToBall(APlayerController* PlayerController)
{
    if (!PlayerController)
        return;

    AVRGolfBall* Ball = GetPlayerBall(PlayerController);
    if (!Ball)
        return;

    FVector TargetLocation = GetTargetLocationForBall(Ball);
    FTransform TeleportTransform = CalculateTeleportTransform(Ball, TargetLocation);

    APawn* PlayerPawn = PlayerController->GetPawn();
    if (PlayerPawn)
    {
        PlayerPawn->SetActorTransform(TeleportTransform);
        UE_LOG(LogTemp, Log, TEXT("Teleported player to ball at %s"), 
               *TeleportTransform.GetLocation().ToString());
    }
}

FVector AVRGolfGameModeBase::GetTargetLocationForBall(AVRGolfBall* Ball) const
{
    if (!Ball || !CurrentHole)
        return FVector::ZeroVector;

    FVector BallLocation = Ball->GetActorLocation();

    // Check if ball is on a surface with a custom target override
    TArray<FHitResult> Hits;
    FVector Start = BallLocation + FVector(0, 0, 10);
    FVector End = BallLocation - FVector(0, 0, 50);

    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(Ball);

    GetWorld()->LineTraceMultiByChannel(Hits, Start, End, ECC_WorldStatic, QueryParams);

    for (const FHitResult& Hit : Hits)
    {
        if (Hit.Component.IsValid())
        {
            // Use hole's target lookup system
            FVector Target = CurrentHole->GetTargetForSurface(Hit.Component.Get());
            if (!Target.IsZero())
            {
                return Target;
            }
        }
    }

    // Default: use hole location
    return CurrentHole->HoleTrigger ? 
        CurrentHole->HoleTrigger->GetComponentLocation() : 
        CurrentHole->GetActorLocation();
}

FTransform AVRGolfGameModeBase::CalculateTeleportTransform(AVRGolfBall* Ball, const FVector& TargetLocation) const
{
    if (!Ball)
        return FTransform::Identity;

    FVector BallLocation = Ball->GetActorLocation();

    // Calculate direction from ball to target
    FVector DirectionToTarget = (TargetLocation - BallLocation).GetSafeNormal2D();
    FRotator LookAtRotation = DirectionToTarget.Rotation();

    // Get teleport distance from settings
    const UVRGolfSettings* Settings = GetGolfSettings();
    float DistanceBehindBall = Settings ? Settings->TeleportDistanceBehindBall : 100.0f;

    // Position player behind the ball
    FVector TeleportLocation = BallLocation - (DirectionToTarget * DistanceBehindBall);

    // Keep at ground level (VR headset handles height offset)
    TeleportLocation.Z = BallLocation.Z;

    return FTransform(LookAtRotation, TeleportLocation, FVector::OneVector);
}

const UVRGolfSettings* AVRGolfGameModeBase::GetGolfSettings() const
{
    return GetDefault<UVRGolfSettings>();
}
