#include "PhysicsCalibrator.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SphereComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

APhysicsCalibrator::APhysicsCalibrator()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

// ────────────────────────────────────────────────────────────────────────────
void APhysicsCalibrator::BeginPlay()
{
	Super::BeginPlay();

	BallPrimitive = FindBallPrimitive();
	if (!BallPrimitive)
	{
		UE_LOG(LogTemp, Error, TEXT("PhysicsCalibrator: No UPrimitiveComponent on BallActor."));
		return;
	}

	BallStartTransform = BallActor->GetActorTransform();
	if (BottomMarkerActor)
		BottomMarkerLocation = BottomMarkerActor->GetActorLocation();

	// Phase 2 flat start = bottom marker location, same rotation as ball start
	FTransform FlatStart = BallStartTransform;
	FlatStart.SetLocation(BottomMarkerLocation);
	BallFlatStartTransform = FlatStart;

	StartPhase1();
}

// ────────────────────────────────────────────────────────────────────────────
void APhysicsCalibrator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!BallPrimitive || Phase == ECalibrationPhase::Complete)
	{
		DrawDebugOverlay();
		return;
	}

	// ── Reset pause ───────────────────────────────────────────────────────────
	if (Phase == ECalibrationPhase::ResettingBall)
	{
		ResetTimer += DeltaTime;
		if (ResetTimer >= ResetDelay)
		{
			ResetTimer = 0.f;
			if (RunPhase == ERunPhase::OnRamp)
				StartRun();
			else
				StartFlatRun(Phase1ExitVelocity);
		}
		DrawDebugOverlay();
		return;
	}

	ElapsedTime += DeltaTime;
	const FVector BallLoc = BallActor->GetActorLocation();

	// ── Phase 1: ramp run ─────────────────────────────────────────────────────
	if (Phase == ECalibrationPhase::SearchingRampDamping)
	{
		// Bail: taking too long — damping too high
		if (ElapsedTime > RampMaxTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("[P1 iter %d] Timeout — damping %.3f too high"), CurrentIteration, CurrentDamping);
			OnRampRunComplete(RampMaxTime, 0.f);
			return;
		}

		if (!bPassedBottomMarker && HasBallPassedMarker())
		{
			bPassedBottomMarker = true;
			const float ExitSpeed = BallPrimitive->GetPhysicsLinearVelocity().Size();
			UE_LOG(LogTemp, Log, TEXT("[P1 iter %d] Bottom at %.3fs (target %.2f)  exitVel=%.1f cm/s"),
				CurrentIteration, ElapsedTime, TargetBottomTime, ExitSpeed);
			OnRampRunComplete(ElapsedTime, ExitSpeed);
		}
	}

	// ── Phase 2: flat run ─────────────────────────────────────────────────────
	else if (Phase == ECalibrationPhase::SearchingFlatFriction)
	{
		const float FlatDist = FVector::Dist2D(BallLoc, BottomMarkerLocation);
		MaxObservedFlatDistance = FMath::Max(MaxObservedFlatDistance, FlatDist);

		// Bail: overshoot
		if (MaxObservedFlatDistance > TargetStopDistance * FlatOvershootRatio)
		{
			UE_LOG(LogTemp, Warning, TEXT("[P2 iter %d] Overshoot %.1f cm — friction %.4f too low"),
				CurrentIteration, MaxObservedFlatDistance, CurrentFlatFriction);
			OnFlatRunComplete(MaxObservedFlatDistance);
			return;
		}

		// Bail: timeout
		if (ElapsedTime > FlatMaxTime)
		{
			UE_LOG(LogTemp, Warning, TEXT("[P2 iter %d] Timeout — friction %.4f too low (still rolling)"),
				CurrentIteration, CurrentFlatFriction);
			OnFlatRunComplete(TargetStopDistance * FlatOvershootRatio);
			return;
		}

		if (IsBallStopped(DeltaTime))
		{
			const float StopDist = FVector::Dist2D(BallLoc, BottomMarkerLocation);
			UE_LOG(LogTemp, Log, TEXT("[P2 iter %d] Stopped at %.1f cm (target %.1f cm)  t=%.2fs"),
				CurrentIteration, StopDist, TargetStopDistance, ElapsedTime);
			OnFlatRunComplete(StopDist);
		}
	}

	// ── HUD ───────────────────────────────────────────────────────────────────
	if (GEngine)
	{
		const bool bIsPhase1 = Phase == ECalibrationPhase::SearchingRampDamping;
		GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::Yellow,
			FString::Printf(TEXT("%s  |  Iter %d  |  %s=%.4f  |  t=%.2fs"),
				bIsPhase1 ? TEXT("PHASE 1 — Ramp Damping") : TEXT("PHASE 2 — Flat Friction"),
				CurrentIteration,
				bIsPhase1 ? TEXT("AngDamp") : TEXT("Friction"),
				bIsPhase1 ? CurrentDamping : CurrentFlatFriction,
				ElapsedTime));

		if (bIsPhase1)
			GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan,
				FString::Printf(TEXT("Binary: [%.3f — %.3f]  target bottom time: %.2fs"),
					BinaryLow, BinaryHigh, TargetBottomTime));
		else
			GEngine->AddOnScreenDebugMessage(2, 0.f, FColor::Cyan,
				FString::Printf(TEXT("Binary: [%.4f — %.4f]  target stop dist: %.1f cm"),
					BinaryLow, BinaryHigh, TargetStopDistance));
	}

	DrawDebugOverlay();
}

