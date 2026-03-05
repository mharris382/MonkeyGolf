#include "VRGolfPutter.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "DrawDebugHelpers.h" // remove in shipping
#include "Engine/World.h"


void AVRGolfPutter::AttachToMotionController(USceneComponent* MotionController)
{
	if(MotionController)
	{
		FAttachmentTransformRules Rules(EAttachmentRule::KeepRelative, false);
		Rules.ScaleRule = EAttachmentRule::KeepWorld; // Don't let the controller's scale affect the putter pivot
		PutterPivot->AttachToComponent(MotionController, Rules);
	}
	
}


// ============================================================
//  Construction
// ============================================================

AVRGolfPutter::AVRGolfPutter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	PutterPivot = CreateDefaultSubobject<USceneComponent>(TEXT("PutterPivot"));
	SetRootComponent(PutterPivot);

	// No mesh, no collision component — intentional.
	// Attach your Motion Controller Component to PutterPivot in the child BP.
}

// ============================================================
//  BeginPlay
// ============================================================

void AVRGolfPutter::BeginPlay()
{
	Super::BeginPlay();

	LastPivotPosition = PutterPivot->GetComponentLocation();
	bHasLastPosition = false;
	bStrikeConsumed = false;
}

// ============================================================
//  Tick
// ============================================================

void AVRGolfPutter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (DeltaTime <= SMALL_NUMBER) return;

	const FVector CurrentPos = PutterPivot->GetComponentLocation();

	// --- Projected velocity (plane-aligned, used for strike force) ---
	if (bHasLastPosition)
	{
		const FVector RawDelta = CurrentPos - LastPivotPosition;
		const FVector ProjectedPos = ProjectOntoTeePlane(CurrentPos);
		const FVector ProjectedLast = ProjectOntoTeePlane(LastPivotPosition);
		ProjectedVelocity = (ProjectedPos - ProjectedLast) / DeltaTime;
	}
	else
	{
		ProjectedVelocity = FVector::ZeroVector;
	}

	TickSurfaceUpdate(DeltaTime);
	TickStrikeCheck();

	LastPivotPosition = CurrentPos;
	bHasLastPosition = true;
}

// ============================================================
//  Runtime API
// ============================================================

void AVRGolfPutter::SetTeeTransform(FVector InBallPosition, FVector InSurfaceNormal)
{
	BallPosition = InBallPosition;
	TeeNormal = InSurfaceNormal.GetSafeNormal();

	// Reset strike guard so the new ball can be hit
	bStrikeConsumed = false;

	UE_LOG(LogTemp, Log, TEXT("VRGolfPutter: Tee updated. Ball=%s Normal=%s"),
		*BallPosition.ToString(), *TeeNormal.ToString());
}

// ============================================================
//  BP Native Event Implementations  (log warnings if not overridden)
// ============================================================

void AVRGolfPutter::OnSurfaceUpdate_Implementation(const FPutterSurfaceData& SurfaceData)
{
	UE_LOG(LogTemp, Warning,
		TEXT("VRGolfPutter: OnSurfaceUpdate fired but has no BP override. "
			"Override this event in your child Blueprint to handle club positioning and aim visuals."));
}

void AVRGolfPutter::OnStrike_Implementation(const FPutterStrikeData& StrikeData)
{
	UE_LOG(LogTemp, Warning,
		TEXT("VRGolfPutter: OnStrike fired but has no BP override. "
			"Override this event in your child Blueprint to apply force to the ball, trigger haptics, etc. "
			"SurfaceAlignedForce=%s  ClampedForce=%s"),
		*StrikeData.SurfaceAlignedForce.ToString(),
		*StrikeData.ClampedSurfaceAlignedForce.ToString());
}

// ============================================================
//  Private — Helpers
// ============================================================

FVector AVRGolfPutter::ProjectOntoTeePlane(const FVector& WorldPos) const
{
	// Standard point-onto-plane projection:
	// P' = P - ((P - PlaneOrigin) · Normal) * Normal
	const float Dist = FVector::DotProduct(WorldPos - BallPosition, TeeNormal);
	return WorldPos - (Dist * TeeNormal);
}

AActor* AVRGolfPutter::ResolvePuttingSurface() const
{
	// Short downward trace from the ball position to find the surface actor.
	FHitResult Hit;
	const FVector TraceStart = BallPosition + TeeNormal * 5.f;
	const FVector TraceEnd = BallPosition - TeeNormal * 20.f;

	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		return Hit.GetActor();
	}
	return nullptr;
}

// ============================================================
//  Private — Tick helpers
// ============================================================

