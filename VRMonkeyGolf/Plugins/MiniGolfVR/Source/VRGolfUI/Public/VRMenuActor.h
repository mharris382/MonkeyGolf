// VRMenuActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetComponent.h"
#include "VRMenuActor.generated.h"

/**
 * World-space VR menu actor.
 * Hosts a UWidgetComponent for VR UI rendering.
 *
 * Spawn this actor in your level (or at runtime) and call SetMenuVisible()
 * to show any UMG widget in world space in front of the player.
 *
 * BP subclass sets DefaultWidgetClass in defaults and handles
 * open/close animations via OnMenuShown / OnMenuHidden overrides.
 */
UCLASS(Abstract, Blueprintable)
class VRGOLFUI_API AVRMenuActor : public AActor
{
    GENERATED_BODY()

public:
    AVRMenuActor();

    // --- Interface ---

    /** Set the widget class to display. Safe to call at runtime before showing. */
    UFUNCTION(BlueprintCallable, Category = "VR Menu")
    void SetWidgetClass(TSubclassOf<UUserWidget> InWidgetClass);

    /**
     * Show or hide the menu.
     * @param bVisible       Show or hide.
     * @param bSnapToPlayer  If true, repositions actor in front of the player camera on show.
     */
    UFUNCTION(BlueprintCallable, Category = "VR Menu")
    void SetMenuVisible(bool bVisible, bool bSnapToPlayer = true);

    UFUNCTION(BlueprintPure, Category = "VR Menu")
    bool IsMenuVisible() const { return bMenuVisible; }

    /** Returns the live UMG widget instance. May be null before first show. */
    UFUNCTION(BlueprintPure, Category = "VR Menu")
    UUserWidget* GetWidgetInstance() const;

protected:
    // --- Components ---

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "VR Menu")
    TObjectPtr<UWidgetComponent> WidgetComponent;

    // --- Config (set in BP subclass defaults) ---

    /** Widget class to instantiate. Set this in your BP subclass defaults. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Menu")
    TSubclassOf<UUserWidget> DefaultWidgetClass;

    /** Distance from player camera when snapping into position (cm). */
    UPROPERTY(EditDefaultsOnly, Category = "VR Menu")
    float SnapDistance = 100.0f;

    /** UMG draw size in pixels. */
    UPROPERTY(EditDefaultsOnly, Category = "VR Menu")
    FVector2D WidgetDrawSize = FVector2D(800.f, 600.f);

    // --- Overridable hooks (implement in BP subclass) ---

    /** Called after the widget becomes visible. Use for open animation. */
    UFUNCTION(BlueprintNativeEvent, Category = "VR Menu")
    void OnMenuShown();
    virtual void OnMenuShown_Implementation() {}

    /** Called before the widget is hidden. Use for close animation. */
    UFUNCTION(BlueprintNativeEvent, Category = "VR Menu")
    void OnMenuHidden();
    virtual void OnMenuHidden_Implementation() {}

    virtual void BeginPlay() override;

private:
    bool bMenuVisible = false;

    void SnapToPlayerCamera();
};
