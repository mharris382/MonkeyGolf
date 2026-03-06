// ClubLineSegment.cpp
#include "Components/ClubLineSegment.h"
#include "VRGolfBall.h"
#include "DrawDebugHelpers.h"

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

FClubLineSegment::FClubLineSegment(const FVector& InCornerA, const FVector& InCornerB,
                                   const AVRGolfBall* InBall, float InBallRadius)
{
    if (!InBall)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ClubLineSegment] Constructed with null ball — segment invalid."));
        return;
    }

    // Projection plane: ball center Z + radius
    // This puts the segment at the equator of the ball rather than the floor,
    // which is where the face actually makes contact.
    ProjectionZ = InBall->GetActorLocation().Z + InBallRadius;

    // Project corners to XY at ProjectionZ — world XY, no custom basis needed
    A = FVector2D(InCornerA.X, InCornerA.Y);
    B = FVector2D(InCornerB.X, InCornerB.Y);

    // Compute face normal — perpendicular to segment (2D, CCW rotation)
    FVector2D SegDir = (B - A).GetSafeNormal();
    FVector2D Perp   = FVector2D(-SegDir.Y, SegDir.X);

    FVector SegDir3D = FVector(B.X - A.X, B.Y - A.Y, 0.f).GetSafeNormal();
    Normal2D = FVector2D(FVector::CrossProduct(FVector::UpVector, SegDir3D).GetSafeNormal());

    bValid = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Crossing Test
// ─────────────────────────────────────────────────────────────────────────────

// Public crossing test that takes ball position explicitly
// (Used by ClubSegmentHelper — declared here for proximity to the math)
bool FClubLineSegment::TestCrossing(const FClubLineSegment& Prev,
                                    const FVector2D& BallPoint2D,
                                    float BallRadius,
                                    float& OutT) const
{
    if (!bValid || !Prev.bValid) return false;

    FVector2D Offset = Normal2D * BallRadius;

    FVector2D PA = Prev.A + Offset;
    FVector2D PB = Prev.B + Offset;
    FVector2D CA = A      + Offset;
    FVector2D CB = B      + Offset;

    if (!IsPointInSweptQuad2D(PA, PB, CA, CB, BallPoint2D))
        return false;

    // Estimate T from midpoint sweep
    FVector2D PrevMid  = Prev.Midpoint();
    FVector2D CurMid   = Midpoint();
    FVector2D SweepDir = CurMid - PrevMid;
    float     SweepLen = SweepDir.Size();

    OutT = SweepLen > SMALL_NUMBER
        ? FMath::Clamp(
            FVector2D::DotProduct(BallPoint2D - PrevMid, SweepDir / SweepLen) / SweepLen,
            0.f, 1.f)
        : 0.5f;

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// Debug Draw
// ─────────────────────────────────────────────────────────────────────────────

void FClubLineSegment::DrawDebug(UWorld* World,
                                 FColor SegmentColor, FColor NormalColor,
                                 float Thickness, float NormalLength) const
{
#if ENABLE_DRAW_DEBUG
    if (!World || !bValid) return;

    // Segment line
    DrawDebugLine(World, A3D(), B3D(), SegmentColor, false, -1.f, 0, Thickness);

    // Endpoint dots
    DrawDebugSphere(World, A3D(), 1.2f, 4, SegmentColor, false, -1.f);
    DrawDebugSphere(World, B3D(), 1.2f, 4, SegmentColor, false, -1.f);

    // Normal arrow from midpoint
    FVector Mid  = Midpoint3D();
    FVector NEnd = Mid + Normal3D() * NormalLength;
    DrawDebugLine(World, Mid, NEnd, NormalColor, false, -1.f, 0, Thickness);
    DrawDebugSphere(World, NEnd, 1.5f, 4, NormalColor, false, -1.f);
#endif
}

// ─────────────────────────────────────────────────────────────────────────────
// Private Math
// ─────────────────────────────────────────────────────────────────────────────

float FClubLineSegment::SignedTriArea2D(const FVector2D& P, const FVector2D& Q, const FVector2D& R) const
{
    return (Q.X - P.X) * (R.Y - P.Y) - (R.X - P.X) * (Q.Y - P.Y);
}

bool FClubLineSegment::IsPointInSweptQuad2D(
    const FVector2D& PA, const FVector2D& PB,
    const FVector2D& CA, const FVector2D& CB,
    const FVector2D& P) const
{
    // Convex quad: PA -> PB -> CB -> CA
    // Point inside if all signed areas share the same sign
    float S0 = SignedTriArea2D(PA, PB, P);
    float S1 = SignedTriArea2D(PB, CB, P);
    float S2 = SignedTriArea2D(CB, CA, P);
    float S3 = SignedTriArea2D(CA, PA, P);

    return (S0 >= 0.f && S1 >= 0.f && S2 >= 0.f && S3 >= 0.f)
        || (S0 <= 0.f && S1 <= 0.f && S2 <= 0.f && S3 <= 0.f);
}