void AVRGolfPutter::TickSurfaceUpdate(float DeltaTime)
{
	const FVector CurrentPos = PutterPivot->GetComponentLocation();
	const FVector ProjectedPos = ProjectOntoTeePlane(CurrentPos);
	const FVector ToBall = BallPosition - ProjectedPos;
	const float   DistOnPlane = ToBall.Size();
	const FVector DirToBall = DistOnPlane > SMALL_NUMBER ? ToBall / DistOnPlane : FVector::ZeroVector;

	// Alignment check: how perpendicular is the club to the surface normal?
	// We check the upward component of the putter pivot's forward vector vs the plane.
	// A dot close to 0 means the club is roughly parallel to the surface — correct putting stance.
	const FVector PivotForward = PutterPivot->GetForwardVector();
	const float   AlignDot = FMath::Abs(FVector::DotProduct(PivotForward, TeeNormal));

	// Fire only when alignment is within threshold (club near-parallel to surface)
	if (AlignDot > SurfaceAlignmentThreshold)
	{
		return;
	}

	FPutterSurfaceData Data;
	Data.bSurfaceFound = true;
	Data.ProjectedClubPosition = ProjectedPos;
	Data.SurfaceNormal = TeeNormal;
	Data.DirectionToBall = DirToBall;
	Data.DistanceToBall = DistOnPlane;
	Data.HandTransform = PutterPivot->GetComponentTransform();

	OnSurfaceUpdate(Data);

	// --- Debug ---
	if (bDebugEnabled && bDebugShowSurfaceData) { VisualizePutterSurfaceData(Data); }
	if (bDebugEnabled && bDebugShowPutterLine) { VisualizeProjectedPutterLine(CurrentPos, ProjectedPos, FLinearColor::White, 0.f, 2.f, 1.f, 1.f); }
	if (bDebugEnabled && bDebugShowPuttPlane) { VisualizePuttPlane(BallPosition, TeeNormal, FLinearColor::Blue, FVector2D(50.f, 50.f)); }
	if (bDebugEnabled && bDebugShowStrikeZone) { VisualizePutterStrikeZone(FLinearColor(0.f, 1.f, 1.f, 1.f), 0.f); }
}

void AVRGolfPutter::TickStrikeCheck()
{
	if (bStrikeConsumed) return;

	const FVector PivotPos = PutterPivot->GetComponentLocation();
	const float   Dist3D = FVector::Dist(PivotPos, BallPosition);

	if (Dist3D > StrikeRadius) return;

	// --- Build raw force (world space delta velocity) ---
	const FVector RawForce = ProjectedVelocity; // already delta/dt

	// --- Surface-aligned force: remove normal component ---
	const float   NormalComponent = FVector::DotProduct(RawForce, TeeNormal);
	const FVector SurfaceAligned = RawForce - (NormalComponent * TeeNormal);

	// --- Clamped force: same direction, magnitude clamped ---
	// TODO: Wire ForceClampMin / ForceClampMax to a project settings asset.
	const float   RawMag = SurfaceAligned.Size();
	const float   ClampedMag = FMath::Clamp(RawMag, ForceClampMin, ForceClampMax);
	const FVector ClampedForce = RawMag > SMALL_NUMBER
		? SurfaceAligned * (ClampedMag / RawMag)
		: FVector::ZeroVector;

	// --- Locations ---
	const FVector RawHitLoc = PivotPos;
	const FVector BallAlignedHitLoc = ProjectOntoTeePlane(PivotPos);

	// --- Owning player ---
	APlayerController* PC = nullptr;
	if (APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		PC = Cast<APlayerController>(Pawn->GetController());
	}

	// --- Surface under ball ---
	AActor* Surface = ResolvePuttingSurface();

	// --- Populate and fire ---
	FPutterStrikeData StrikeData;
	StrikeData.RawHitForce = RawForce;
	StrikeData.SurfaceAlignedForce = SurfaceAligned;
	StrikeData.ClampedSurfaceAlignedForce = ClampedForce;
	StrikeData.RawHitLocation = RawHitLoc;
	StrikeData.BallAlignedHitLocation = BallAlignedHitLoc;
	StrikeData.HitBall = nullptr; // TODO: pass actual ball actor reference
	StrikeData.Club = this;
	StrikeData.Player = PC;
	StrikeData.PuttingSurface = Surface;
	StrikeData.HandTransform = PutterPivot->GetComponentTransform();

	OnStrike(StrikeData);

	// --- Debug ---
	if (bDebugEnabled && bDebugShowStrikeVectors) { VisualizePutterStrike(StrikeData); }

	// Consume — reset via SetTeeTransform on next hole / next ball placement
	bStrikeConsumed = true;
}

// ============================================================
//  Debug — Primitive Implementations
// ============================================================

void AVRGolfPutter::VisualizePuttVector(FVector Origin, FVector Vector, FLinearColor Color, float Duration) const
{
	if (!GetWorld()) return;
	const FColor C = Color.ToFColor(true);
	// Arrow: line + small cone tip drawn as a short secondary line offset
	DrawDebugLine(GetWorld(), Origin, Origin + Vector, C, false, Duration, 0, 1.5f);
	DrawDebugPoint(GetWorld(), Origin + Vector, 6.f, C, false, Duration);
}

