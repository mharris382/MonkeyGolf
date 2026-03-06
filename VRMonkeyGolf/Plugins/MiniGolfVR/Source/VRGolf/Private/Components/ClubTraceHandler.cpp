// ClubTraceHandler.cpp
#include "Components/ClubTraceHandler.h"
#include "Components/ClubSegmentHelper.h"
#include "VRGolfBall.h"
#include "VRFloorProxy.h"
#include "DrawDebugHelpers.h"
#include "Components/SphereComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

// ─────────────────────────────────────────────────────────────────────────────
// Debug Utilities
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::DebugScreen(int32 Key, const FString& Msg, FColor Color) const
{
#if !UE_BUILD_SHIPPING
    if (GEngine)
        GEngine->AddOnScreenDebugMessage(Key, DebugScreenDuration, Color,
            FString::Printf(TEXT("[CTH] %s"), *Msg));
#endif
}

void UClubTraceHandler::DebugLog(const FString& Msg, bool bWarning) const
{
#if !UE_BUILD_SHIPPING
    if (bWarning)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ClubTraceHandler] %s"), *Msg);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("[ClubTraceHandler] %s"), *Msg);
    }
#endif
}

void UClubTraceHandler::DebugLine(const FVector& Start, const FVector& End, FColor Color, float Thickness) const
{
#if ENABLE_DRAW_DEBUG
    if (GetWorld())
        DrawDebugLine(GetWorld(), Start, End, Color, false, -1.0f, 0, Thickness);
#endif
}

void UClubTraceHandler::DebugLineSegments(const TArray<FVector>& Points, FColor Color, float Thickness, bool bClosed) const
{
#if ENABLE_DRAW_DEBUG
    if (!GetWorld() || Points.Num() < 2) return;
    for (int32 i = 0; i < Points.Num() - 1; ++i)
        DrawDebugLine(GetWorld(), Points[i], Points[i + 1], Color, false, -1.0f, 0, Thickness);
    if (bClosed && Points.Num() > 2)
        DrawDebugLine(GetWorld(), Points.Last(), Points[0], Color, false, -1.0f, 0, Thickness);
#endif
}

void UClubTraceHandler::DebugSphere(const FVector& Center, float Radius, FColor Color) const
{
#if ENABLE_DRAW_DEBUG
    if (GetWorld())
        DrawDebugSphere(GetWorld(), Center, Radius, 8, Color, false, -1.0f, 0, 0.5f);
#endif
}

void UClubTraceHandler::DebugBoxDraw(const FVector& Center, const FVector& Extent, const FQuat& Rot, FColor Color) const
{
#if ENABLE_DRAW_DEBUG
    if (GetWorld())
        DrawDebugBox(GetWorld(), Center, Extent, Rot, Color, false, -1.0f, 0, 0.5f);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

UClubTraceHandler::UClubTraceHandler()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
}

