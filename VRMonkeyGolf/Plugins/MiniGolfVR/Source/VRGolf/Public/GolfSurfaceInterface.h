// GolfSurfaceInterface.h
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GolfSurfaceInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UGolfSurfaceInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * Interface for surfaces that golf balls can interact with
 * Allows actors to define custom physics properties for golf gameplay
 */
class IGolfSurfaceInterface
{
    GENERATED_BODY()

public:
    // Called when ball lands on this surface
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golf")
    bool IsPuttableSurface() const;
    bool IsPuttableSurface_Implementation() const { return true; }

    // Optional: custom friction for this surface
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golf")
    float GetSurfaceFriction() const;
    float GetSurfaceFriction_Implementation() const { return 1.0f; }

    // Optional: custom bounciness for this surface
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Golf")
    float GetSurfaceBounciness() const;
    float GetSurfaceBounciness_Implementation() const { return 0.0f; }
};
