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
 * BP subclass sets DefaultWidgetClass in defaults and handles
 * open/close animations via OnMenuShown / OnMenuHidden overrides.
 *
 * Editor preview: a box component matches the widget draw size so you
 * can see position and scale in the viewport before PIE.
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
 
#if WITH_EDITORONLY_DATA
    /** Editor-only box showing the widget plane size in world space. Hidden at runtime. */
    UPROPERTY(VisibleAnywhere, Category = "VR Menu|Editor")
    TObjectPtr<class UBoxComponent> PreviewBox;
 
    /** Editor-only billboard so the actor is selectable when no widget is visible. */
    UPROPERTY(VisibleAnywhere, Category = "VR Menu|Editor")
    TObjectPtr<class UBillboardComponent> Billboard;
#endif
 
    // --- Config ---
 
    /** Widget class to instantiate. Set this in your BP subclass defaults. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "VR Menu")
    TSubclassOf<UUserWidget> DefaultWidgetClass;
 
    /**
     * Distance from player camera when snapping (cm).
     * 100cm = 1m in front of player.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Menu")
    float SnapDistance = 100.0f;
 
    /**
     * UMG draw size in pixels. Also drives the editor preview box size.
     * Changing this in editor updates the preview box immediately.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Menu", meta = (ClampMin = "1.0"))
    FVector2D WidgetDrawSize = FVector2D(800.f, 600.f);
 
    /**
     * Scale applied to the widget in world space.
     * 0.1 = each pixel is 0.1cm → 800px wide = 80cm wide panel.
     * Adjust to change physical menu size without changing widget resolution.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Menu")
    float WidgetWorldScale = 0.1f;
 
    /**
     * If true, menu shows automatically on BeginPlay at its placed location.
     * If false, call SetMenuVisible() from GameMode or BP.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "VR Menu")
    bool bShowOnBeginPlay = false;
 
    // --- BP hooks ---
 
    /** Called after the widget becomes visible. Wire open animation here. */
    UFUNCTION(BlueprintNativeEvent, Category = "VR Menu")
    void OnMenuShown();
    virtual void OnMenuShown_Implementation() {}
 
    /** Called before the widget is hidden. Wire close animation here. */
    UFUNCTION(BlueprintNativeEvent, Category = "VR Menu")
    void OnMenuHidden();
    virtual void OnMenuHidden_Implementation() {}
 
    virtual void BeginPlay() override;
 
#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
 
private:
    bool bMenuVisible = false;
 
    void SnapToPlayerCamera();
    void SyncPreviewBox();
};