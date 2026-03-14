// UIInteractorActor.cpp

#include "UIInteractorActor.h"
#include "Components/WidgetInteractionComponent.h"
#include "Components/WidgetComponent.h"

AUIInteractorActor::AUIInteractorActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bStartWithTickEnabled = true;

    // WidgetInteractionComponent is created here but not attached yet —
    // attachment happens in BeginPlay once AimComponent is assigned.
    WidgetInteraction = CreateDefaultSubobject<UWidgetInteractionComponent>(
        TEXT("WidgetInteraction"));
    WidgetInteraction->InteractionDistance = TraceDistance;
    WidgetInteraction->InteractionSource = EWidgetInteractionSource::Custom;
    // Custom source means we drive the trace manually via
    // SetCustomHitResult each tick, using AimComponent's transform.
}

void AUIInteractorActor::BeginPlay()
{
    Super::BeginPlay();

    // Apply initial mode — fires OnInteractorModeChanged and sets laser state
    SetInteractorMode(InitialMode);
}

void AUIInteractorActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    TickInteraction();
}

// -------------------------------------------------------------------
// Mode
// -------------------------------------------------------------------

void AUIInteractorActor::SetInteractorMode(EUIInteractorMode NewMode)
{
    if (CurrentMode == NewMode) return;

    CurrentMode = NewMode;
    OnInteractorModeChanged(NewMode);

    if (NewMode == EUIInteractorMode::Disabled)
    {
        // Kill laser immediately
        SetLaserVisible(false);
        bHoveringWidget = false;
        SetActorTickEnabled(false);
    }
    else
    {
        SetActorTickEnabled(true);

        if (NewMode == EUIInteractorMode::Active)
        {
            // Active always shows laser
            SetLaserVisible(true);
        }
        // Passive: laser state determined per-tick by hit result
    }
}

// -------------------------------------------------------------------
// Input
// -------------------------------------------------------------------

void AUIInteractorActor::NotifyTriggerPressed()
{
    if (CurrentMode == EUIInteractorMode::Disabled) return;
    if (!bLaserVisible || !bHoveringWidget) return;

    WidgetInteraction->PressPointerKey(EKeys::LeftMouseButton);
    OnUIClickPressed();
}

void AUIInteractorActor::NotifyTriggerReleased()
{
    if (CurrentMode == EUIInteractorMode::Disabled) return;
    if (!bLaserVisible) return;

    WidgetInteraction->ReleasePointerKey(EKeys::LeftMouseButton);
    OnUIClickReleased();
}

// -------------------------------------------------------------------
// Tick
// -------------------------------------------------------------------

void AUIInteractorActor::TickInteraction()
{
    if (CurrentMode == EUIInteractorMode::Disabled) return;
    if (!AimComponent) return;

    const FVector Start = AimComponent->GetComponentLocation();
    const FVector Forward = AimComponent->GetForwardVector();
    const FVector End = Start + (Forward * TraceDistance);

    // Drive WidgetInteractionComponent with a custom hit result
    FHitResult HitResult;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);

    const bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult, Start, End, ECC_Visibility, Params);

    const bool bHitWidget = bHit && HitResult.GetComponent() &&
        HitResult.GetComponent()->IsA<UWidgetComponent>();

    // Feed result into WidgetInteractionComponent
    if (bHitWidget)
    {
        WidgetInteraction->SetCustomHitResult(HitResult);
    }

    bHoveringWidget = bHitWidget;

    // Determine laser endpoint
    const FVector LaserEnd = bHit ? HitResult.ImpactPoint : End;

    // Fire per-tick event for BP to update mesh
    OnLaserTick(Start, LaserEnd, bHitWidget);

    // Update laser visibility for Passive mode
    if (CurrentMode == EUIInteractorMode::Passive)
    {
        SetLaserVisible(bHitWidget);
    }
    // Active mode: laser always on, no change needed here
}

// -------------------------------------------------------------------
// Internal helpers
// -------------------------------------------------------------------

void AUIInteractorActor::SetLaserVisible(bool bVisible)
{
    if (bLaserVisible == bVisible) return;  // Only fire on actual change

    bLaserVisible = bVisible;
    OnLaserVisibilityChanged(bVisible);
}