// ────────────────────────────────────────────────────────────────────────────
// Phase 1 — Ramp: binary search angular damping
// ────────────────────────────────────────────────────────────────────────────

void APhysicsCalibrator::StartPhase1()
{
	Phase = ECalibrationPhase::SearchingRampDamping;
	RunPhase = ERunPhase::OnRamp;
	BinaryLow = DampingSearchMin;
	BinaryHigh = DampingSearchMax;
	CurrentIteration = 0;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan,
			TEXT("═══ PHASE 1: Searching angular damping for ramp ═══"));

	UE_LOG(LogTemp, Log, TEXT("PhysicsCalibrator: Phase 1 start — binary search damping [%.2f, %.2f]"),
		DampingSearchMin, DampingSearchMax);

	StartRun();
}

void APhysicsCalibrator::StartRun()
{
	CurrentDamping = (BinaryLow + BinaryHigh) * 0.5f;
	ElapsedTime = 0.f;
	bPassedBottomMarker = false;
	++CurrentIteration;

	LockBallParameters();
	ApplySlopeMaterial();

	// Flat material during phase 1 — lock to low friction so it doesn't interfere
	ApplyFlatMaterial(LockedSlopeFriction);

	if (BallPrimitive)
		BallPrimitive->SetAngularDamping(CurrentDamping);

	ResetBallToRampStart();
	Phase = ECalibrationPhase::SearchingRampDamping;
	RunPhase = ERunPhase::OnRamp;

	UE_LOG(LogTemp, Log, TEXT("[P1 iter %d] Testing damping=%.4f  bounds=[%.4f, %.4f]"),
		CurrentIteration, CurrentDamping, BinaryLow, BinaryHigh);
}

void APhysicsCalibrator::OnRampRunComplete(float MeasuredBottomTime, float ExitVelocity)
{
	const float Error = MeasuredBottomTime - TargetBottomTime;

	// Binary search: too slow (time too high) = damping too high = lower the high bound
	//                too fast (time too low)  = damping too low  = raise the low bound
	if (MeasuredBottomTime > TargetBottomTime)
		BinaryHigh = CurrentDamping;
	else
		BinaryLow = CurrentDamping;

	const bool bConverged = FMath::Abs(Error) <= DampingTimeTolerance;
	const bool bExhausted = CurrentIteration >= DampingMaxIterations;

	if (bConverged || bExhausted)
	{
		LockedAngularDamping = CurrentDamping;
		Phase1ExitVelocity = ExitVelocity > 0.f ? ExitVelocity : 0.f;

		Phase1Result.Value = CurrentDamping;
		Phase1Result.Measured = MeasuredBottomTime;
		Phase1Result.Target = TargetBottomTime;
		Phase1Result.Error = Error;

		UE_LOG(LogTemp, Log,
			TEXT("Phase 1 COMPLETE — AngularDamping=%.4f  bottomTime=%.3f (target %.2f)  exitVel=%.1f cm/s  %s"),
			LockedAngularDamping, MeasuredBottomTime, TargetBottomTime, Phase1ExitVelocity,
			bConverged ? TEXT("CONVERGED") : TEXT("MAX ITERATIONS"));

		if (GEngine)
			GEngine->AddOnScreenDebugMessage(-1, 10.f, FColor::Green,
				FString::Printf(TEXT("Phase 1 done — AngDamp=%.4f  bottomTime=%.3f  → Phase 2"),
					LockedAngularDamping, MeasuredBottomTime));

		Phase = ECalibrationPhase::ResettingBall;
		ResetTimer = 0.f;

		// Next reset will call StartFlatRun via RunPhase flag
		RunPhase = ERunPhase::OnFlat;

		// Small delay then start phase 2
		// We use the reset mechanism — override it to call StartPhase2 instead
		// by setting a flag handled in the reset block
		StartPhase2();
		return;
	}

	// Not converged — reset and try next binary midpoint
	Phase = ECalibrationPhase::ResettingBall;
	RunPhase = ERunPhase::OnRamp;
	ResetTimer = 0.f;
}

