#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "PhysicsCalibrator.generated.h"

UENUM(BlueprintType)
enum class ECalibrationPhase : uint8
{
	// Phase 1: binary search angular damping to match ramp time
	SearchingRampDamping,
	// Phase 2: binary search flat friction to match stop distance
	SearchingFlatFriction,
	// Brief pause between runs to let physics settle
	ResettingBall,
	Complete
};

UENUM(BlueprintType)
enum class ERunPhase : uint8
{
	OnRamp,
	OnFlat
};

USTRUCT(BlueprintType)
struct FPhaseResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) float Value = 0.f; // The parameter value tested
	UPROPERTY(BlueprintReadOnly) float Measured = 0.f; // What we measured
	UPROPERTY(BlueprintReadOnly) float Target = 0.f; // What we wanted
	UPROPERTY(BlueprintReadOnly) float Error = 0.f; // Measured - Target
};

UCLASS()
class VRGOLFTESTING_API APhysicsCalibrator : public AActor
{
	GENERATED_BODY()

public:
	APhysicsCalibrator();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;

	// ── Walkabout ground truth ───────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Calibration|Targets")
	float TargetBottomTime = 1.74f;   // seconds to reach bottom of ramp

	UPROPERTY(EditAnywhere, Category = "Calibration|Targets")
	float TargetStopDistance = 640.08f; // cm from bottom of ramp on flat

	UPROPERTY(EditAnywhere, Category = "Calibration|Targets")
	float TargetStopTime = 6.03f;   // total time (used for orange ghost only)

	// ── Phase 1: angular damping binary search ───────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Calibration|Phase1 Ramp")
	float DampingSearchMin = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Calibration|Phase1 Ramp")
	float DampingSearchMax = 15.0f;

	// Stop when bottom time is within this tolerance of target (seconds)
	UPROPERTY(EditAnywhere, Category = "Calibration|Phase1 Ramp")
	float DampingTimeTolerance = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Calibration|Phase1 Ramp")
	int32 DampingMaxIterations = 20;

	// ── Phase 2: flat friction binary search ─────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Calibration|Phase2 Flat")
	float FrictionSearchMin = 0.005f;

	UPROPERTY(EditAnywhere, Category = "Calibration|Phase2 Flat")
	float FrictionSearchMax = 0.4f;

	// Stop when stop distance is within this tolerance of target (cm)
	UPROPERTY(EditAnywhere, Category = "Calibration|Phase2 Flat")
	float FrictionDistTolerance = 5.0f; // cm

	UPROPERTY(EditAnywhere, Category = "Calibration|Phase2 Flat")
	int32 FrictionMaxIterations = 20;

	// ── Ball locked values ───────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Calibration|BallLock")
	float BallFriction = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Calibration|BallLock")
	float BallStaticFriction = 1.0f;

	UPROPERTY(EditAnywhere, Category = "Calibration|BallLock")
	float BallRestitution = 0.25f;

	UPROPERTY(EditAnywhere, Category = "Calibration|BallLock")
	float BallLinearDamping = 0.01f;

	UPROPERTY(EditAnywhere, Category = "Calibration|BallLock")
	TObjectPtr<UPhysicalMaterial> BallPhysicalMaterial;

	// ── Slope physical material (low friction, tuned separately) ────────────
	UPROPERTY(EditAnywhere, Category = "Calibration|Scene")
	TObjectPtr<UPhysicalMaterial> SlopePhysicalMaterial;

	// ── Flat physical material (friction is phase 2 search variable) ─────────
	UPROPERTY(EditAnywhere, Category = "Calibration|Scene")
	TObjectPtr<UPhysicalMaterial> FlatPhysicalMaterial;

	// Slope friction locked during phase 1 and 2
	UPROPERTY(EditAnywhere, Category = "Calibration|Scene")
	float LockedSlopeFriction = 0.05f;

	// ── Scene refs ───────────────────────────────────────────────────────────
	UPROPERTY(EditAnywhere, Category = "Calibration|Scene")
	TObjectPtr<AActor> BallActor;

	UPROPERTY(EditAnywhere, Category = "Calibration|Scene")
	TObjectPtr<AActor> BottomMarkerActor;

	// ── Bail-out ─────────────────────────────────────────────────────────────
	// Phase 1: if ramp takes longer than this, damping is too high
	UPROPERTY(EditAnywhere, Category = "Calibration|BailOut")
	float RampMaxTime = 8.0f;

	// Phase 2: if ball travels beyond this multiple of target, friction too low
	UPROPERTY(EditAnywhere, Category = "Calibration|BailOut")
	float FlatOvershootRatio = 1.4f;

	// Phase 2 max run time
	UPROPERTY(EditAnywhere, Category = "Calibration|BailOut")
	float FlatMaxTime = 20.0f;

	// ── Runtime ──────────────────────────────────────────────────────────────
	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	ECalibrationPhase Phase = ECalibrationPhase::SearchingRampDamping;

	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	ERunPhase RunPhase = ERunPhase::OnRamp;

	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	int32 CurrentIteration = 0;

	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	float CurrentDamping = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	float CurrentFlatFriction = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	float ElapsedTime = 0.f;

	// Final locked values after each phase
	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	float LockedAngularDamping = 0.f;

	UPROPERTY(VisibleInstanceOnly, Category = "Calibration|Runtime")
	float LockedFlatFriction = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Calibration|Runtime")
	FPhaseResult Phase1Result;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Calibration|Runtime")
	FPhaseResult Phase2Result;

private:
	TObjectPtr<UPrimitiveComponent> BallPrimitive = nullptr;
	FTransform BallStartTransform;      // top of ramp
	FTransform BallFlatStartTransform;  // bottom of ramp — phase 2 start
	FVector    BottomMarkerLocation = FVector::ZeroVector;

	// Velocity the ball had when it reached the bottom in phase 1
	// Used to inject the ball at the correct speed for phase 2
	float      Phase1ExitVelocity = 0.f;

	// Binary search bounds
	float BinaryLow = 0.f;
	float BinaryHigh = 0.f;

	// Stop detection
	float StopDetectionTimer = 0.f;
	float MaxObservedFlatDistance = 0.f;
	float ResetTimer = 0.f;
	bool  bPassedBottomMarker = false;

	// Debug draw
	FVector BestStopLocation = FVector::ZeroVector;
	bool    bHasBestStop = false;

	static constexpr float ResetDelay = 0.5f;
	static constexpr float StopVelocityThreshold = 2.0f;
	static constexpr float StopConfirmDuration = 0.3f;

	void  StartPhase1();
	void  StartPhase2();
	void  StartRun();
	void  StartFlatRun(float InjectedVelocity);
	void  OnRampRunComplete(float MeasuredBottomTime, float ExitVelocity);
	void  OnFlatRunComplete(float MeasuredStopDistance);
	void  LockBallParameters();
	void  ApplySlopeMaterial();
	void  ApplyFlatMaterial(float Friction);
	void  ResetBallToRampStart();
	void  ResetBallToFlatStart(float InjectedVelocity);
	void  OnComplete();
	void  DrawDebugOverlay() const;
	bool  IsBallStopped(float DeltaTime);
	bool  HasBallPassedMarker() const;
	UPrimitiveComponent* FindBallPrimitive() const;
};