#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PlaytestMathSubsystem.generated.h"

/**
 * Stateless math utility subsystem for playtest agent shot calculations.
 * Flat/planar assumptions only for now — no banking, jumps, or obstacles.
 */
UCLASS()
class VRGOLF_API UPlaytestMathSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    // -------------------------------------------------------------------------
    // Projection Utilities
    // -------------------------------------------------------------------------

    /** Project a world-space point onto a plane defined by an origin and normal. */
    UFUNCTION(BlueprintCallable, Category = "Playtest|Math")
    static FVector ProjectPointOntoPlane(const FVector& Point, const FVector& PlaneOrigin, const FVector& PlaneNormal);

    /** Project a world-space direction vector onto a plane normal (removes component along normal). */
    UFUNCTION(BlueprintCallable, Category = "Playtest|Math")
    static FVector ProjectDirectionOntoPlane(const FVector& Direction, const FVector& PlaneNormal);

    /** Project both Tee and Hole onto a shared plane before any shot calculation. */
    UFUNCTION(BlueprintCallable, Category = "Playtest|Math")
    static void ProjectTeeAndHoleOntoPlane(
        const FVector& Tee, const FVector& Hole,
        const FVector& PlaneOrigin, const FVector& PlaneNormal,
        FVector& OutProjectedTee, FVector& OutProjectedHole);

    // -------------------------------------------------------------------------
    // Shot Calculation — Flat Connected Surface, No Obstacles
    // -------------------------------------------------------------------------

    /**
     * Calculate the impulse force vector to putt a ball from Tee to Hole
     * on a flat, connected surface.
     *
     * @param Tee            World position of the ball at stroke time.
     * @param Hole           World position of the hole center.
     * @param PlaneNormal    Surface normal of the putting plane (need not be up).
     * @param FrictionCoeff  Rolling friction coefficient for the surface material.
     * @param BallMass       Mass of the ball in kg (matches physics asset).
     * @return               Impulse vector in world space (direction + magnitude).
     *
     * Assumptions:
     *   - Surface is planar and continuous between Tee and Hole.
     *   - No elevation change along the shot path (flat plane).
     *   - Ball decelerates uniformly under rolling friction: a = -μg
     *   - Required speed derived from kinematics: v² = 2μg·d  (v_final = 0 at hole)
     *   - Impulse = mass * velocity (applied as single frame impulse to physics body).
     */
    UFUNCTION(BlueprintCallable, Category = "Playtest|Math")
    static FVector CalculateForceOnFlat(
        const FVector& Tee,
        const FVector& Hole,
        const FVector& PlaneNormal,
        float FrictionCoeff,
        float BallMass);

    /**
     * Returns just the required launch speed (cm/s) to reach the hole on flat surface.
     * Useful for debugging and stat logging independently of direction.
     */
    UFUNCTION(BlueprintCallable, Category = "Playtest|Math")
    static float CalculateFlatShotSpeed(float DistanceCm, float FrictionCoeff);

    // -------------------------------------------------------------------------
    // Helpers
    // -------------------------------------------------------------------------

    /** Flat distance between Tee and Hole projected onto the putting plane. */
    UFUNCTION(BlueprintCallable, Category = "Playtest|Math")
    static float GetPuttingDistance(const FVector& Tee, const FVector& Hole, const FVector& PlaneNormal);

    /** Direction from Tee to Hole, projected onto the putting plane, normalized. */
    UFUNCTION(BlueprintCallable, Category = "Playtest|Math")
    static FVector GetPuttingDirection(const FVector& Tee, const FVector& Hole, const FVector& PlaneNormal);
};