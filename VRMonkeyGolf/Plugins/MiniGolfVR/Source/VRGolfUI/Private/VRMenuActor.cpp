// VRMenuActor.cpp
 
#include "VRMenuActor.h"
#include "Components/WidgetComponent.h"
#include "Components/BoxComponent.h"
#include "Blueprint/UserWidget.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
 
#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#endif
 
AVRMenuActor::AVRMenuActor()
{
    PrimaryActorTick.bCanEverTick = false;
 
    // --- Widget (runtime) ---
    WidgetComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
    SetRootComponent(WidgetComponent);
    WidgetComponent->SetWidgetSpace(EWidgetSpace::World);
    WidgetComponent->SetDrawSize(WidgetDrawSize);
    WidgetComponent->SetTwoSided(true);
    WidgetComponent->SetVisibility(false);
    WidgetComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
 
#if WITH_EDITORONLY_DATA
    // --- Billboard (editor icon, always selectable) ---
    Billboard = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
    if (Billboard)
    {
        Billboard->SetupAttachment(RootComponent);
        Billboard->SetRelativeLocation(FVector::ZeroVector);
        Billboard->bIsScreenSizeScaled = true;
    }
 
    // --- Preview box (shows widget plane size in editor) ---
    PreviewBox = CreateEditorOnlyDefaultSubobject<UBoxComponent>(TEXT("PreviewBox"));
    if (PreviewBox)
    {
        PreviewBox->SetupAttachment(RootComponent);
        // Thin box — depth of 1cm so it's clearly a plane, not a volume
        PreviewBox->SetBoxExtent(FVector(
            WidgetDrawSize.X * WidgetWorldScale * 0.5f,
            WidgetDrawSize.Y * WidgetWorldScale * 0.5f,
            0.5f
        ));
        PreviewBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PreviewBox->SetLineThickness(2.0f);
        // Editor-only — hidden in game automatically
        PreviewBox->SetHiddenInGame(true);
    }
#endif
}
 
void AVRMenuActor::BeginPlay()
{
    Super::BeginPlay();
 
    WidgetComponent->SetDrawSize(WidgetDrawSize);
    WidgetComponent->SetWorldScale3D(FVector(WidgetWorldScale));
 
    if (DefaultWidgetClass)
    {
        WidgetComponent->SetWidgetClass(DefaultWidgetClass);
    }
 
    if (bShowOnBeginPlay)
    {
        SetMenuVisible(true, false);
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

    UCameraComponent* Cam = Pawn->FindComponentByClass<UCameraComponent>();
    FVector  Origin  = Cam ? Cam->GetComponentLocation() : Pawn->GetActorLocation();
    FRotator Facing  = Cam ? Cam->GetComponentRotation() : Pawn->GetActorRotation();

    // Strip pitch so menu stays upright even if player is looking down
    FRotator FlatFacing = FRotator(0.f, Facing.Yaw, 0.f);

    FVector  Forward     = FlatFacing.Vector();
    FVector  MenuLocation = Origin + (Forward * SnapDistance);
    FRotator MenuRotation = (Origin - MenuLocation).Rotation();

    SetActorLocationAndRotation(MenuLocation, MenuRotation);
}
 
void AVRMenuActor::SyncPreviewBox()
{
#if WITH_EDITORONLY_DATA
    if (PreviewBox)
    {
        PreviewBox->SetBoxExtent(FVector(
            WidgetDrawSize.X * WidgetWorldScale * 0.5f,
            WidgetDrawSize.Y * WidgetWorldScale * 0.5f,
            0.5f
        ));
    }
    if (WidgetComponent)
    {
        WidgetComponent->SetDrawSize(WidgetDrawSize);
        WidgetComponent->SetWorldScale3D(FVector(WidgetWorldScale));
    }
#endif
}
 
#if WITH_EDITOR
void AVRMenuActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
 
    // Sync preview box whenever WidgetDrawSize or WidgetWorldScale changes in editor
    const FName PropName = PropertyChangedEvent.GetPropertyName();
    if (PropName == GET_MEMBER_NAME_CHECKED(AVRMenuActor, WidgetDrawSize) ||
        PropName == GET_MEMBER_NAME_CHECKED(AVRMenuActor, WidgetWorldScale))
    {
        SyncPreviewBox();
    }
}
#endif