// ────────────────────────────────────────────────────────────────────────────
// Phase 2 — Flat: binary search flat friction
// ────────────────────────────────────────────────────────────────────────────

void APhysicsCalibrator::StartPhase2()
{
	Phase = ECalibrationPhase::SearchingFlatFriction;
	RunPhase = ERunPhase::OnFlat;
	BinaryLow = FrictionSearchMin;
	BinaryHigh = FrictionSearchMax;
	CurrentIteration = 0;

	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 6.f, FColor::Cyan,
			TEXT("═══ PHASE 2: Searching flat friction ═══"));

	UE_LOG(LogTemp, Log, TEXT("PhysicsCalibrator: Phase 2 start — binary search friction [%.4f, %.4f]  injectedVel=%.1f cm/s"),
		FrictionSearchMin, FrictionSearchMax, Phase1ExitVelocity);

	StartFlatRun(Phase1ExitVelocity);
}

void APhysicsCalibrator::StartFlatRun(float InjectedVelocity)
{
	CurrentFlatFriction = (BinaryLow + BinaryHigh) * 0.5f;
	ElapsedTime = 0.f;
	MaxObservedFlatDistance = 0.f;
	StopDetectionTimer = 0.f;
	++CurrentIteration;

	LockBallParameters();
	ApplySlopeMaterial();
	ApplyFlatMaterial(CurrentFlatFriction);

	if (BallPrimitive)
		BallPrimitive->SetAngularDamping(LockedAngularDamping);

	ResetBallToFlatStart(InjectedVelocity);
	Phase = ECalibrationPhase::SearchingFlatFriction;
	RunPhase = ERunPhase::OnFlat;

	UE_LOG(LogTemp, Log, TEXT("[P2 iter %d] Testing friction=%.5f  bounds=[%.5f, %.5f]  vel=%.1f cm/s"),
		CurrentIteration, CurrentFlatFriction, BinaryLow, BinaryHigh, InjectedVelocity);
}

void APhysicsCalibrator::OnFlatRunComplete(float MeasuredStopDistance)
{
	const float Error = MeasuredStopDistance - TargetStopDistance;

	// Binary search: went too far = friction too low  = raise low bound
	//                stopped short = friction too high = lower high bound
	if (MeasuredStopDistance > TargetStopDistance)
		BinaryLow = CurrentFlatFriction;
	else
		BinaryHigh = CurrentFlatFriction;

	// Record best stop location for debug draw
	BestStopLocation = BallActor->GetActorLocation();
	bHasBestStop = true;

	const bool bConverged = FMath::Abs(Error) <= FrictionDistTolerance;
	const bool bExhausted = CurrentIteration >= FrictionMaxIterations;

	if (bConverged || bExhausted)
	{
		LockedFlatFriction = CurrentFlatFriction;

		Phase2Result.Value = CurrentFlatFriction;
		Phase2Result.Measured = MeasuredStopDistance;
		Phase2Result.Target = TargetStopDistance;
		Phase2Result.Error = Error;

		UE_LOG(LogTemp, Log,
			TEXT("Phase 2 COMPLETE — FlatFriction=%.5f  stopDist=%.1f cm (target %.1f cm)  %s"),
			LockedFlatFriction, MeasuredStopDistance, TargetStopDistance,
			bConverged ? TEXT("CONVERGED") : TEXT("MAX ITERATIONS"));

		OnComplete();
		return;
	}

	Phase = ECalibrationPhase::ResettingBall;
	RunPhase = ERunPhase::OnFlat;
	ResetTimer = 0.f;
}

