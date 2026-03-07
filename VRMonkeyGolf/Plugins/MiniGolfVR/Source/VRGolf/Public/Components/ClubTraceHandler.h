// ClubTraceHandler.h
#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/ClubLineSegment.h"
#include "Delegates/DelegateCombinations.h"
#include "ClubTraceHandler.generated.h"

class AVRGolfBall;
class AVRFloorProxy;

// ─────────────────────────────────────────────────────────────────────────────
// Structs
// ─────────────────────────────────────────────────────────────────────────────

USTRUCT(BlueprintType)
struct FClubVelocityFrame
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FVector Velocity = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    float ProjectedSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bWasSnapped = false;
};

// All data produced at stroke detection.
// Direction = face normal at contact (geometry-derived).
// Velocity fields = club velocity representations (use for impulse magnitude).
USTRUCT(BlueprintType)
struct FStrokeData
{
    GENERATED_BODY()

    // Normalized impulse direction from face geometry — use for ApplyStroke direction
    UPROPERTY(BlueprintReadOnly)
    FVector Direction = FVector::ZeroVector;

    // Average velocity over valid buffer frames — default recommendation for magnitude
    UPROPERTY(BlueprintReadOnly)
    FVector AverageVelocity = FVector::ZeroVector;

    // Peak speed frame velocity
    UPROPERTY(BlueprintReadOnly)
    FVector PeakVelocity = FVector::ZeroVector;

    // Velocity at the exact crossing frame
    UPROPERTY(BlueprintReadOnly)
    FVector VelocityAtCrossing = FVector::ZeroVector;

    // Average floor-projected speed (scalar)
    UPROPERTY(BlueprintReadOnly)
    float AverageProjectedSpeed = 0.0f;

    // Peak floor-projected speed (scalar)
    UPROPERTY(BlueprintReadOnly)
    float PeakProjectedSpeed = 0.0f;

    // Number of valid (snapped, above min speed) frames that contributed
    UPROPERTY(BlueprintReadOnly)
    int32 ValidFrameCount = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Delegates
// ─────────────────────────────────────────────────────────────────────────────

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStrokeDetected,
    class UClubTraceHandler*, Handler,
    const FStrokeData&, StrokeData);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClubSnapped,
    class UClubTraceHandler*, Handler);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClubUnsnapped,
    class UClubTraceHandler*, Handler);

// ─────────────────────────────────────────────────────────────────────────────
// Component
// ─────────────────────────────────────────────────────────────────────────────

/**
 * UClubTraceHandler
 *
 * Non-visual scene component. Attach to a MotionControllerComponent.
 * Forward vector defines the club shaft axis.
 *
 * CanSnapToSurface (BlueprintNativeEvent)
 *   Slides BoxComponent along the forward vector to match floor height.
 *
 * CanDetectStroke (BlueprintNativeEvent)
 *   When snapped: builds a FClubLineSegment each tick via ClubSegmentHelper,
 *   tests for crossing against the ball, broadcasts FStrokeData on hit.
 *   Direction is geometry-derived (face normal). Velocity fields are for
 *   impulse magnitude — BP applies its own multiplier.
 */
