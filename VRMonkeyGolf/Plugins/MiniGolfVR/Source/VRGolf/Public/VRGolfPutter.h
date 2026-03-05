#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRGolfPutter.generated.h"

// ============================================================
//  Structs
// ============================================================

/**
 * Data delivered every tick when the club pivot is near/facing the putting surface.
 * Consume in BP via OnSurfaceUpdate override.
 */
USTRUCT(BlueprintType)
struct FPutterSurfaceData
{
	GENERATED_BODY()

	/** True when the club pivot is within range and the surface plane was resolved. */
	UPROPERTY(BlueprintReadOnly)
	bool bSurfaceFound = false;

	/** Club pivot projected onto the tee plane (snapped to surface height). */
	UPROPERTY(BlueprintReadOnly)
	FVector ProjectedClubPosition = FVector::ZeroVector;

	/** Normal of the putting surface under the ball (world up on flat lies). */
	UPROPERTY(BlueprintReadOnly)
	FVector SurfaceNormal = FVector::UpVector;

	/** Direction from projected club position to ball, on the plane (normalized). */
	UPROPERTY(BlueprintReadOnly)
	FVector DirectionToBall = FVector::ZeroVector;

	/** Distance from projected club position to ball, measured on the plane. */
	UPROPERTY(BlueprintReadOnly)
	float DistanceToBall = 0.f;

	/** Raw motion controller world transform this tick. */
	UPROPERTY(BlueprintReadOnly)
	FTransform HandTransform;
};


/**
 * Data delivered at the moment the putter pivot overlaps the ball.
 * Consume in BP via OnStrike override.
 */
USTRUCT(BlueprintType)
struct FPutterStrikeData
{
	GENERATED_BODY()

	/** Raw controller velocity projected onto delta-position (world space, not plane-clamped). */
	UPROPERTY(BlueprintReadOnly)
	FVector RawHitForce = FVector::ZeroVector;

	/** RawHitForce with vertical component removed — aligned to the putting surface plane. */
	UPROPERTY(BlueprintReadOnly)
	FVector SurfaceAlignedForce = FVector::ZeroVector;

	/**
	 * SurfaceAlignedForce magnitude clamped to [ForceClampMin, ForceClampMax].
	 * TODO: Wire ForceClampMin / ForceClampMax to a project-level settings asset.
	 * Currently mirrors SurfaceAlignedForce — change the clamp values to tune feel.
	 */
	UPROPERTY(BlueprintReadOnly)
	FVector ClampedSurfaceAlignedForce = FVector::ZeroVector;

	/** World position of the putter pivot at the moment of contact. */
	UPROPERTY(BlueprintReadOnly)
	FVector RawHitLocation = FVector::ZeroVector;

	/** Putter pivot snapped to the tee plane at the moment of contact. */
	UPROPERTY(BlueprintReadOnly)
	FVector BallAlignedHitLocation = FVector::ZeroVector;

	/** The ball actor that was struck. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> HitBall = nullptr;

	/** This putter actor. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Club = nullptr;

	/** The owning player controller. */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<APlayerController> Player = nullptr;

	/** The surface actor the ball is resting on (can be null if not found). */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> PuttingSurface = nullptr;

	/** Raw motion controller world transform at the moment of contact. */
	UPROPERTY(BlueprintReadOnly)
	FTransform HandTransform;
};


// ============================================================
//  AVRGolfPutter
// ============================================================

/**
 * Logical putter actor — no mesh, no visuals, no feel.
 *
 * Attach to a Motion Controller Component on the player pawn.
 * Set TeeTransform (or call SetTeeTransform at runtime) whenever a new hole begins.
 *
 * Override the two BlueprintImplementableEvents in a child BP to handle
 * all visual, haptic, and force-application logic yourself.
 */
UCLASS(Blueprintable, BlueprintType)
class VRGOLF_API AVRGolfPutter : public AActor
{
	GENERATED_BODY()

public:

	AVRGolfPutter();

	// --------------------------------------------------------
	//  Configuration
	// --------------------------------------------------------

	/**
	 * Normal of the putting surface under the current ball.
	 * Defaults to world up (flat green).  Override per-hole for slopes.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Surface")
	FVector TeeNormal = FVector::UpVector;

	/**
	 * World position of the ball on its tee.
	 * Used to define the putting plane and compute distance/direction.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Surface")
	FVector BallPosition = FVector::ZeroVector;

	/**
	 * Radius of the logical overlap sphere around the putter pivot.
	 * No visual representation — tune to match whatever mesh you attach in BP.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Collision", meta = (ClampMin = "0.5", ClampMax = "20.0"))
	float StrikeRadius = 4.f;

	/**
	 * Minimum dot product between putter-to-ball direction and surface normal
	 * (inverted) for OnSurfaceUpdate to fire.  Keeps the callback silent when
	 * the club is nowhere near the surface plane.
	 * 0 = fire always, 1 = only fire when perfectly aligned.
	 * TODO: tune this threshold during playtesting.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Surface", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SurfaceAlignmentThreshold = 0.1f;

	/**
	 * Minimum clamped force magnitude (cm/s equivalent after scaling).
	 * TODO: move to a shared project settings asset alongside ForceClampMax.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Force", meta = (ClampMin = "0.0"))
	float ForceClampMin = 0.f;

	/**
	 * Maximum clamped force magnitude.
	 * TODO: move to a shared project settings asset alongside ForceClampMin.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Force", meta = (ClampMin = "0.0"))
	float ForceClampMax = 1000.f;

	// --------------------------------------------------------
	//  Runtime API
	// --------------------------------------------------------

	/** Call at the start of each hole to update the tee plane. */
	UFUNCTION(BlueprintCallable, Category = "Putter")
	void SetTeeTransform(FVector InBallPosition, FVector InSurfaceNormal);

