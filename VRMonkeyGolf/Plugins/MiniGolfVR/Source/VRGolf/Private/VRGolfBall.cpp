// VRGolfBall.cpp

#include "VRGolfBall.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"
#include "Engine/OverlapResult.h"
#include "GolfSurfaceInterface.h"

AVRGolfBall::AVRGolfBall()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    SetReplicateMovement(true);

    // Create collision sphere (root component)
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    RootComponent = CollisionSphere;
    CollisionSphere->SetSphereRadius(2.135f); // Standard golf ball radius in cm
    CollisionSphere->SetSimulatePhysics(true);
    CollisionSphere->SetEnableGravity(true);
    CollisionSphere->SetLinearDamping(0.2f);
    CollisionSphere->SetAngularDamping(0.3f);
    CollisionSphere->SetMassOverrideInKg(NAME_None, 0.04593f); // Standard golf ball mass in kg
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionResponseToAllChannels(ECR_Block);
    CollisionSphere->SetNotifyRigidBodyCollision(true);

    // Create visual mesh
    BallMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BallMesh"));
    BallMesh->SetupAttachment(CollisionSphere);
    BallMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BallMesh->SetRelativeScale3D(FVector(4.27f)); // Visual scale to match 2.135cm radius

    // Initialize state
    CurrentState = EBallState::Idle;
    bIsOnPuttableSurface = true;
    CurrentStrokes = 0;
    TimeInAir = 0.0f;
    TimeSinceLastMovement = 0.0f;
    StuckCheckTimer = 0.0f;
    LastPositionCheck = FVector::ZeroVector;
    bDisableAirborneTimeout = false;
}

void AVRGolfBall::SetBallStateBP(EBallState NewState, bool forceSendEvents)
{
	SetBallState(NewState, forceSendEvents);
}

void AVRGolfBall::BeginPlay()
{
    Super::BeginPlay();
	
    // Bind collision events
    CollisionSphere->OnComponentHit.AddDynamic(this, &AVRGolfBall::OnBallHit);
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AVRGolfBall::OnBallBeginOverlap);

    // Store initial position as last valid position
    LastValidPosition = GetActorLocation();
    LastPositionCheck = GetActorLocation();
}

void AVRGolfBall::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AVRGolfBall, CurrentState);
    DOREPLIFETIME(AVRGolfBall, bIsOnPuttableSurface);
    DOREPLIFETIME(AVRGolfBall, LastValidPosition);
    DOREPLIFETIME(AVRGolfBall, CurrentStrokes);
}

void AVRGolfBall::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Only run logic on server
    if (!HasAuthority())
        return;

    UpdateBallState(DeltaTime);
}

void AVRGolfBall::UpdateBallState(float DeltaTime)
{
    switch (CurrentState)
    {
        case EBallState::Idle:
            // Ball is waiting for stroke, nothing to update
            break;

        case EBallState::InMotion:
            CheckResetConditions(DeltaTime);
            CheckIfStopped(DeltaTime);
            break;

        case EBallState::Completed:
            // Ball is in hole, nothing to update
            break;

        case EBallState::Resetting:
            // Resetting is handled by ResetToPosition, nothing to update
            break;
    }
}

void AVRGolfBall::CheckResetConditions(float DeltaTime)
{
    CheckAirborneTime(DeltaTime);
    CheckIfStuck(DeltaTime);
}

void AVRGolfBall::CheckIfStopped(float DeltaTime)
{
    if (!CollisionSphere)
        return;

    FVector LinearVelocity = CollisionSphere->GetPhysicsLinearVelocity();
    FVector AngularVelocity = CollisionSphere->GetPhysicsAngularVelocityInRadians();

    float LinearSpeed = LinearVelocity.Size();
    float AngularSpeed = AngularVelocity.Size();

    // Check if ball has stopped moving
    bool bHasStopped = (LinearSpeed < StoppingVelocityThreshold && 
                        AngularSpeed < StoppingAngularVelocityThreshold);

    if (bHasStopped)
    {
        // Ball stopped - check if it's on a valid surface
        if (bIsOnPuttableSurface)
        {
            // Valid stop - transition to Idle for next stroke
            CurrentState = EBallState::Idle;
            LastValidPosition = GetActorLocation();

            // Zero out any remaining velocity
            CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
            CollisionSphere->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);

            UE_LOG(LogTemp, Log, TEXT("Ball stopped on valid surface at %s"), *GetActorLocation().ToString());
        }
        else
        {
            // Stopped on invalid surface - reset
            ResetToPosition(LastValidPosition, EBallResetReason::InvalidSurface);
        }
    }
}

