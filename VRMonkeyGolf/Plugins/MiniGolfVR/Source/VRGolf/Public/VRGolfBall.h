// VRGolfBall.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h"
#include "Delegates/DelegateCombinations.h"
#include "VRGolfBall.generated.h"

UENUM(BlueprintType)
enum class EBallResetReason : uint8
{
    None,
    OutOfBounds,
    DestroyedByFeature,
    InvalidSurface,
    Stuck,
    TooLongInAir
};

UENUM(BlueprintType)
enum class EBallState : uint8
{
    Idle,           // Waiting for stroke
    InMotion,       // Moving after stroke
    Completed,      // In the hole
    Resetting       // Being reset to last valid position
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnGolfBallReset, class AVRGolfBall*, BallActor, EBallResetReason, ResetReason);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnBallStopped, class AVRGolfBall*, BallActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallStruck, class AVRGolfBall*, BallActor, FVector, HitForce);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnBallBounced, class AVRGolfBall*, BallActor, const FHitResult&, HitResult);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBallHoled, class AVRGolfBall*, BallActor, int32, strokes, const AActor*, HoleActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnBallStateChanged, class AVRGolfBall*, BallActor, EBallState, NewState, EBallState, PreviousState);



/**
 * VR Golf Ball - Main gameplay actor with physics simulation
 * Implements all Walkabout Mini Golf ball rules:
 * - Valid stop detection (on puttable surface + velocity threshold)
 * - Reset conditions (out of bounds, destroyed, invalid surface, stuck, too long airborne)
 * - Server-authoritative stroke validation
 * - Replicated state for multiplayer
 */
UCLASS()
class VRGOLF_API AVRGolfBall : public AActor
{
    GENERATED_BODY()

public:
    AVRGolfBall();

    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // Ball control
    UFUNCTION(BlueprintCallable, Category = "Golf")
    virtual void ApplyStroke(const FVector& ImpulseDirection, float ImpulseMagnitude);

    UFUNCTION(BlueprintCallable, Category = "Golf")
    void ResetToPosition(const FVector& Position, EBallResetReason Reason);

    UFUNCTION(BlueprintCallable, Category = "Golf")
    bool IsValidForStroke() const;

    // State queries
    UFUNCTION(BlueprintPure, Category = "Golf")
    EBallState GetBallState() const { return CurrentState; }

    UFUNCTION(BlueprintPure, Category = "Golf")
    bool IsOnPuttableSurface() const { return bIsOnPuttableSurface; }

    UFUNCTION(BlueprintPure, Category = "Golf")
    bool HasStopped() const;

    UFUNCTION(BlueprintPure, Category = "Golf")
    int32 GetStrokeCount() const { return CurrentStrokes; }


	UFUNCTION(BlueprintCallable, Category = "Golf")
	void SetBallStateBP(EBallState NewState, bool forceSendEvents = false);
    

protected:
    virtual void BeginPlay() override;

    // Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class UStaticMeshComponent* BallMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    class USphereComponent* CollisionSphere;

    
    
    // Replicated state
    UPROPERTY(ReplicatedUsing = OnRep_BallState)
    EBallState CurrentState;

    UPROPERTY(Replicated)
    bool bIsOnPuttableSurface;

    UPROPERTY(Replicated)
    FVector LastValidPosition;

    UPROPERTY(Replicated)
    int32 CurrentStrokes;

    // Movement thresholds (Walkabout-style tuning)
    UPROPERTY(EditDefaultsOnly, Category = "Golf|Movement")
    float StoppingVelocityThreshold = 1.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Movement")
    float StoppingAngularVelocityThreshold = 0.1f;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Movement")
    float MaxAirborneTime = 10.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Movement")
    float StuckCheckInterval = 2.0f;

    UPROPERTY(EditDefaultsOnly, Category = "Golf|Movement")
    float StuckMovementThreshold = 5.0f;

    // Trick shot override (hold trigger to disable airborne timeout)
    UPROPERTY(BlueprintReadWrite, Category = "Golf|Movement")
    bool bDisableAirborneTimeout = false;

    // Internal tracking
    float TimeInAir;
    float TimeSinceLastMovement;
    FVector LastPositionCheck;
    float StuckCheckTimer;

    // Collision callbacks
    UFUNCTION()
    void OnBallHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);


    UFUNCTION()
    virtual void OnBallBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);



    UPROPERTY(BlueprintAssignable, Category = "GolfBall|Events")
    FOnGolfBallReset OnBallReset;

    UPROPERTY(BlueprintAssignable, Category = "GolfBall|Events")
    FOnBallBounced OnBallBounced;

    UPROPERTY(BlueprintAssignable, Category = "GolfBall|Events")
    FOnBallStruck OnBallStruck;

    UPROPERTY(BlueprintAssignable, Category = "GolfBall|Events")
    FOnBallHoled OnBallHoled;

    UPROPERTY(BlueprintAssignable, Category = "GolfBall|Events")
    FOnBallStopped OnBallStopped;

    UPROPERTY(BlueprintAssignable, Category = "GolfBall|Events")
    FOnBallStateChanged OnBallStateChanged;


    virtual AActor* GetBallOwner() { return nullptr; }
    

    // State management
    UFUNCTION()
    void OnRep_BallState();

    void UpdateBallState(float DeltaTime);
    void CheckResetConditions(float DeltaTime);
    void CheckIfStopped(float DeltaTime);
    void CheckAirborneTime(float DeltaTime);
    void CheckIfStuck(float DeltaTime);

    // Server RPCs
    UFUNCTION(Server, Reliable)
    void Server_ApplyStroke(const FVector& ImpulseDirection, float ImpulseMagnitude);

    UFUNCTION(Server, Reliable)
    void Server_ResetBall(const FVector& Position, EBallResetReason Reason);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnBallReset(EBallResetReason Reason);

    UFUNCTION(NetMulticast, Reliable)
    void Multicast_OnBallCompleted();


    private:

    void BroadcastBallStopped()
    {
        OnBallStopped.Broadcast(this);
    }
    void BroadcastBallHoled()
    {
        OnBallHoled.Broadcast(this, CurrentStrokes, LastHoleReached);
    }
    void BroadcasBallStruck(FVector Force)
    {
        OnBallStruck.Broadcast(this, Force);
    }
    void BroadcastBallReset(EBallResetReason Reason)
    {
        OnBallReset.Broadcast(this, Reason);
    }
    
    void BroadcastBallStateChanged(EBallState PreviousState)
    {
        OnBallStateChanged.Broadcast(this, CurrentState, PreviousState);
    }

    void SetBallState(EBallState NewState, bool forceSendEvents=false)
    {
        if(bStateSetCalled && !forceSendEvents && NewState == CurrentState)
        {
            UE_LOG(LogTemp, Warning, TEXT("SetBallState called with same state %d, ignoring"), (int32)NewState);
            //no change, don't send events
            return;
        }
        bStateSetCalled = true;
        EBallState prev = CurrentState;
        CurrentState = NewState;
        BroadcastBallStateChanged(prev);
    }

    bool bStateSetCalled = false;
    AActor* LastHoleReached;
};