void AVRGolfPutter::VisualizePuttPlane(FVector Origin, FVector PlaneNormal, FLinearColor Color, FVector2D Extents) const
{
	if (!GetWorld()) return;

	// Build two tangent axes from the normal
	const FVector SafeNormal = PlaneNormal.GetSafeNormal();
	FVector AxisX, AxisY;
	SafeNormal.FindBestAxisVectors(AxisX, AxisY);

	const FColor C = Color.ToFColor(true);
	const float HX = Extents.X;
	const float HY = Extents.Y;

	// Four corners
	const FVector C0 = Origin + AxisX * HX + AxisY * HY;
	const FVector C1 = Origin - AxisX * HX + AxisY * HY;
	const FVector C2 = Origin - AxisX * HX - AxisY * HY;
	const FVector C3 = Origin + AxisX * HX - AxisY * HY;

	DrawDebugLine(GetWorld(), C0, C1, C, false, 0.f, 0, 0.5f);
	DrawDebugLine(GetWorld(), C1, C2, C, false, 0.f, 0, 0.5f);
	DrawDebugLine(GetWorld(), C2, C3, C, false, 0.f, 0, 0.5f);
	DrawDebugLine(GetWorld(), C3, C0, C, false, 0.f, 0, 0.5f);

	// Cross diagonals so it reads as a plane, not just a box
	DrawDebugLine(GetWorld(), C0, C2, C, false, 0.f, 0, 0.25f);
	DrawDebugLine(GetWorld(), C1, C3, C, false, 0.f, 0, 0.25f);

	// Normal arrow
	DrawDebugLine(GetWorld(), Origin, Origin + SafeNormal * 10.f, C, false, 0.f, 0, 1.f);
}

void AVRGolfPutter::VisualizeProjectedPutterLine(FVector Handle, FVector Head, FLinearColor Color,
	float Duration, float EndSphereRadius,
	float StartSphereRadius, float Thickness) const
{
	if (!GetWorld()) return;
	const FColor C = Color.ToFColor(true);
	DrawDebugLine(GetWorld(), Handle, Head, C, false, Duration, 0, Thickness);
	DrawDebugSphere(GetWorld(), Handle, StartSphereRadius, 6, C, false, Duration);  // grip end
	DrawDebugSphere(GetWorld(), Head, EndSphereRadius, 6, C, false, Duration);  // strike head
}

void AVRGolfPutter::VisualizePutterStrikeZone(FLinearColor Color, float Duration) const
{
	if (!GetWorld()) return;
	DrawDebugSphere(GetWorld(), PutterPivot->GetComponentLocation(),
		StrikeRadius, 12, Color.ToFColor(true), false, Duration);
}

// ============================================================
//  Debug — Composite Implementations
// ============================================================

void AVRGolfPutter::VisualizePutterSurfaceData(const FPutterSurfaceData& SurfaceData) const
{
	if (!GetWorld()) return;

	// Projected club position — white dot
	DrawDebugPoint(GetWorld(), SurfaceData.ProjectedClubPosition, 8.f, FColor::White, false, 0.f);

	// Direction to ball on plane — green arrow
	VisualizePuttVector(SurfaceData.ProjectedClubPosition,
		SurfaceData.DirectionToBall * FMath::Min(SurfaceData.DistanceToBall, 30.f),
		FLinearColor::Green, 0.f);

	// Surface normal — blue arrow from ball position
	VisualizePuttVector(BallPosition, SurfaceData.SurfaceNormal * 15.f, FLinearColor::Blue, 0.f);

	// Distance label via a small sphere at ball position
	DrawDebugSphere(GetWorld(), BallPosition, 2.f, 6, FColor::Yellow, false, 0.f);
}

void AVRGolfPutter::VisualizePutterStrike(const FPutterStrikeData& StrikeData) const
{
	if (!GetWorld()) return;

	const float DrawDuration = 3.f; // strikes persist longer so you can read them

	// Raw hit location — white dot
	DrawDebugPoint(GetWorld(), StrikeData.RawHitLocation, 10.f, FColor::White, false, DrawDuration);

	// Ball-aligned hit location — yellow dot
	DrawDebugPoint(GetWorld(), StrikeData.BallAlignedHitLocation, 10.f, FColor::Yellow, false, DrawDuration);

	// Line between raw and aligned to show the vertical snap offset
	DrawDebugLine(GetWorld(), StrikeData.RawHitLocation, StrikeData.BallAlignedHitLocation,
		FColor::White, false, DrawDuration, 0, 0.5f);

	// RawHitForce — red
	VisualizePuttVector(StrikeData.BallAlignedHitLocation,
		StrikeData.RawHitForce * 0.05f,   // scale so it's readable in world units
		FLinearColor::Red, DrawDuration);

	// SurfaceAlignedForce — green
	VisualizePuttVector(StrikeData.BallAlignedHitLocation,
		StrikeData.SurfaceAlignedForce * 0.05f,
		FLinearColor::Green, DrawDuration);

	// ClampedSurfaceAlignedForce — yellow
	VisualizePuttVector(StrikeData.BallAlignedHitLocation,
		StrikeData.ClampedSurfaceAlignedForce * 0.05f,
		FLinearColor::Yellow, DrawDuration);
}