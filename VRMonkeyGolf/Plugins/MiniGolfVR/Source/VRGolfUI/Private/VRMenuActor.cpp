// VRMenuActor.cpp

#include "VRMenuActor.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

AVRMenuActor::AVRMenuActor()
{
    PrimaryActorTick.bCanEverTick = false;

    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    SetRootComponent(WidgetComponent);

    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawSize(WidgetDrawSize);
    WidgetComponent->SetTwoSided(true);         // No backface culling in VR
    WidgetComponent->SetVisibility(false);
    WidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly); // Required for interactor hit
}

void AVRMenuActor::BeginPlay()
{
    Super::BeginPlay();

    // Apply draw size — may differ from constructor default if changed in BP
    WidgetComponent->SetDrawSize(WidgetDrawSize);

    if (DefaultWidgetClass)
    {
        WidgetComponent->SetWidgetClass(DefaultWidgetClass);
    }
}

void AVRMenuActor::SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass)
{
    if (InWidgetClass)
    {
        WidgetComponent->SetWidgetClass(InWidgetClass);
    }
}

void AVRMenuActor::SetMenuVisible(bool bVisible, bool bSnapToPlayer)
{
    bMenuVisible = bVisible;
    WidgetComponent->SetVisibility(bVisible);

    if (bVisible)
    {
        if (bSnapToPlayer)
        {
            SnapToPlayerCamera();
        }
        OnMenuShown();
    }
    else
    {
        OnMenuHidden();
    }
}

UUserWidget* AVRMenuActor::GetWidgetInstance() const
{
    return WidgetComponent->GetUserWidgetObject();
}

void AVRMenuActor::SnapToPlayerCamera()
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
    if (!PC) return;

    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    // Use camera component for accurate HMD position in VR
    UCameraComponent* Cam = Pawn->FindComponentByClass<UCameraComponent>();
    FVector  Origin = Cam ? Cam->GetComponentLocation() : Pawn->GetActorLocation();
    FRotator Facing = Cam ? Cam->GetComponentRotation() : Pawn->GetActorRotation();

    // Place at SnapDistance in front, facing back toward the player
    FVector Forward     = Facing.Vector();
    FVector MenuLocation = Origin + (Forward * SnapDistance);
    FRotator MenuRotation = (Origin - MenuLocation).Rotation();

    SetActorLocationAndRotation(MenuLocation, MenuRotation);
}