// ────────────────────────────────────────────────────────────────────────────
void APhysicsCalibrator::OnComplete()
{
	Phase = ECalibrationPhase::Complete;

	// Apply final values
	ApplyFlatMaterial(LockedFlatFriction);
	if (BallPrimitive)
		BallPrimitive->SetAngularDamping(LockedAngularDamping);

	const FString Summary = FString::Printf(
		TEXT("══ CALIBRATION COMPLETE ══\n"
			"  Angular Damping  = %.4f\n"
			"  Flat Friction    = %.5f\n"
			"  Slope Friction   = %.4f (locked)\n"
			"  Ramp bottom time = %.3fs  (target %.2fs  error %.3fs)\n"
			"  Stop distance    = %.1f cm  (target %.1f cm  error %.1f cm)"),
		LockedAngularDamping,
		LockedFlatFriction,
		LockedSlopeFriction,
		Phase1Result.Measured, TargetBottomTime, Phase1Result.Error,
		Phase2Result.Measured, TargetStopDistance, Phase2Result.Error);

	UE_LOG(LogTemp, Log, TEXT("%s"), *Summary);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 120.f, FColor::Cyan, TEXT("══ CALIBRATION COMPLETE ══"));
		GEngine->AddOnScreenDebugMessage(-1, 120.f, FColor::Green,
			FString::Printf(TEXT("Angular Damping = %.4f\nFlat Friction   = %.5f\nSlope Friction  = %.4f"),
				LockedAngularDamping, LockedFlatFriction, LockedSlopeFriction));
		GEngine->AddOnScreenDebugMessage(-1, 120.f, FColor::White,
			FString::Printf(TEXT("Ramp error: %.3fs  |  Stop dist error: %.1f cm"),
				Phase1Result.Error, Phase2Result.Error));
	}

	// Write summary to CSV
	FString CSV = TEXT("Parameter,Value,Measured,Target,Error\n");
	CSV += FString::Printf(TEXT("AngularDamping,%.4f,%.4f,%.4f,%.4f\n"),
		LockedAngularDamping, Phase1Result.Measured, Phase1Result.Target, Phase1Result.Error);
	CSV += FString::Printf(TEXT("FlatFriction,%.5f,%.2f,%.2f,%.2f\n"),
		LockedFlatFriction, Phase2Result.Measured, Phase2Result.Target, Phase2Result.Error);
	CSV += FString::Printf(TEXT("SlopeFriction,%.4f,,,\n"), LockedSlopeFriction);

	const FString Path = FPaths::ProjectSavedDir() / TEXT("PhysicsCalibration.csv");
	FFileHelper::SaveStringToFile(CSV, *Path);
	UE_LOG(LogTemp, Log, TEXT("Results → %s"), *Path);
}

// ────────────────────────────────────────────────────────────────────────────
// Physics application helpers
// ────────────────────────────────────────────────────────────────────────────

void APhysicsCalibrator::LockBallParameters()
{
	if (BallPhysicalMaterial)
	{
		BallPhysicalMaterial->Friction = BallFriction;
		BallPhysicalMaterial->StaticFriction = BallStaticFriction;
		BallPhysicalMaterial->Restitution = BallRestitution;
#if WITH_EDITOR
		BallPhysicalMaterial->RebuildPhysicalMaterials();
#endif
	}
	if (BallPrimitive)
		BallPrimitive->SetLinearDamping(BallLinearDamping);
}

void APhysicsCalibrator::ApplySlopeMaterial()
{
	if (SlopePhysicalMaterial)
	{
		SlopePhysicalMaterial->Friction = LockedSlopeFriction;
		SlopePhysicalMaterial->StaticFriction = LockedSlopeFriction;
#if WITH_EDITOR
		SlopePhysicalMaterial->RebuildPhysicalMaterials();
#endif
	}
}

void APhysicsCalibrator::ApplyFlatMaterial(float Friction)
{
	if (FlatPhysicalMaterial)
	{
		FlatPhysicalMaterial->Friction = Friction;
		FlatPhysicalMaterial->StaticFriction = Friction;
#if WITH_EDITOR
		FlatPhysicalMaterial->RebuildPhysicalMaterials();
#endif
	}
}

// ────────────────────────────────────────────────────────────────────────────
// Ball reset helpers
// ────────────────────────────────────────────────────────────────────────────

void APhysicsCalibrator::ResetBallToRampStart()
{
	if (!BallActor || !BallPrimitive) return;
	BallPrimitive->SetSimulatePhysics(false);
	BallActor->SetActorTransform(BallStartTransform, false, nullptr, ETeleportType::ResetPhysics);
	BallPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BallPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	BallPrimitive->SetSimulatePhysics(true);
}