void AVRGolfBall::CheckAirborneTime(float DeltaTime)
{
    if (!CollisionSphere)
        return;

    // Check if ball is airborne (not touching ground)
    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(this);

    bool bIsGrounded = GetWorld()->OverlapMultiByChannel(
        Overlaps,
        GetActorLocation(),
        FQuat::Identity,
        ECC_WorldStatic,
        FCollisionShape::MakeSphere(CollisionSphere->GetScaledSphereRadius() + 0.1f),
        QueryParams
    );

    if (!bIsGrounded)
    {
        TimeInAir += DeltaTime;

        // Check if exceeded max airborne time (unless disabled for trick shots)
        if (!bDisableAirborneTimeout && TimeInAir > MaxAirborneTime)
        {
            ResetToPosition(LastValidPosition, EBallResetReason::TooLongInAir);
        }
    }
    else
    {
        // Reset airborne timer when grounded
        TimeInAir = 0.0f;
    }
}

void AVRGolfBall::CheckIfStuck(float DeltaTime)
{
    StuckCheckTimer += DeltaTime;

    if (StuckCheckTimer >= StuckCheckInterval)
    {
        FVector CurrentPosition = GetActorLocation();
        float DistanceMoved = FVector::Dist(CurrentPosition, LastPositionCheck);

        // If ball hasn't moved much but is still "in motion"
        if (DistanceMoved < StuckMovementThreshold)
        {
            FVector Velocity = CollisionSphere->GetPhysicsLinearVelocity();
            float Speed = Velocity.Size();

            // If it has non-zero velocity but isn't actually moving, it's stuck
            if (Speed > StoppingVelocityThreshold)
            {
                UE_LOG(LogTemp, Warning, TEXT("Ball is stuck! Velocity: %.2f, Distance moved: %.2f"), 
                       Speed, DistanceMoved);
                ResetToPosition(LastValidPosition, EBallResetReason::Stuck);
            }
        }

        // Update for next check
        LastPositionCheck = CurrentPosition;
        StuckCheckTimer = 0.0f;
    }
}

bool AVRGolfBall::HasStopped() const
{
    if (!CollisionSphere)
        return true;

    FVector LinearVelocity = CollisionSphere->GetPhysicsLinearVelocity();
    FVector AngularVelocity = CollisionSphere->GetPhysicsAngularVelocityInRadians();

    return (LinearVelocity.Size() < StoppingVelocityThreshold && 
            AngularVelocity.Size() < StoppingAngularVelocityThreshold);
}

bool AVRGolfBall::IsValidForStroke() const
{
    return CurrentState == EBallState::Idle && HasStopped();
}

void AVRGolfBall::ApplyStroke(const FVector& ImpulseDirection, float ImpulseMagnitude)
{
    if (!HasAuthority())
    {
        Server_ApplyStroke(ImpulseDirection, ImpulseMagnitude);
        return;
    }

    // Validate stroke is allowed
    if (!IsValidForStroke())
    {
        UE_LOG(LogTemp, Warning, TEXT("Stroke attempted but ball not valid for stroke. State: %d"), 
               (int32)CurrentState);
        return;
    }

    // Apply impulse
    FVector NormalizedDirection = ImpulseDirection.GetSafeNormal();
    FVector Impulse = NormalizedDirection * ImpulseMagnitude;

    if (CollisionSphere)
    {
        CollisionSphere->AddImpulse(Impulse, NAME_None, true);
    }

    // Update state
    CurrentState = EBallState::InMotion;
    CurrentStrokes++;
    TimeInAir = 0.0f;
    TimeSinceLastMovement = 0.0f;
    StuckCheckTimer = 0.0f;
    LastPositionCheck = GetActorLocation();

    UE_LOG(LogTemp, Log, TEXT("Stroke applied! Impulse: %s, Magnitude: %.2f, Strokes: %d"), 
           *Impulse.ToString(), ImpulseMagnitude, CurrentStrokes);
}

