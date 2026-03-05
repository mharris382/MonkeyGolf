#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "Engine/EngineTypes.h" // Might be needed for EPhysicalSurface
#include "GolfingSurfaceMappingEntry.generated.h"



UENUM(BlueprintType)
enum class EGolfSurfaceBehavior : uint8
{
    Puttable = 0,
    Collide = 1,
    Destroy = 2
};


//b/c putting is kinematic, we should decouple putting specific settings from the actual surface phyiscal material
USTRUCT(BlueprintType)
struct FGolfSurfacePuttingProfile
{
    GENERATED_BODY()

    //this mechanic does not exist in walkable.  Essentially think of an actual tee instead of a putting green, where you can strike a ball into the air.  1 is exactly like walkabout mini-golf, 0 is extreme and would likely never be used (glub aligned force, putting direction ignored).   
    //1 is perfectly flat, ball force follows the surface perfectly.   0 means the ball can actually be hit upwards from this surface (instead of hit force following surface normal precisely)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface|Putting", meta = (UIMin = "0.0", UIMax = "1.0"))
    float PuttingSurfaceFlattness = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface|Putting", meta = (InlineEditConditionToggle))
    bool bOverridePuttingSurfaceFriction = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface|Putting", meta = (EditCondition = "bOverridePuttingSurfaceFriction"))
    float PuttingSurfaceFriction = 1.0f;

    float GetSurfaceFriction(TObjectPtr<UPhysicalMaterial> physicsMaterial)
    {
        if (!physicsMaterial || bOverridePuttingSurfaceFriction)
            return PuttingSurfaceFriction;
        return physicsMaterial->Friction;
    }
};


//b/c putting is kinematic, we should decouple putting specific settings from the actual surface phyiscal material
USTRUCT(BlueprintType)
struct FGolfSurfaceBaseFeedbacks
{
    GENERATED_BODY()

    
};

USTRUCT(BlueprintType)
struct FGolfingSurfaceMappingEntry : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface")
    TEnumAsByte<EPhysicalSurface> SurfaceType = EPhysicalSurface::SurfaceType_Default;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface", meta = (EditCondition = "SurfaceBehavior!=EGolfSurfaceBehavior::Destroy", EditConditionHides))
    TSoftObjectPtr<UPhysicalMaterial> DefaultSurfacePhysicsMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface")
    EGolfSurfaceBehavior SurfaceBehavior = EGolfSurfaceBehavior::Puttable;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface", meta = (EditCondition = "SurfaceBehavior==EGolfSurfaceBehavior::Puttable", EditConditionHides))
    FGolfSurfacePuttingProfile PuttingProfile;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface", meta = (EditCondition = "SurfaceBehavior==EGolfSurfaceBehavior::Puttable", EditConditionHides))
    TSoftObjectPtr<UMaterialInterface> DefaultSurfaceMaterial;


    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface|Feedbacks")
    FGameplayTag ImpactFeedbackTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GolfSurface|Feedbacks", meta = (EditCondition = "SurfaceBehavior!=EGolfSurfaceBehavior::Destroy", EditConditionHides))
    FGameplayTag BounceFeedbackTag;
};