void APhysicsCalibrator::ResetBallToFlatStart(float InjectedVelocity)
{
	if (!BallActor || !BallPrimitive) return;
	BallPrimitive->SetSimulatePhysics(false);
	BallActor->SetActorTransform(BallFlatStartTransform, false, nullptr, ETeleportType::ResetPhysics);
	BallPrimitive->SetPhysicsLinearVelocity(FVector::ZeroVector);
	BallPrimitive->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	BallPrimitive->SetSimulatePhysics(true);

	// Inject velocity in the flat travel direction (-Y by convention, adjust if needed)
	// Direction from bottom marker toward where ball should stop
	const FVector FlatDir = FVector(0.f, -1.f, 0.f); // adjust to match your level axis
	BallPrimitive->SetPhysicsLinearVelocity(FlatDir * InjectedVelocity);
}

// ────────────────────────────────────────────────────────────────────────────
bool APhysicsCalibrator::IsBallStopped(float DeltaTime)
{
	if (!BallPrimitive) return false;
	const float Speed = BallPrimitive->GetPhysicsLinearVelocity().Size();
	if (Speed < StopVelocityThreshold)
	{
		StopDetectionTimer += DeltaTime;
		return StopDetectionTimer >= StopConfirmDuration;
	}
	StopDetectionTimer = 0.f;
	return false;
}

bool APhysicsCalibrator::HasBallPassedMarker() const
{
	if (!BallActor) return false;
	// Ball rolls in -Y — passed when ball Y < marker Y
	return BallActor->GetActorLocation().Y < BottomMarkerLocation.Y;
}

// ────────────────────────────────────────────────────────────────────────────
void APhysicsCalibrator::DrawDebugOverlay() const
{
	UWorld* World = GetWorld();
	if (!World) return;

	const float BallRadius = 4.3f;

	// ── Target stop position (red) ────────────────────────────────────────────
	// Computed from bottom marker + target distance in flat direction
	const FVector FlatDir = FVector(0.f, -1.f, 0.f);
	const FVector TargetStopPos = BottomMarkerLocation + FlatDir * TargetStopDistance;
	DrawDebugSphere(World, TargetStopPos,
		BallRadius * 1.5f, 16, FColor::Red, false, -1.f, 0, 0.f);

	// ── Best stop so far (green) ──────────────────────────────────────────────
	if (bHasBestStop)
		DrawDebugSphere(World, BestStopLocation,
			BallRadius * 1.5f, 16, FColor::Green, false, -1.f, 0, 0.f);

	// ── Orange ghost: where ball should be right now ──────────────────────────
	if (Phase != ECalibrationPhase::SearchingRampDamping &&
		Phase != ECalibrationPhase::SearchingFlatFriction)
		return;

	static constexpr float RampAccel = 980.f * 0.3746f; // g * sin(22 deg)
	const float VBottom = RampAccel * TargetBottomTime;
	const float FlatDecelTime = TargetStopTime - TargetBottomTime;
	const float FlatDecel = FlatDecelTime > 0.f ? VBottom / FlatDecelTime : 0.f;

	FVector ApproxPos;

	if (RunPhase == ERunPhase::OnRamp && ElapsedTime <= TargetBottomTime)
	{
		const float Dist = 0.5f * RampAccel * ElapsedTime * ElapsedTime;
		const FVector RampDir = (BottomMarkerLocation - BallStartTransform.GetLocation()).GetSafeNormal();
		ApproxPos = BallStartTransform.GetLocation() + RampDir * Dist;
	}
	else
	{
		// Flat phase — time since hitting flat
		const float T = (RunPhase == ERunPhase::OnFlat) ? ElapsedTime
			: ElapsedTime - TargetBottomTime;
		const float TimeToStop = FlatDecel > 0.f ? VBottom / FlatDecel : FlatMaxTime;
		const float TClamp = FMath::Min(T, TimeToStop);
		const float Dist = VBottom * TClamp - 0.5f * FlatDecel * TClamp * TClamp;
		ApproxPos = BottomMarkerLocation + FlatDir * Dist;
	}

	DrawDebugSphere(World, ApproxPos,
		BallRadius, 12, FColor::Orange, false, -1.f, 0, 0.f);
}

// ────────────────────────────────────────────────────────────────────────────
UPrimitiveComponent* APhysicsCalibrator::FindBallPrimitive() const
{
	if (!BallActor) return nullptr;
	if (auto* S = BallActor->FindComponentByClass<USphereComponent>()) return S;
	return BallActor->FindComponentByClass<UPrimitiveComponent>();
}