#include "Subsystems/PlaytestMathSubsystem.h"

// UE gravity in cm/s² (9.81 m/s² * 100)
static constexpr float GravityCmS2 = 980.f;

// -------------------------------------------------------------------------
// Projection Utilities
// -------------------------------------------------------------------------

FVector UPlaytestMathSubsystem::ProjectPointOntoPlane(
    const FVector& Point, const FVector& PlaneOrigin, const FVector& PlaneNormal)
{
    const FVector N = PlaneNormal.GetSafeNormal();
    const float Dist = FVector::DotProduct(Point - PlaneOrigin, N);
    return Point - Dist * N;
}

FVector UPlaytestMathSubsystem::ProjectDirectionOntoPlane(
    const FVector& Direction, const FVector& PlaneNormal)
{
    const FVector N = PlaneNormal.GetSafeNormal();
    return (Direction - FVector::DotProduct(Direction, N) * N).GetSafeNormal();
}

void UPlaytestMathSubsystem::ProjectTeeAndHoleOntoPlane(
    const FVector& Tee, const FVector& Hole,
    const FVector& PlaneOrigin, const FVector& PlaneNormal,
    FVector& OutProjectedTee, FVector& OutProjectedHole)
{
    OutProjectedTee  = ProjectPointOntoPlane(Tee,  PlaneOrigin, PlaneNormal);
    OutProjectedHole = ProjectPointOntoPlane(Hole, PlaneOrigin, PlaneNormal);
}

// -------------------------------------------------------------------------
// Shot Calculation
// -------------------------------------------------------------------------

float UPlaytestMathSubsystem::CalculateFlatShotSpeed(float DistanceCm, float FrictionCoeff)
{
    // Kinematic: v² = 2 * a * d,  a = μg,  solve for v
    // v = sqrt(2 * μ * g * d)
    if (DistanceCm <= 0.f || FrictionCoeff <= 0.f)
    {
        UE_LOG(LogTemp, Warning, TEXT("CalculateFlatShotSpeed: Invalid distance or friction. Returning 0."));
        return 0.f;
    }

    return FMath::Sqrt(2.f * FrictionCoeff * GravityCmS2 * DistanceCm);
}

FVector UPlaytestMathSubsystem::CalculateForceOnFlat(
    const FVector& Tee,
    const FVector& Hole,
    const FVector& PlaneNormal,
    float FrictionCoeff,
    float BallMass)
{
    // Project both points onto the putting plane
    // Use Tee as the plane origin (it's on the surface)
    FVector ProjectedTee, ProjectedHole;
    ProjectTeeAndHoleOntoPlane(Tee, Hole, Tee, PlaneNormal, ProjectedTee, ProjectedHole);

    const float Distance = FVector::Dist(ProjectedTee, ProjectedHole);
    if (Distance < KINDA_SMALL_NUMBER)
    {
        UE_LOG(LogTemp, Warning, TEXT("CalculateForceOnFlat: Tee and Hole are coincident."));
        return FVector::ZeroVector;
    }

    // Direction along the putting plane toward hole
    const FVector ShotDirection = (ProjectedHole - ProjectedTee).GetSafeNormal();

    // Required launch speed for ball to just reach hole under rolling friction
    const float LaunchSpeed = CalculateFlatShotSpeed(Distance, FrictionCoeff);

    // Impulse = mass * velocity
    // (caller applies this via AddImpulse on the ball's primitive component)
    return ShotDirection * LaunchSpeed * BallMass;
}

// -------------------------------------------------------------------------
// Helpers
// -------------------------------------------------------------------------

float UPlaytestMathSubsystem::GetPuttingDistance(
    const FVector& Tee, const FVector& Hole, const FVector& PlaneNormal)
{
    FVector ProjTee, ProjHole;
    ProjectTeeAndHoleOntoPlane(Tee, Hole, Tee, PlaneNormal, ProjTee, ProjHole);
    return FVector::Dist(ProjTee, ProjHole);
}

FVector UPlaytestMathSubsystem::GetPuttingDirection(
    const FVector& Tee, const FVector& Hole, const FVector& PlaneNormal)
{
    FVector ProjTee, ProjHole;
    ProjectTeeAndHoleOntoPlane(Tee, Hole, Tee, PlaneNormal, ProjTee, ProjHole);
    return (ProjHole - ProjTee).GetSafeNormal();
}