UCLASS(Blueprintable, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class VRGOLF_API UClubTraceHandler : public USceneComponent
{
    GENERATED_BODY()

public:
    UClubTraceHandler();

    // ── Setup ────────────────────────────────────────────────────────────────

    UFUNCTION(BlueprintCallable, Category = "Club|Setup")
    void InitializeTraceHandler(UBoxComponent* InBox, AVRGolfBall* InBall);

    UFUNCTION(BlueprintCallable, Category = "Club|Setup")
    void AssignBall(AVRGolfBall* TargetBall);

    UFUNCTION(BlueprintCallable, Category = "Club|Setup")
    void SetStrokeDetectionEnabled(bool bEnabled);

    // ── Gameplay Condition Overrides ─────────────────────────────────────────

    // Default: proxy exists, is grounded, has valid impact point
    UFUNCTION(BlueprintNativeEvent, Category = "Club|Detection")
    bool CanSnapToSurface() const;
    virtual bool CanSnapToSurface_Implementation() const;

    // Default: snapped this frame + detection enabled + ball valid for stroke
    UFUNCTION(BlueprintNativeEvent, Category = "Club|Detection")
    bool CanDetectStroke() const;
    virtual bool CanDetectStroke_Implementation() const;

    // ── State Queries ─────────────────────────────────────────────────────────

    UFUNCTION(BlueprintPure, Category = "Club|State")
    bool IsInitialized() const { return bIsInitialized; }

    UFUNCTION(BlueprintPure, Category = "Club|State")
    bool IsSnappedThisFrame() const { return bSnappedThisFrame; }

    UFUNCTION(BlueprintPure, Category = "Club|State")
    bool IsStrokeDetectionEnabled() const { return bStrokeDetectionEnabled; }

    UFUNCTION(BlueprintPure, Category = "Club|State")
    FVector GetRawVelocity() const { return RawVelocity; }

    UFUNCTION(BlueprintPure, Category = "Club|State")
    FVector GetFloorNormal() const { return CurrentFloorNormal; }

    UFUNCTION(BlueprintPure, Category = "Club|State")
    TArray<FClubVelocityFrame> GetVelocityBuffer() const { return VelocityBuffer; }

    // ── Events ────────────────────────────────────────────────────────────────

    UPROPERTY(BlueprintAssignable, Category = "Club|Events")
    FOnStrokeDetected OnStrokeDetected;

    UPROPERTY(BlueprintAssignable, Category = "Club|Events")
    FOnClubSnapped OnClubSnapped;

    UPROPERTY(BlueprintAssignable, Category = "Club|Events")
    FOnClubUnsnapped OnClubUnsnapped;

    // ── Settings ──────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Snap")
    float MinNeckLength = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Snap")
    float MaxNeckLength = 80.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Detection",
        meta = (ClampMin = "2", ClampMax = "32"))
    int32 VelocityBufferSize = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Detection")
    float MinValidFrameSpeed = 5.0f;

    // ── Debug ─────────────────────────────────────────────────────────────────

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Debug")
    bool bDebugVerboseTick = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Debug")
    bool bDebugDrawSnap = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Debug")
    bool bDebugDrawSegment = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Debug")
    bool bDebugDrawVelocityBuffer = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Club|Debug")
    float DebugScreenDuration = 3.0f;

    UFUNCTION(BlueprintCallable, Category = "Club|Debug")
    void PrintDebugStatus();

protected:
    virtual void BeginPlay() override;

public:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    // ── References ────────────────────────────────────────────────────────────

    UPROPERTY()
    TObjectPtr<AVRGolfBall> Ball;

    UPROPERTY()
    TObjectPtr<UBoxComponent> Box;

    // ── Runtime State ─────────────────────────────────────────────────────────

    bool bIsInitialized = false;
    bool bStrokeDetectionEnabled = false;
    bool bSnappedThisFrame = false;
    bool bWasSnappedLastFrame = false;
    bool bPendingDebugPrint = false;

    FVector    CurrentFloorNormal = FVector::UpVector;
    FVector    RawVelocity = FVector::ZeroVector;
    FVector    PreviousBoxWorldPosition = FVector::ZeroVector;
    FHitResult CachedFloorHit;

    FClubLineSegment PrevSegment;

    TArray<FClubVelocityFrame> VelocityBuffer;
    int32 BufferHead = 0;

    // ── Screen Message Keys ───────────────────────────────────────────────────

    static const int32 DBG_Init = 100;
    static const int32 DBG_SnapState = 101;
    static const int32 DBG_BallState = 102;
    static const int32 DBG_2DHit = 103;
    static const int32 DBG_Velocity = 104;
    static const int32 DBG_NeckLength = 105;
    static const int32 DBG_General = 106;

    // ── Tick Sub-Functions ────────────────────────────────────────────────────

    void TickVelocity(float DeltaTime);
    void TickSnap();
    void TickStrokeDetection();
    void TickBufferDebug() const;

    // ── Snap ──────────────────────────────────────────────────────────────────

    void SetNeckLength(float Distance);

    // ── Velocity Buffer ───────────────────────────────────────────────────────

    void PushVelocityFrame(const FClubVelocityFrame& Frame);
    FStrokeData ComputeStrokeData(const FVector& VelocityAtCrossing, const FVector2D& ImpulseDirection2D) const;

    // ── Stroke ────────────────────────────────────────────────────────────────

    void HandleStrokeDetected(const FVector& CrossingVelocity, const FVector2D& ImpulseDirection2D);

    // ── Helpers ───────────────────────────────────────────────────────────────

    AVRFloorProxy* GetBallFloorProxy() const;
    FVector ProjectOntoFloorPlane(const FVector& V) const;

    // ── Debug Utilities ───────────────────────────────────────────────────────

    void DebugScreen(int32 Key, const FString& Msg, FColor Color = FColor::White) const;
    void DebugLog(const FString& Msg, bool bWarning = false) const;
    void DebugLine(const FVector& Start, const FVector& End, FColor Color, float Thickness = 0.5f) const;
    void DebugLineSegments(const TArray<FVector>& Points, FColor Color, float Thickness = 0.5f, bool bClosed = false) const;
    void DebugSphere(const FVector& Center, float Radius, FColor Color) const;
    void DebugBoxDraw(const FVector& Center, const FVector& Extent, const FQuat& Rot, FColor Color) const;
};