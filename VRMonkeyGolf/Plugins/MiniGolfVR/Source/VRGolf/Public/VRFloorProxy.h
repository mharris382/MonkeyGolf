// VRFloorProxy.h
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "VRGolfBall.h"
#include "VRFloorProxy.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnProxyLanded, class AVRFloorProxy*, Proxy, const FHitResult&, LandHit);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnProxyAirborne, class AVRFloorProxy*, Proxy);

UCLASS()
class VRGOLF_API AVRFloorProxy : public AActor
{
    GENERATED_BODY()

public:
    AVRFloorProxy();

    // Called by the ball each tick when grounded — updates XY/Z from floor hit, never rotation
    void UpdateFromFloorHit(const FVector& FloorPosition, const FHitResult& Hit);

    // Called by the ball when it leaves the ground
    void NotifyAirborne();

    // Called by the ball when it lands
    void NotifyLanded(const FHitResult& Hit);




    // Returns the up vector of the floor surface — safe to call in air (returns last known)
    UFUNCTION(BlueprintPure, Category = "GolfBall|FloorProxy")
    FVector GetFloorSurfaceNormal() const { return LastFloorNormal; }

    // Returns the floor plane as a world-space up vector (for club projection)
    UFUNCTION(BlueprintPure, Category = "GolfBall|FloorProxy")
    FVector GetFloorSurfacePlane() const { return LastFloorNormal; }

    UFUNCTION(BlueprintPure, Category = "GolfBall|FloorProxy")
    bool IsGrounded() const { return bIsGrounded; }

    // Last floor hit result — valid even when airborne (stale but safe)
    UFUNCTION(BlueprintPure, Category = "GolfBall|FloorProxy")
    const FHitResult& GetLastFloorHit() const { return LastFloorHit; }

    UPROPERTY(BlueprintAssignable, Category = "GolfBall|FloorProxy")
    FOnProxyLanded OnProxyLanded;
    
    
    
    

    UPROPERTY(BlueprintAssignable, Category = "GolfBall|FloorProxy")
    FOnProxyAirborne OnProxyAirborne;



    UFUNCTION(BlueprintNativeEvent, Category = "GolfBall|FloorProxy|Events")
    void NotifyBallStateChanged(const EBallState NewState);
    void NotifyBallStateChanged_Implementation(const EBallState NewState) {};


	UFUNCTION(BlueprintNativeEvent, Category = "GolfBall|FloorProxy|Events")
	void OnBallAssigned(AVRGolfBall* Ball);
    void OnBallAssigned_Implementation(AVRGolfBall* Ball) {}


    UFUNCTION(BlueprintNativeEvent, Category = "GolfBall|FloorProxy|Events")
    void OnBallHit(const FHitResult Hit);
    void OnBallHit_Implementation(const FHitResult Hit) {}

private:
    bool bIsGrounded = false;
    FVector LastFloorNormal = FVector::UpVector;
    FHitResult LastFloorHit;
};