// ─────────────────────────────────────────────────────────────────────────────
// Setup
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::InitializeTraceHandler(UBoxComponent* InBox, AVRGolfBall* InBall)
{
    if (!InBox)
    {
        DebugLog(TEXT("INIT FAILED — BoxComponent is NULL."), true);
        DebugScreen(DBG_Init, TEXT("INIT FAILED: Box is NULL"), FColor::Red);
        return;
    }

    Box = InBox;
    Ball = InBall;

    if (!InBall)
    {
        DebugLog(TEXT("INIT WARNING — Ball NULL. Assign later via AssignBall()."), true);
        DebugScreen(DBG_Init, TEXT("INIT WARN: Ball NULL (assign later)"), FColor::Orange);
    }

    float ComputedDist = FVector::Dist(GetComponentLocation(), Box->GetComponentLocation());
    if (ComputedDist < 1.0f)
    {
        DebugLog(FString::Printf(TEXT("INIT WARNING — Initial box distance nearly zero (%.2f). "
            "Box may not be positioned on club yet."), ComputedDist), true);
        DebugScreen(DBG_Init,
            FString::Printf(TEXT("INIT WARN: BoxDist=%.2f (check attachment)"), ComputedDist),
            FColor::Orange);
    }

    if (ComputedDist < MinNeckLength || ComputedDist > MaxNeckLength)
    {
        DebugLog(FString::Printf(TEXT("INIT WARNING — Initial box distance (%.1f) outside neck range [%.1f, %.1f]"),
            ComputedDist, MinNeckLength, MaxNeckLength), true);
        DebugScreen(DBG_Init,
            FString::Printf(TEXT("INIT WARN: BoxDist=%.1f outside [%.1f-%.1f]"),
                ComputedDist, MinNeckLength, MaxNeckLength),
            FColor::Orange);
    }

    VelocityBuffer.SetNum(VelocityBufferSize);
    for (auto& F : VelocityBuffer) F = FClubVelocityFrame();
    BufferHead = 0;

    PreviousBoxWorldPosition = Box->GetComponentLocation();
    PrevSegment = FClubLineSegment();
    bIsInitialized = true;

    DebugLog(FString::Printf(TEXT("INIT OK — Box:%s Ball:%s BoxDist:%.1f Neck:[%.1f-%.1f] BufSize:%d"),
        *InBox->GetName(),
        InBall ? *InBall->GetName() : TEXT("None"),
        ComputedDist, MinNeckLength, MaxNeckLength, VelocityBufferSize));

    DebugScreen(DBG_Init,
        FString::Printf(TEXT("INIT OK | Neck:[%.0f-%.0f] Ball:%s Buf:%d"),
            MinNeckLength, MaxNeckLength,
            InBall ? *InBall->GetName() : TEXT("None"),
            VelocityBufferSize),
        FColor::Green);

    SetComponentTickEnabled(true);
}

void UClubTraceHandler::AssignBall(AVRGolfBall* TargetBall)
{
    Ball = TargetBall;
    PrevSegment = FClubLineSegment(); // Reset on ball swap

    if (Ball)
    {
        DebugLog(FString::Printf(TEXT("Ball assigned: %s"), *Ball->GetName()));
        DebugScreen(DBG_BallState, FString::Printf(TEXT("Ball: %s"), *Ball->GetName()), FColor::Green);
    }
    else
    {
        DebugLog(TEXT("Ball cleared"), true);
        DebugScreen(DBG_BallState, TEXT("Ball: cleared"), FColor::Orange);
    }
}

void UClubTraceHandler::SetStrokeDetectionEnabled(bool bEnabled)
{
    bStrokeDetectionEnabled = bEnabled;
    SetComponentTickEnabled(bEnabled);
    if (!bEnabled)
    {
        bSnappedThisFrame = false;
        PrevSegment = FClubLineSegment();
    }

    DebugLog(FString::Printf(TEXT("Stroke detection %s"), bEnabled ? TEXT("ENABLED") : TEXT("DISABLED")));
    DebugScreen(DBG_General,
        FString::Printf(TEXT("Detection: %s"), bEnabled ? TEXT("ON") : TEXT("OFF")),
        bEnabled ? FColor::Green : FColor::Red);
}

// ─────────────────────────────────────────────────────────────────────────────
// BlueprintNativeEvent — Gameplay Conditions
// ─────────────────────────────────────────────────────────────────────────────

bool UClubTraceHandler::CanSnapToSurface_Implementation() const
{
    return Ball!=nullptr;//&& Ball->GetBallState() == EBallState::Idle;
}

bool UClubTraceHandler::CanDetectStroke_Implementation() const
{
    return bSnappedThisFrame
        && bStrokeDetectionEnabled
        && Ball != nullptr
        && Ball->IsValidForStroke();
}