	// --------------------------------------------------------
	//  BP Implementable Events  (override these — C++ just logs)
	// --------------------------------------------------------

	/**
	 * Fired every tick the club pivot is near the putting surface.
	 * Override in BP to position your mesh, show aim indicators, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Putter|Events")
	void OnSurfaceUpdate(const FPutterSurfaceData& SurfaceData);
	virtual void OnSurfaceUpdate_Implementation(const FPutterSurfaceData& SurfaceData);

	/**
	 * Fired once when the putter pivot enters the ball's strike radius.
	 * Override in BP to apply force to the ball, trigger haptics, play VFX, etc.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Putter|Events")
	void OnStrike(const FPutterStrikeData& StrikeData);
	virtual void OnStrike_Implementation(const FPutterStrikeData& StrikeData);

	
	void AttachToMotionController(USceneComponent* MotionController);


	UFUNCTION(BlueprintNativeEvent, Category = "Putter|Events")
	void OnAttachedToMC(USceneComponent* MotionController);
	virtual void OnAttachedToMC_Implementation(USceneComponent* MotionController) {}


protected:

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:

	// --------------------------------------------------------
	//  Internal state
	// --------------------------------------------------------

	/** Pivot scene component — attach your MC component to this in BP. */
	UPROPERTY(VisibleAnywhere, Category = "Putter|Internal")
	TObjectPtr<USceneComponent> PutterPivot;

	/** Position of the pivot last tick (world space) — used for velocity. */
	FVector LastPivotPosition;

	/** Velocity of the projected (plane-snapped) pivot this tick. */
	FVector ProjectedVelocity;

	/** True after the first tick so LastPivotPosition is valid. */
	bool bHasLastPosition = false;

	/** Guard against firing OnStrike multiple times per swing. */
	bool bStrikeConsumed = false;

	// --------------------------------------------------------
	//  Helpers
	// --------------------------------------------------------

	/** Project WorldPos onto the plane defined by BallPosition + TeeNormal. */
	FVector ProjectOntoTeePlane(const FVector& WorldPos) const;

	/** Build and fire OnSurfaceUpdate if alignment threshold is met. */
	void TickSurfaceUpdate(float DeltaTime);

	/** Check overlap and fire OnStrike once per swing. */
	void TickStrikeCheck();

	/** Resolve the surface actor under the ball via a simple line trace downward. */
	AActor* ResolvePuttingSurface() const;




	public:
	// --------------------------------------------------------
	//  Debug — Master Gate
	// --------------------------------------------------------

	/** Kill switch for all debug drawing. Individual toggles below have no effect when this is false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Debug")
	bool bDebugEnabled = false;

	// --------------------------------------------------------
	//  Debug — Per-Category Toggles
	// --------------------------------------------------------

	/** Draw the projected putter line (handle → head) each tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bDebugShowPutterLine = true;

	/** Draw the putting plane quad each tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bDebugShowPuttPlane = true;

	/** Draw the strike zone sphere each tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bDebugShowStrikeZone = true;

	/** Draw raw and surface-aligned force vectors on strike. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bDebugShowStrikeVectors = true;

	/** Draw the full SurfaceData composite each tick (direction to ball, projected position, normal). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Putter|Debug", meta = (EditCondition = "bDebugEnabled"))
	bool bDebugShowSurfaceData = true;

	// --------------------------------------------------------
	//  Debug — Primitive Functions (callable from BP)
	// --------------------------------------------------------

	/** Draw a vector arrow from Origin. */
	UFUNCTION(BlueprintCallable, Category = "Putter|Debug")
	void VisualizePuttVector(FVector Origin, FVector Vector, FLinearColor Color, float Duration) const;

	/**
	 * Draw a flat quad representing the putting plane.
	 * Extents = half-size in each axis of the plane (cm).
	 */
	UFUNCTION(BlueprintCallable, Category = "Putter|Debug")
	void VisualizePuttPlane(FVector Origin, FVector PlaneNormal, FLinearColor Color, FVector2D Extents) const;

	/**
	 * Draw a line from Handle to Head with spheres at each end.
	 * Mirrors the logical putter shaft for positional debugging.
	 */
	UFUNCTION(BlueprintCallable, Category = "Putter|Debug")
	void VisualizeProjectedPutterLine(FVector Handle, FVector Head, FLinearColor Color, float Duration,
		float EndSphereRadius, float StartSphereRadius, float Thickness) const;

	/** Draw the strike detection sphere around the putter pivot. */
	UFUNCTION(BlueprintCallable, Category = "Putter|Debug")
	void VisualizePutterStrikeZone(FLinearColor Color, float Duration) const;

	// --------------------------------------------------------
	//  Debug — Composite Functions (callable from BP)
	// --------------------------------------------------------

	/** Draw all relevant vectors and positions from a SurfaceData snapshot. */
	UFUNCTION(BlueprintCallable, Category = "Putter|Debug")
	void VisualizePutterSurfaceData(const FPutterSurfaceData& SurfaceData) const;

	/** Draw all force vectors and hit locations from a StrikeData snapshot. */
	UFUNCTION(BlueprintCallable, Category = "Putter|Debug")
	void VisualizePutterStrike(const FPutterStrikeData& StrikeData) const;
};