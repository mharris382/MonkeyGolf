// VRGolfPlayerPawn.h
#pragma once

#include "CoreMinimal.h"
#include "VRCharacter.h"
#include "VRGolfPlayerPawn.generated.h"

class AVRGolfPutter;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;

UENUM(BlueprintType)
enum class EGolfActiveHand : uint8
{
    Right,
    Left
};

/**
 * VRGolfPlayerPawn
 *
 * Inherits AVRCharacter (VRExpansionPlugin). Strips down to mini golf needs:
 *   - Single putter attached to whichever hand last sent input (auto-switch, no setting)
 *   - Both controllers mapped identically; hand is determined by which IA fires
 *   - Only the active controller mesh is visible at any time
 *   - HMD body-collision fade disabled
 */
UCLASS()
class VRGOLFPLAYER_API AVRGolfPlayerPawn : public AVRCharacter
{
    GENERATED_BODY()

public:
    AVRGolfPlayerPawn(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

    // --- Active hand query ---
    UFUNCTION(BlueprintPure, Category = "Golf|Hand")
    EGolfActiveHand GetActiveHand() const { return ActiveHand; }

    UFUNCTION(BlueprintPure, Category = "Golf|Hand")
    UGripMotionControllerComponent* GetActiveController() const;

    UFUNCTION(BlueprintPure, Category = "Golf|Hand")
    UGripMotionControllerComponent* GetInactiveController() const;

    // --- Putter access ---
    UFUNCTION(BlueprintPure, Category = "Golf|Putter")
    AVRGolfPutter* GetPutter() const { return Putter; }

    // --- Input Mapping ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Golf|Input")
    TObjectPtr<UInputMappingContext> GolfMappingContext;

    // Any input on left hand (trigger, grip, thumbstick — your choice in the asset)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Golf|Input")
    TObjectPtr<UInputAction> IA_LeftHandActive;

    // Any input on right hand
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Golf|Input")
    TObjectPtr<UInputAction> IA_RightHandActive;

    // Teleport (bound to both controllers in the IMC, disambiguated by hand)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Golf|Input")
    TObjectPtr<UInputAction> IA_Teleport_Left;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Golf|Input")
    TObjectPtr<UInputAction> IA_Teleport_Right;

    // --- Putter class to spawn ---
    UPROPERTY(EditDefaultsOnly, Category = "Golf|Putter")
    TSubclassOf<AVRGolfPutter> PutterClass;

protected:
    // Called when any left-hand input fires
    void OnLeftHandInput(const FInputActionValue& Value);

    // Called when any right-hand input fires
    void OnRightHandInput(const FInputActionValue& Value);

    void OnTeleportLeft(const FInputActionValue& Value);
    void OnTeleportRight(const FInputActionValue& Value);

    // Core hand switch — re-attaches putter and swaps visibility
    void SetActiveHand(EGolfActiveHand NewHand);

    // Spawn putter and attach to current active controller
    void SpawnAndAttachPutter();

    // Update which controller mesh is visible
    void RefreshControllerVisibility();

    // Teleport implementation shared between hands
    void ExecuteTeleport(EGolfActiveHand FromHand);

private:
    UPROPERTY()
    TObjectPtr<AVRGolfPutter> Putter;

    EGolfActiveHand ActiveHand = EGolfActiveHand::Right;

    // Tracks whether the putter has been spawned (deferred to BeginPlay)
    bool bPutterSpawned = false;
};