// ClubLineSegment.h
#pragma once

#include "CoreMinimal.h"
#include "ClubLineSegment.generated.h"

class AVRGolfBall;

/**
 * FClubLineSegment
 *
 * Represents the two bottom corners of a putter face projected onto the
 * horizontal plane at ball-center height (ball Z + radius).
 *
 * Construct with two 3D world-space corner points and a ball reference.
 * The ball must not be null — it provides both the projection height and
 * the reference point for orienting the face normal toward the ball.
 *
 * All 2D coordinates live in world XY (no custom basis needed — projection
 * plane is always horizontal at a fixed Z, so X and Y are world X and Y).
 */
USTRUCT(BlueprintType)
struct VRGOLF_API FClubLineSegment
{
    GENERATED_BODY()

    // ── Projected 2D points (world XY at ball-center Z) ──────────────────────

    UPROPERTY(BlueprintReadOnly)
    FVector2D A = FVector2D::ZeroVector;

    UPROPERTY(BlueprintReadOnly)
    FVector2D B = FVector2D::ZeroVector;

    // Face normal in 2D — always oriented toward the ball
    UPROPERTY(BlueprintReadOnly)
    FVector2D Normal2D = FVector2D::ZeroVector;

    // Z height used for projection (ball Z + ball radius)
    UPROPERTY(BlueprintReadOnly)
    float ProjectionZ = 0.0f;

    // Whether this segment was successfully constructed
    UPROPERTY(BlueprintReadOnly)
    bool bValid = false;

    // ── Construction ─────────────────────────────────────────────────────────

    FClubLineSegment() = default;

    /**
     * Construct from two 3D world-space corner points and a ball.
     * Projects both corners to the plane at (ball center Z + ball radius).
     * Normal is computed from the projected segment and oriented toward ball.
     *
     * @param InCornerA   First bottom corner of the selected box face (world space)
     * @param InCornerB   Second bottom corner of the selected box face (world space)
     * @param InBall      Must not be null
     * @param InBallRadius  Collision radius of the ball (cm)
     */
    FClubLineSegment(const FVector& InCornerA, const FVector& InCornerB,
                     const AVRGolfBall* InBall, float InBallRadius);

    // ── Queries ───────────────────────────────────────────────────────────────

    // Midpoint of the segment in 2D
    FVector2D Midpoint() const { return (A + B) * 0.5f; }

    // Length of the segment
    float Length() const { return FVector2D::Distance(A, B); }

    // Returns the 3D world position of A lifted to ProjectionZ
    FVector A3D() const { return FVector(A.X, A.Y, ProjectionZ); }

    // Returns the 3D world position of B lifted to ProjectionZ
    FVector B3D() const { return FVector(B.X, B.Y, ProjectionZ); }

    // Returns the 3D world midpoint lifted to ProjectionZ
    FVector Midpoint3D() const { return FVector(Midpoint().X, Midpoint().Y, ProjectionZ); }

    // Normal as 3D vector (Z=0, floor-plane)
    FVector Normal3D() const { return FVector(Normal2D.X, Normal2D.Y, 0.f); }

    // ── Crossing Test ─────────────────────────────────────────────────────────

    /**
     * Tests whether this segment swept from PrevSegment to this segment
     * crossed the ball center point (expanded by BallRadius).
     *
     * @param PrevSegment   Segment from the previous tick
     * @param BallRadius    Expansion radius for the crossing test
     * @param OutT          Estimated 0-1 interpolation along the sweep at crossing
     */
    bool TestCrossing(const FClubLineSegment& PrevSegment, const FVector2D& BallPoint2D, float BallRadius, float& OutT) const;

    // ── Debug Draw ────────────────────────────────────────────────────────────

    /**
     * Draws the segment and its normal in world space at ProjectionZ.
     * Safe to call every tick — all draws are single-frame (no persistent).
     *
     * @param World         Must not be null
     * @param SegmentColor  Color for the AB line
     * @param NormalColor   Color for the normal arrow
     * @param Thickness     Line thickness
     * @param NormalLength  Length of the normal arrow (cm)
     */
    void DrawDebug(UWorld* World,
                   FColor SegmentColor = FColor::Yellow,
                   FColor NormalColor  = FColor::Orange,
                   float Thickness     = 1.5f,
                   float NormalLength  = 12.0f) const;

private:
    float SignedTriArea2D(const FVector2D& P, const FVector2D& Q, const FVector2D& R) const;
    bool  IsPointInSweptQuad2D(const FVector2D& PA, const FVector2D& PB,
                                const FVector2D& CA, const FVector2D& CB,
                                const FVector2D& P) const;
};