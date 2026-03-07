// ClubSegmentHelper.h
#pragma once

#include "CoreMinimal.h"
#include "Components/ClubLineSegment.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ClubSegmentHelper.generated.h"

class AVRGolfBall;
class UBoxComponent;

/**
 * UClubSegmentHelper
 *
 * Blueprint Function Library.
 * Owns face selection (box + ball -> two 3D corners) and wraps the full
 * pipeline into a single BP-callable function for validation.
 *
 * Test from BP:
 *   Call BuildAndDraw every tick with your box and ball references.
 *   If the yellow segment and orange normal appear correctly positioned
 *   at ball-center height, the geometry is correct.
 */
UCLASS()
class VRGOLF_API UClubSegmentHelper : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:

    /**
     * Selects the box face closest to the ball, extracts its two bottom
     * corners, and returns a fully constructed FClubLineSegment.
     *
     * Returns an invalid segment (bValid=false) if Box or Ball is null.
     */
    UFUNCTION(BlueprintCallable, Category = "Club|Segment")
    static FClubLineSegment BuildSegment(UBoxComponent* Box, AVRGolfBall* Ball, FVector SwingDirection);
    
    /**
     * Builds the segment and immediately draws it in world space.
     * Call every tick from BP to visually validate geometry before
     * wiring into stroke detection.
     *
     * Returns the segment so you can inspect it in BP if needed.
     */
    UFUNCTION(BlueprintCallable, Category = "Club|Segment")
    static FClubLineSegment BuildAndDraw(UObject* WorldContext, UBoxComponent* Box, AVRGolfBall* Ball, FVector SwingDirection);
    
    /**
     * Tests whether CurSegment crossing the ball (using PrevSegment as the
     * previous frame position). Returns true and sets OutT if a crossing occurred.
     */
    UFUNCTION(BlueprintCallable, Category = "Club|Segment")
    static bool TestCrossing(const FClubLineSegment& PrevSegment,
                             const FClubLineSegment& CurSegment,
                             AVRGolfBall* Ball,
                             float& OutT, FVector2D& OutImpulseDirection);

    /**
     * Returns the ball radius from its SphereComponent.
     * Falls back to 2.135 (standard golf ball cm) if not found.
     */
    UFUNCTION(BlueprintPure, Category = "Club|Segment")
    static float GetBallRadius(AVRGolfBall* Ball);

private:

    /**
     * Selects the two bottom corners of the box face most aligned with the
     * direction from box center to ball (floor-projected).
     *
     * @param Box           Source box component
     * @param Ball          Ball actor — used for direction, not null
     * @param OutCornerA    First bottom corner (world space)
     * @param OutCornerB    Second bottom corner (world space)
     * @return              False if selection failed
     */
    static bool SelectFaceCorners(UBoxComponent* Box, const FVector& SwingDirection,
                                  FVector& OutCornerA, FVector& OutCornerB);
};