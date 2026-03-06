// ClubSegmentHelper.cpp
#include "Components/ClubSegmentHelper.h"
#include "VRGolfBall.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

FClubLineSegment UClubSegmentHelper::BuildSegment(UBoxComponent* Box, AVRGolfBall* Ball, FVector SwingDirection)
{
    if (!Box || !Ball)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ClubSegmentHelper] BuildSegment — Box or Ball is null."));
        return FClubLineSegment();
    }

    FVector CornerA, CornerB;
    if (!SelectFaceCorners(Box, SwingDirection, CornerA, CornerB))
    {
        UE_LOG(LogTemp, Warning, TEXT("[ClubSegmentHelper] BuildSegment — SelectFaceCorners failed."));
        return FClubLineSegment();
    }

    return FClubLineSegment(CornerA, CornerB, Ball, GetBallRadius(Ball));
}

FClubLineSegment UClubSegmentHelper::BuildAndDraw(UObject* WorldContext, UBoxComponent* Box, AVRGolfBall* Ball, FVector SwingDirection)
{
    FClubLineSegment Seg = BuildSegment(Box, Ball, SwingDirection);

#if ENABLE_DRAW_DEBUG
    UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    if (World)
    {
        if (Seg.bValid)
        {
            Seg.DrawDebug(World);

            // Also draw a sphere at ball center XY projected to segment height
            // so you can see how close the segment sits to the ball
            FVector BallCenter = Ball->GetActorLocation();
            FVector BallAtSegZ = FVector(BallCenter.X, BallCenter.Y, Seg.ProjectionZ);
            DrawDebugSphere(World, BallAtSegZ, GetBallRadius(Ball), 12, FColor::White, false, -1.f, 0, 0.8f);

            // Line from segment midpoint to ball center — should be parallel to Normal
            DrawDebugLine(World, Seg.Midpoint3D(), BallAtSegZ, FColor::Cyan, false, -1.f, 0, 0.5f);
        }
        else
        {
            // Draw a red X at box location so failure is visible in viewport
            FVector BoxLoc = Box ? Box->GetComponentLocation() : FVector::ZeroVector;
            DrawDebugSphere(World, BoxLoc, 8.f, 6, FColor::Red, false, -1.f, 0, 1.f);
            UE_LOG(LogTemp, Warning, TEXT("[ClubSegmentHelper] BuildAndDraw — segment invalid, drawing red marker."));
        }
    }
#endif

    return Seg;
}

bool UClubSegmentHelper::TestCrossing(const FClubLineSegment& PrevSegment,
                                      const FClubLineSegment& CurSegment,
                                      AVRGolfBall* Ball,
                                      float& OutT,FVector2D& OutImpulseDirection)
{
    if (!Ball) return false;

    FVector2D BallPoint2D(Ball->GetActorLocation().X, Ball->GetActorLocation().Y);
    float Radius = GetBallRadius(Ball);

    bool bCrossed = CurSegment.TestCrossing(PrevSegment, BallPoint2D, Radius, OutT);

    if (bCrossed)
    {
        // Average the normal of prev and current segment — handles the case
        // where the face rotated slightly mid-swing
        FVector2D AvgNormal = (PrevSegment.Normal2D + CurSegment.Normal2D).GetSafeNormal();

        OutImpulseDirection = AvgNormal;
    }

    return bCrossed;
}

float UClubSegmentHelper::GetBallRadius(AVRGolfBall* Ball)
{
    if (Ball)
        if (USphereComponent* Sphere = Ball->FindComponentByClass<USphereComponent>())
            return Sphere->GetScaledSphereRadius();

    return 2.135f;
}

// ─────────────────────────────────────────────────────────────────────────────
// Face Selection
// ─────────────────────────────────────────────────────────────────────────────

bool UClubSegmentHelper::SelectFaceCorners(UBoxComponent* Box, const FVector& SwingDirection,
    FVector& OutCornerA, FVector& OutCornerB)
{
    if (!Box) return false;

    FQuat   BoxRot = Box->GetComponentQuat();
    FVector BoxCenter = Box->GetComponentLocation();
    FVector HalfExtent = Box->GetScaledBoxExtent();

    FVector LocalAxes[2] = { BoxRot.GetAxisX(), BoxRot.GetAxisY() };
    float   Extents[2] = { HalfExtent.X,      HalfExtent.Y };

    // Flatten swing direction to XY — face selection is a 2D decision
    FVector FlatSwing = SwingDirection;
    FlatSwing.Z = 0.f;
    FlatSwing = FlatSwing.GetSafeNormal();

    if (FlatSwing.IsNearlyZero())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ClubSegmentHelper] SelectFaceCorners — SwingDirection is zero or vertical."));
        return false;
    }

    // Find face most aligned with swing direction
    int32 BestAxis = 0;
    float BestDot = -1.f;
    float BestSign = 1.f;

    for (int32 i = 0; i < 2; ++i)
    {
        FVector FlatAxis = LocalAxes[i];
        FlatAxis.Z = 0.f;
        FlatAxis = FlatAxis.GetSafeNormal();

        float DotPos = FVector::DotProduct(FlatSwing, FlatAxis);
        float DotNeg = FVector::DotProduct(FlatSwing, -FlatAxis);

        if (DotPos > BestDot) { BestDot = DotPos; BestAxis = i; BestSign = 1.f; }
        if (DotNeg > BestDot) { BestDot = DotNeg; BestAxis = i; BestSign = -1.f; }
    }

    FVector FaceNormal = LocalAxes[BestAxis] * BestSign;
    FVector FaceCenter = BoxCenter + FaceNormal * Extents[BestAxis];
    int32   SpanAxis = (BestAxis == 0) ? 1 : 0;
    FVector SpanDir = LocalAxes[SpanAxis];
    float   SpanExt = Extents[SpanAxis];

    FVector BottomCenter = FaceCenter - BoxRot.GetAxisZ() * HalfExtent.Z;
    OutCornerA = BottomCenter + SpanDir * SpanExt;
    OutCornerB = BottomCenter - SpanDir * SpanExt;

    return true;
}