// ─────────────────────────────────────────────────────────────────────────────
// PrintDebugStatus
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::PrintDebugStatus()
{
    DebugLog(TEXT("========== ClubTraceHandler Status =========="));
    DebugLog(FString::Printf(TEXT("  Initialized:      %s"), bIsInitialized ? TEXT("YES") : TEXT("NO")));
    DebugLog(FString::Printf(TEXT("  Box:              %s"), Box ? *Box->GetName() : TEXT("NULL")));
    DebugLog(FString::Printf(TEXT("  Ball:             %s"), Ball ? *Ball->GetName() : TEXT("NULL")));
    DebugLog(FString::Printf(TEXT("  DetectionEnabled: %s"), bStrokeDetectionEnabled ? TEXT("YES") : TEXT("NO")));
    DebugLog(FString::Printf(TEXT("  SnappedThisFrame: %s"), bSnappedThisFrame ? TEXT("YES") : TEXT("NO")));
    DebugLog(FString::Printf(TEXT("  FloorNormal:      %s"), *CurrentFloorNormal.ToString()));
    DebugLog(FString::Printf(TEXT("  NeckRange:        [%.1f - %.1f]"), MinNeckLength, MaxNeckLength));
    DebugLog(FString::Printf(TEXT("  BufferSize:       %d"), VelocityBufferSize));
    DebugLog(FString::Printf(TEXT("  RawVelocity:      %s (%.1f)"), *RawVelocity.ToString(), RawVelocity.Size()));
    DebugLog(FString::Printf(TEXT("  PrevSegment.Valid:%s"), PrevSegment.bValid ? TEXT("YES") : TEXT("NO")));

    if (Ball)
    {
        DebugLog(FString::Printf(TEXT("  Ball.ValidStroke: %s  State:%d"),
            Ball->IsValidForStroke() ? TEXT("YES") : TEXT("NO"),
            (int32)Ball->GetBallState()));
        AVRFloorProxy* Proxy = GetBallFloorProxy();
        DebugLog(FString::Printf(TEXT("  FloorProxy:       %s %s"),
            Proxy ? TEXT("EXISTS") : TEXT("NULL"),
            Proxy ? (Proxy->IsGrounded() ? TEXT("GROUNDED") : TEXT("AIRBORNE")) : TEXT("")));
        if (Proxy)
            DebugLog(FString::Printf(TEXT("  Proxy.ImpactPt:   %s"),
                *Proxy->GetLastFloorHit().ImpactPoint.ToString()));
    }
    DebugLog(TEXT("============================================="));
    bPendingDebugPrint = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::BeginPlay()
{
    Super::BeginPlay();
    DebugLog(TEXT("BeginPlay — awaiting InitializeTraceHandler()"));
    DebugScreen(DBG_Init, TEXT("CTH: awaiting Init()"), FColor::Yellow);
}

void UClubTraceHandler::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bIsInitialized || !Box)
    {
        DebugScreen(DBG_Init,
            !bIsInitialized ? TEXT("TICK SKIP: not initialized") : TEXT("TICK SKIP: Box NULL"),
            FColor::Red);
        return;
    }

    TickVelocity(DeltaTime);

    TickSnap();

    if (bSnappedThisFrame)
        TickStrokeDetection();
    else
        PrevSegment = FClubLineSegment(); // Clear stale segment when not snapped

    // Push buffer frame
    FClubVelocityFrame Frame;
    Frame.Velocity = RawVelocity;
    Frame.ProjectedSpeed = ProjectOntoFloorPlane(RawVelocity).Size();
    Frame.bWasSnapped = bSnappedThisFrame;
    PushVelocityFrame(Frame);

    if (bDebugDrawVelocityBuffer)
        TickBufferDebug();

    PreviousBoxWorldPosition = Box->GetComponentLocation();

    if (bPendingDebugPrint)
    {
        bPendingDebugPrint = false;
        DebugScreen(DBG_Init,
            FString::Printf(TEXT("Init:%s Box:%s Ball:%s Snapped:%s"),
                bIsInitialized ? TEXT("Y") : TEXT("N"),
                Box ? *Box->GetName() : TEXT("NULL"),
                Ball ? *Ball->GetName() : TEXT("NULL"),
                bSnappedThisFrame ? TEXT("Y") : TEXT("N")),
            FColor::Cyan);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Tick Sub-Functions
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::TickVelocity(float DeltaTime)
{
    FVector CurrentBoxPos = Box->GetComponentLocation();
    RawVelocity = (CurrentBoxPos - PreviousBoxWorldPosition) / DeltaTime;

    if (bDebugVerboseTick)
        DebugScreen(DBG_Velocity,
            FString::Printf(TEXT("Spd:%.1f | Vel:%s"), RawVelocity.Size(), *RawVelocity.ToString()),
            FColor::White);
}

void UClubTraceHandler::TickSnap()
{
    bWasSnappedLastFrame = bSnappedThisFrame;
    bSnappedThisFrame = false;

    if (!CanSnapToSurface())
    {
        if (bWasSnappedLastFrame)
        {
            DebugLog(TEXT("Snap LOST"));
            DebugScreen(DBG_SnapState, TEXT("Snap: LOST"), FColor::Orange);
            OnClubUnsnapped.Broadcast(this);
        }
        else if (bDebugVerboseTick)
            DebugScreen(DBG_SnapState, TEXT("Snap: no surface"), FColor::Silver);
        return;
    }

    AVRFloorProxy* Proxy = GetBallFloorProxy();
    const FHitResult& FloorHit = Proxy->GetLastFloorHit();
    CurrentFloorNormal = FloorHit.ImpactNormal;
    CachedFloorHit = FloorHit;

    // Neck length = vertical drop from club origin to floor
    // TODO: support non-upward normals (inverted gravity surfaces)
    float FloorZ = Ball ? Ball->GetActorLocation().Z : FloorHit.ImpactPoint.Z;
    float NeckLength = FMath::Abs(GetComponentLocation().Z - FloorZ);

    SetNeckLength(NeckLength);
    bSnappedThisFrame = true;

    if (!bWasSnappedLastFrame)
    {
        DebugLog(FString::Printf(TEXT("Snap ACQUIRED — Floor:%s Normal:%s Neck:%.1f"),
            *FloorHit.ImpactPoint.ToString(), *CurrentFloorNormal.ToString(), NeckLength));
        DebugScreen(DBG_SnapState,
            FString::Printf(TEXT("Snap: ACQUIRED | Neck=%.1f"), NeckLength),
            FColor::Green);
        OnClubSnapped.Broadcast(this);
    }
    else if (bDebugVerboseTick)
        DebugScreen(DBG_SnapState,
            FString::Printf(TEXT("Snap: OK | Neck=%.1f"), NeckLength),
            FColor::Green);

#if ENABLE_DRAW_DEBUG
    if (bDebugDrawSnap && Box)
    {
        DebugBoxDraw(Box->GetComponentLocation(), Box->GetScaledBoxExtent(),
            Box->GetComponentQuat(), FColor::Green);
        DebugSphere(FloorHit.ImpactPoint, 3.0f, FColor::Green);
        DebugLine(FloorHit.ImpactPoint,
            FloorHit.ImpactPoint + CurrentFloorNormal * 20.0f, FColor::Blue, 1.0f);
    }
#endif
}

void UClubTraceHandler::TickStrokeDetection()
{
    if (!CanDetectStroke())
    {
        PrevSegment = FClubLineSegment();

        if (bDebugVerboseTick)
        {
            FString Reason;
            if (!Ball)                          Reason = TEXT("no ball");
            else if (!Ball->IsValidForStroke()) Reason = FString::Printf(
                TEXT("ball state=%d"), (int32)Ball->GetBallState());
            else if (!bStrokeDetectionEnabled)  Reason = TEXT("detection disabled");
            DebugScreen(DBG_2DHit,
                FString::Printf(TEXT("StrokeDetect BLOCKED: %s"), *Reason), FColor::Silver);
        }
        return;
    }

    // Build segment from current box face, using velocity as swing direction
    FClubLineSegment CurSegment = UClubSegmentHelper::BuildSegment(Box, Ball, RawVelocity);

    if (!CurSegment.bValid)
    {
        DebugLog(TEXT("TickStrokeDetection — BuildSegment failed"), true);
        DebugScreen(DBG_2DHit, TEXT("Segment: INVALID"), FColor::Red);
        PrevSegment = FClubLineSegment();
        return;
    }

#if ENABLE_DRAW_DEBUG
    if (bDebugDrawSegment)
        CurSegment.DrawDebug(GetWorld());
#endif

    // Test crossing if we have a valid previous segment
    if (PrevSegment.bValid)
    {
        float     CrossingT = 0.f;
        FVector2D ImpulseDirection2D = FVector2D::ZeroVector;

        bool bCrossed = UClubSegmentHelper::TestCrossing(
            PrevSegment, CurSegment, Ball, CrossingT, ImpulseDirection2D);

        if (bCrossed)
        {
            DebugLog(FString::Printf(TEXT("CROSSING DETECTED | T=%.2f | Spd=%.1f | Impulse:%s"),
                CrossingT, RawVelocity.Size(), *ImpulseDirection2D.ToString()));
            DebugScreen(DBG_2DHit,
                FString::Printf(TEXT("HIT | T=%.2f Spd=%.0f Impulse:%s"),
                    CrossingT, RawVelocity.Size(), *ImpulseDirection2D.ToString()),
                FColor::Cyan);

            HandleStrokeDetected(RawVelocity, ImpulseDirection2D);

            // Disarm until club leaves and re-enters
            PrevSegment = FClubLineSegment();
            return;
        }
    }

    PrevSegment = CurSegment;
}

void UClubTraceHandler::TickBufferDebug() const
{
#if ENABLE_DRAW_DEBUG
    if (!Ball) return;

    FVector Origin = Ball->GetActorLocation() + FVector(0, 0, 5.0f);
    float   DrawScale = 0.05f;

    for (int32 i = 0; i < VelocityBuffer.Num(); ++i)
    {
        int32 Idx = (BufferHead + i) % VelocityBuffer.Num();
        const FClubVelocityFrame& Frame = VelocityBuffer[Idx];

        FVector Dir = ProjectOntoFloorPlane(Frame.Velocity).GetSafeNormal();
        FVector End = Origin + Dir * (Frame.ProjectedSpeed * DrawScale);
        FColor  Col = Frame.bWasSnapped ? FColor::Green : FColor::Silver;

        DrawDebugLine(GetWorld(), Origin, End, Col, false, -1.f, 0, 0.5f);
        DrawDebugSphere(GetWorld(), End, 0.8f, 4, Col, false, -1.f);
    }

    DebugSphere(Origin, 1.5f, FColor::Yellow);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// SetNeckLength — single writer
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::SetNeckLength(float Distance)
{
    if (!Box) return;

    float Clamped = FMath::Clamp(Distance, MinNeckLength, MaxNeckLength);

    if (bDebugVerboseTick && !FMath::IsNearlyEqual(Distance, Clamped, 0.1f))
        DebugScreen(DBG_NeckLength,
            FString::Printf(TEXT("Neck clamped %.1f->%.1f [%.1f-%.1f]"),
                Distance, Clamped, MinNeckLength, MaxNeckLength),
            FColor::Orange);

    FVector NewPos = GetComponentLocation() + GetForwardVector() * Clamped;

    // Hard floor Z clamp — box bottom must never penetrate floor plane
    if (CachedFloorHit.bBlockingHit)
    {
        float HalfBoxZ = Box->GetScaledBoxExtent().Z;
        float MinZ = CachedFloorHit.ImpactPoint.Z + HalfBoxZ;
        if (NewPos.Z < MinZ)
        {
            if (bDebugVerboseTick)
                DebugScreen(DBG_NeckLength,
                    FString::Printf(TEXT("BoxZ clamped %.1f->%.1f (floor=%.1f)"),
                        NewPos.Z, MinZ, CachedFloorHit.ImpactPoint.Z),
                    FColor::Orange);
            NewPos.Z = MinZ;
        }
    }

    Box->SetWorldLocation(NewPos, false, nullptr, ETeleportType::TeleportPhysics);

    if (bDebugVerboseTick)
        DebugScreen(DBG_NeckLength,
            FString::Printf(TEXT("Neck=%.1f pos=%s"), Clamped, *NewPos.ToString()),
            FColor::Green);
}

// ─────────────────────────────────────────────────────────────────────────────
// Velocity Buffer
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::PushVelocityFrame(const FClubVelocityFrame& Frame)
{
    if (VelocityBuffer.Num() != VelocityBufferSize)
    {
        VelocityBuffer.SetNum(VelocityBufferSize);
        BufferHead = 0;
    }
    VelocityBuffer[BufferHead] = Frame;
    BufferHead = (BufferHead + 1) % VelocityBufferSize;
}

FStrokeData UClubTraceHandler::ComputeStrokeData(const FVector& VelocityAtCrossing,
    const FVector2D& ImpulseDirection2D) const
{
    FStrokeData Result;
    Result.VelocityAtCrossing = VelocityAtCrossing;

    // Direction is purely geometry-derived — never from velocity
    Result.Direction = FVector(ImpulseDirection2D.X, ImpulseDirection2D.Y, 0.f).GetSafeNormal();

    if (Result.Direction.IsNearlyZero())
    {
        DebugLog(TEXT("ComputeStrokeData ERROR: ImpulseDirection2D is zero — using forward vector"), true);
        DebugScreen(DBG_2DHit, TEXT("STROKE ERROR: ImpulseDir=zero, using forward"), FColor::Red);
        Result.Direction = ProjectOntoFloorPlane(GetForwardVector()).GetSafeNormal();
    }

    FVector VelSum = FVector::ZeroVector;
    FVector PeakVel = FVector::ZeroVector;
    float   PeakSpeed = 0.f;
    float   SpeedSum = 0.f;
    int32   ValidCount = 0;

    for (const FClubVelocityFrame& Frame : VelocityBuffer)
    {
        if (!Frame.bWasSnapped)                        continue;
        if (Frame.ProjectedSpeed < MinValidFrameSpeed) continue;

        VelSum += Frame.Velocity;
        SpeedSum += Frame.ProjectedSpeed;
        ValidCount++;

        if (Frame.ProjectedSpeed > PeakSpeed)
        {
            PeakSpeed = Frame.ProjectedSpeed;
            PeakVel = Frame.Velocity;
        }
    }

    Result.ValidFrameCount = ValidCount;
    Result.PeakVelocity = PeakVel;
    Result.PeakProjectedSpeed = PeakSpeed;

    if (ValidCount > 0)
    {
        Result.AverageVelocity = VelSum / (float)ValidCount;
        Result.AverageProjectedSpeed = SpeedSum / (float)ValidCount;
    }
    else
    {
        DebugLog(TEXT("ComputeStrokeData WARNING: no valid buffer frames — using crossing velocity"), true);
        DebugScreen(DBG_2DHit, TEXT("STROKE WARN: no valid buffer frames"), FColor::Orange);
        Result.AverageVelocity = VelocityAtCrossing;
        Result.AverageProjectedSpeed = ProjectOntoFloorPlane(VelocityAtCrossing).Size();
        Result.PeakVelocity = VelocityAtCrossing;
        Result.PeakProjectedSpeed = Result.AverageProjectedSpeed;
    }

    return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
// Stroke Handling
// ─────────────────────────────────────────────────────────────────────────────

void UClubTraceHandler::HandleStrokeDetected(const FVector& CrossingVelocity,
    const FVector2D& ImpulseDirection2D)
{
    FStrokeData StrokeData = ComputeStrokeData(CrossingVelocity, ImpulseDirection2D);

    DebugLog(FString::Printf(TEXT("STROKE | Dir:%s AvgSpd:%.0f PeakSpd:%.0f Frames:%d"),
        *StrokeData.Direction.ToString(),
        StrokeData.AverageProjectedSpeed,
        StrokeData.PeakProjectedSpeed,
        StrokeData.ValidFrameCount));

    DebugScreen(DBG_2DHit,
        FString::Printf(TEXT("STROKE | Avg:%.0f Peak:%.0f Frames:%d"),
            StrokeData.AverageProjectedSpeed,
            StrokeData.PeakProjectedSpeed,
            StrokeData.ValidFrameCount),
        FColor::Cyan);

#if ENABLE_DRAW_DEBUG
    if (bDebugDrawSegment && Ball)
    {
        FVector BallPos = Ball->GetActorLocation();
        // Cyan = impulse direction (face normal)
        DebugLine(BallPos, BallPos + StrokeData.Direction * 60.f, FColor::Cyan, 2.0f);
        // Green = average velocity direction
        DebugLine(BallPos, BallPos + StrokeData.AverageVelocity.GetSafeNormal() * 50.f, FColor::Green, 1.0f);
        // Red = peak velocity direction
        DebugLine(BallPos, BallPos + StrokeData.PeakVelocity.GetSafeNormal() * 50.f, FColor::Red, 1.0f);
        DebugSphere(BallPos, 4.0f, FColor::Cyan);
    }
#endif

    OnStrokeDetected.Broadcast(this, StrokeData);
}

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

AVRFloorProxy* UClubTraceHandler::GetBallFloorProxy() const
{
    if (!Ball) return nullptr;
    return const_cast<AVRGolfBall*>(Ball.Get())->GetFloorProxy();
}

FVector UClubTraceHandler::ProjectOntoFloorPlane(const FVector& V) const
{
    return V - CurrentFloorNormal * FVector::DotProduct(V, CurrentFloorNormal);
}