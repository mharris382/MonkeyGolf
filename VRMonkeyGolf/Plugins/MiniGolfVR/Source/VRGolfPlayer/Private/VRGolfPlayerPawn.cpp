// VRGolfPlayerPawn.cpp
#include "VRGolfPlayerPawn.h"

#include "VRGolfPutter.h"

// VRExpansionPlugin
#include "GripMotionControllerComponent.h"
#include "VRBaseCharacter.h"

// Enhanced Input
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

// -------------------------------------------------------------------------
// Constructor
// -------------------------------------------------------------------------
AVRGolfPlayerPawn::AVRGolfPlayerPawn(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    PrimaryActorTick.bCanEverTick = true;

    // -----------------------------------------------------------------------
    // Disable HMD body-collision fade/blackout.
    // VRBaseCharacter exposes this flag to prevent the "fade to black when you
    // walk your head into a wall" behavior — unwanted for a stationary golf game.
    // -----------------------------------------------------------------------
    bUseControllerRotationYaw = false;
    
    // VRE property: disables the overlap-triggered HMD fade
    // (lives on VRBaseCharacter as of VRE 4.x/5.x)
    bRetainRoomscale = true;  // keep physical roomscale, no auto-correction fade
}

// -------------------------------------------------------------------------
// BeginPlay
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::BeginPlay()
{
    Super::BeginPlay();

    // Add input mapping context
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
        {
            if (GolfMappingContext)
            {
                Subsystem->AddMappingContext(GolfMappingContext, 0);
            }
        }
    }

    // Spawn the putter on the default active hand (Right)
    SpawnAndAttachPutter();
    RefreshControllerVisibility();
}

// -------------------------------------------------------------------------
// Tick
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
}

// -------------------------------------------------------------------------
// Input setup
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EIC) return;

    // Any left-hand input → mark left as active
    if (IA_LeftHandActive)
    {
        EIC->BindAction(IA_LeftHandActive, ETriggerEvent::Started, this, &AVRGolfPlayerPawn::OnLeftHandInput);
    }

    // Any right-hand input → mark right as active
    if (IA_RightHandActive)
    {
        EIC->BindAction(IA_RightHandActive, ETriggerEvent::Started, this, &AVRGolfPlayerPawn::OnRightHandInput);
    }

    // Teleport — separate per hand so we know which controller to raycast from
    if (IA_Teleport_Left)
    {
        EIC->BindAction(IA_Teleport_Left, ETriggerEvent::Triggered, this, &AVRGolfPlayerPawn::OnTeleportLeft);
    }

    if (IA_Teleport_Right)
    {
        EIC->BindAction(IA_Teleport_Right, ETriggerEvent::Triggered, this, &AVRGolfPlayerPawn::OnTeleportRight);
    }
}

// -------------------------------------------------------------------------
// Hand input handlers
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::OnLeftHandInput(const FInputActionValue& Value)
{
    SetActiveHand(EGolfActiveHand::Left);
}

void AVRGolfPlayerPawn::OnRightHandInput(const FInputActionValue& Value)
{
    SetActiveHand(EGolfActiveHand::Right);
}

void AVRGolfPlayerPawn::OnTeleportLeft(const FInputActionValue& Value)
{
    // Switching to the hand that initiated teleport feels natural
    SetActiveHand(EGolfActiveHand::Left);
    ExecuteTeleport(EGolfActiveHand::Left);
}

void AVRGolfPlayerPawn::OnTeleportRight(const FInputActionValue& Value)
{
    SetActiveHand(EGolfActiveHand::Right);
    ExecuteTeleport(EGolfActiveHand::Right);
}

// -------------------------------------------------------------------------
// SetActiveHand  — the core of the whole system
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::SetActiveHand(EGolfActiveHand NewHand)
{
    if (ActiveHand == NewHand) return;

    ActiveHand = NewHand;

    // Re-attach putter to the newly active controller
    if (Putter)
    {
        UGripMotionControllerComponent* ctrler = GetActiveController();
        if (ctrler)
        {
            Putter->AttachToMotionController(ctrler);
        }
    }

    RefreshControllerVisibility();
}

// -------------------------------------------------------------------------
// Controller accessors
// -------------------------------------------------------------------------
UGripMotionControllerComponent* AVRGolfPlayerPawn::GetActiveController() const
{
    // VRBaseCharacter exposes these as LeftMotionController / RightMotionController
    return (ActiveHand == EGolfActiveHand::Right) ? RightMotionController : LeftMotionController;
}

UGripMotionControllerComponent* AVRGolfPlayerPawn::GetInactiveController() const
{
    return (ActiveHand == EGolfActiveHand::Right) ? LeftMotionController : RightMotionController;
}

// -------------------------------------------------------------------------
// SpawnAndAttachPutter
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::SpawnAndAttachPutter()
{
    if (!PutterClass || !GetWorld()) return;

    FActorSpawnParameters Params;
    Params.Owner = this;
    Params.Instigator = GetInstigator();

    Putter = GetWorld()->SpawnActor<AVRGolfPutter>(PutterClass, FTransform::Identity, Params);

    if (Putter)
    {
        UGripMotionControllerComponent* ActiveController = GetActiveController();
        if (ActiveController)
        {
            Putter->AttachToMotionController(ActiveController);
        }
        bPutterSpawned = true;
    }
}

// -------------------------------------------------------------------------
// RefreshControllerVisibility — only active hand visible
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::RefreshControllerVisibility()
{
    // VRE's motion controller components have display meshes you can hide.
    // We hide the entire component subtree on the inactive hand.
    if (RightMotionController)
    {
        const bool bRightActive = (ActiveHand == EGolfActiveHand::Right);
        RightMotionController->SetVisibility(bRightActive, /*bPropagateToChildren=*/true);
    }

    if (LeftMotionController)
    {
        const bool bLeftActive = (ActiveHand == EGolfActiveHand::Left);
        LeftMotionController->SetVisibility(bLeftActive, /*bPropagateToChildren=*/true);
    }
}

// -------------------------------------------------------------------------
// ExecuteTeleport
// -------------------------------------------------------------------------
void AVRGolfPlayerPawn::ExecuteTeleport(EGolfActiveHand FromHand)
{
    // TODO: Implement your teleport arc/raycast from the active controller.
    // VRE provides TeleportTo() on the character — feed it the destination
    // you resolve from the controller's forward vector + arc trace.
    //
    // Minimal stub:
    UGripMotionControllerComponent* ctrl = GetActiveController();
    if (!ctrl) return;

    // Your teleport destination logic goes here.
    // Example pattern:
    //   FVector Start = Controller->GetComponentLocation();
    //   FVector Direction = Controller->GetForwardVector();
    //   FHitResult Hit;
    //   ... arc trace ...
    //   TeleportTo(GetTeleportLocation(Hit.ImpactPoint), GetActorRotation());
}