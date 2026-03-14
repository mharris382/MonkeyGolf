// UIInteractorActor.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/WidgetInteractionComponent.h"
#include "UIInteractorActor.generated.h"

UENUM(BlueprintType)
enum class EUIInteractorMode : uint8
{
    Disabled    UMETA(DisplayName = "Disabled"),     // No traces, no events, completely silent
    Active      UMETA(DisplayName = "Active"),       // Trace runs, laser always visible
    Passive     UMETA(DisplayName = "Passive"),      // Trace runs, laser only visible on widget hit
};

/**
 * Spawnable world-space UI interactor.
 * Attach an aim SceneComponent (e.g. a motion controller scene root) to drive
 * the trace direction. No hard reference to any pawn or controller.
 *
 * Mode controls all behavior:
 *   Disabled — silent, nothing runs
 *   Active   — laser always visible (use when inside a menu area)
 *   Passive  — laser only visible when trace hits a widget
 *
 * BP subclass implements all visuals via NativeEvent hooks.
 * Club hide/show should be driven by OnLaserVisibilityChanged.
 */
UCLASS(Blueprintable)
class VRGOLFUI_API AUIInteractorActor : public AActor
{
    GENERATED_BODY()

public:
    AUIInteractorActor();

    // -------------------------------------------------------------------
    // Expose on spawn — assign aim source at spawn time or swap at runtime
    // -------------------------------------------------------------------

    /**
     * The scene component whose forward vector drives the interaction trace.
     * Typically a motion controller scene root. Swappable at runtime to
     * support switching between left/right controllers.
     * Exposed on spawn so BP SpawnActor node shows it directly.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Interactor",
              meta = (ExposeOnSpawn = true))
    TObjectPtr<USceneComponent> AimComponent;

    // -------------------------------------------------------------------
    // Mode control
    // -------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "UI Interactor")
    void SetInteractorMode(EUIInteractorMode NewMode);

    UFUNCTION(BlueprintPure, Category = "UI Interactor")
    EUIInteractorMode GetInteractorMode() const { return CurrentMode; }

    // -------------------------------------------------------------------
    // Input — call these from pawn input events
    // -------------------------------------------------------------------

    UFUNCTION(BlueprintCallable, Category = "UI Interactor")
    void NotifyTriggerPressed();

    UFUNCTION(BlueprintCallable, Category = "UI Interactor")
    void NotifyTriggerReleased();

    /**
     * Returns true if the interactor consumed the trigger (laser is visible
     * and hovering a widget). Use this to gate putter stroke logic.
     */
    UFUNCTION(BlueprintPure, Category = "UI Interactor")
    bool IsConsumingInput() const { return bLaserVisible && bHoveringWidget; }

    // -------------------------------------------------------------------
    // State queries
    // -------------------------------------------------------------------

    UFUNCTION(BlueprintPure, Category = "UI Interactor")
    bool IsLaserVisible() const { return bLaserVisible; }

    UFUNCTION(BlueprintPure, Category = "UI Interactor")
    bool IsHoveringWidget() const { return bHoveringWidget; }

protected:
    // -------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI Interactor")
    TObjectPtr<UWidgetInteractionComponent> WidgetInteraction;

    // -------------------------------------------------------------------
    // Config
    // -------------------------------------------------------------------

    /** How far the interaction trace reaches (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Interactor")
    float TraceDistance = 1000.0f;

    /** Starting mode on BeginPlay. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI Interactor",
              meta = (ExposeOnSpawn = true))
    EUIInteractorMode InitialMode = EUIInteractorMode::Passive;

    // -------------------------------------------------------------------
    // BP visual hooks — implement these in your BP subclass
    // -------------------------------------------------------------------

    /** Laser visibility changed. Drive club hide/show from here. */
    UFUNCTION(BlueprintNativeEvent, Category = "UI Interactor|Visuals")
    void OnLaserVisibilityChanged(bool bVisible);
    virtual void OnLaserVisibilityChanged_Implementation(bool bVisible) {}

    /** Fires every tick while trace is running. Use to update laser mesh endpoints. */
    UFUNCTION(BlueprintNativeEvent, Category = "UI Interactor|Visuals")
    void OnLaserTick(FVector Start, FVector End, bool bHitWidget);
    virtual void OnLaserTick_Implementation(FVector Start, FVector End, bool bHitWidget) {}

    /** Trigger pressed while laser visible and hovering a widget. */
    UFUNCTION(BlueprintNativeEvent, Category = "UI Interactor|Input")
    void OnUIClickPressed();
    virtual void OnUIClickPressed_Implementation() {}

    /** Trigger released after a UI click. */
    UFUNCTION(BlueprintNativeEvent, Category = "UI Interactor|Input")
    void OnUIClickReleased();
    virtual void OnUIClickReleased_Implementation() {}

    /** Mode changed — useful for BP debug display or future logic. */
    UFUNCTION(BlueprintNativeEvent, Category = "UI Interactor|Mode")
    void OnInteractorModeChanged(EUIInteractorMode NewMode);
    virtual void OnInteractorModeChanged_Implementation(EUIInteractorMode NewMode) {}

    // -------------------------------------------------------------------
    // Lifecycle
    // -------------------------------------------------------------------

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

private:
    EUIInteractorMode CurrentMode = EUIInteractorMode::Passive;

    bool bLaserVisible    = false;
    bool bHoveringWidget  = false;

    void SetLaserVisible(bool bVisible);
    void TickInteraction();
};