void AVRGolfBall::Server_ApplyStroke_Implementation(const FVector& ImpulseDirection, float ImpulseMagnitude)
{
    ApplyStroke(ImpulseDirection, ImpulseMagnitude);
}

void AVRGolfBall::ResetToPosition(const FVector& Position, EBallResetReason Reason)
{
    if (!HasAuthority())
    {
        Server_ResetBall(Position, Reason);
        return;
    }

    UE_LOG(LogTemp, Warning, TEXT("Ball reset! Reason: %d, Position: %s"), 
           (int32)Reason, *Position.ToString());

    // Set position
    SetActorLocation(Position);

    // Zero out physics
    if (CollisionSphere)
    {
        CollisionSphere->SetPhysicsLinearVelocity(FVector::ZeroVector);
        CollisionSphere->SetPhysicsAngularVelocityInRadians(FVector::ZeroVector);
    }

    // Reset state
    CurrentState = EBallState::Idle;
    TimeInAir = 0.0f;
    TimeSinceLastMovement = 0.0f;
    StuckCheckTimer = 0.0f;
    LastPositionCheck = Position;
    bIsOnPuttableSurface = true;

    // Notify clients
    Multicast_OnBallReset(Reason);
}

void AVRGolfBall::Server_ResetBall_Implementation(const FVector& Position, EBallResetReason Reason)
{
    ResetToPosition(Position, Reason);
}

void AVRGolfBall::Multicast_OnBallReset_Implementation(EBallResetReason Reason)
{
    // Play reset VFX/SFX here
    UE_LOG(LogTemp, Log, TEXT("Ball reset broadcast to clients. Reason: %d"), (int32)Reason);
}

void AVRGolfBall::Multicast_OnBallCompleted_Implementation()
{
    // Play completion VFX/SFX here
    UE_LOG(LogTemp, Log, TEXT("Ball completed broadcast to clients."));
}

void AVRGolfBall::OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, 
                            UPrimitiveComponent* OtherComp, FVector NormalImpulse, 
                            const FHitResult& Hit)
{
    if (!HasAuthority())
        return;

    // Check if we hit a surface that implements the golf interface
    if (OtherActor && OtherActor->GetClass()->ImplementsInterface(UGolfSurfaceInterface::StaticClass()))
    {
        bIsOnPuttableSurface = IGolfSurfaceInterface::Execute_IsPuttableSurface(OtherActor);
    }
    else
    {
        // Default: assume static geometry is puttable unless marked otherwise
        bIsOnPuttableSurface = !OtherActor || !OtherActor->ActorHasTag(TEXT("NonPuttable"));
    }
}

void AVRGolfBall::OnBallBeginOverlap(UPrimitiveComponent* OverlappedComponent, 
                                      AActor* OtherActor, UPrimitiveComponent* OtherComp, 
                                      int32 OtherBodyIndex, bool bFromSweep, 
                                      const FHitResult& SweepResult)
{
    if (!HasAuthority())
        return;

    if (!OtherActor)
        return;

    // Check for hole trigger (completion)
    if (OtherActor->ActorHasTag(TEXT("HoleTrigger")))
    {
        CurrentState = EBallState::Completed;
        Multicast_OnBallCompleted();
        
        UE_LOG(LogTemp, Log, TEXT("Ball completed hole in %d strokes!"), CurrentStrokes);
        
        // Notify game mode will be handled externally
        return;
    }

    // Check for out of bounds trigger
    if (OtherActor->ActorHasTag(TEXT("OutOfBounds")))
    {
        ResetToPosition(LastValidPosition, EBallResetReason::OutOfBounds);
        return;
    }

    // Check for destroy trigger (hazards like water, lava, etc.)
    if (OtherActor->ActorHasTag(TEXT("DestroyTrigger")))
    {
        ResetToPosition(LastValidPosition, EBallResetReason::DestroyedByFeature);
        return;
    }
}

void AVRGolfBall::OnRep_BallState()
{
    // Client-side state change handling
    switch (CurrentState)
    {
        case EBallState::Idle:
            // Visual feedback that ball is ready
            break;

        case EBallState::InMotion:
            // Start motion trail VFX
            break;

        case EBallState::Completed:
            // Play celebration VFX
            break;

        case EBallState::Resetting:
            // Play reset VFX
            break;
    }
}
