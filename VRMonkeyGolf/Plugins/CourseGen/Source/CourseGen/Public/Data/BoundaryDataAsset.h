#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PCGGraph.h"
#include "Materials/MaterialInterface.h"
#include "BoundaryDataAsset.generated.h"


/*
generation scope defines which template graph the data asset uses.  The scope defines where this graph is called from.  
 Larger scopes have more spatial awareness but are heavier because they have to regenerate the whole thing whenever anything within the scope changes and have to process/output more data.  
 
 The scope determines where it will be called from.   Both per hole and per spline are called individually

 This data asset contains a PCG generation graph, which will be called after green generation completes (before bake).  
*/
UENUM(BlueprintType)
enum class EBoundaryGenerationScope : uint8
{
    PerCourse = 0 UMETA(DisplayName = "Generated Per Course", Tooltip = "Executes once for a CourseActor.   It recieves splines and the PCG output from all CourseHoleActors on the CourseActor. "),
    PerHole = 1 UMETA(DisplayName = "Generated Per Hole", Tooltip = "Executes once for a CourseHoleActor. Does not have awareness of other holes. It recieves the splines and PCG output from a single CourseHoleActors on the CourseActor.")
};

// TODO: ADD a project settings to provide global default values

/**
 * UBoundaryDataAsset
 *
 *  Visual Configurations shared by all holes on a course.  Used in PCG and in CourseGenCore baking process.   A specific hole can override this, but we can also add a CourseActor
 *
 * V0 — minimal. Expanded in future passes.
 */
UCLASS(BlueprintType)
class COURSEGEN_API UBoundaryDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
    

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BoundaryGeneration")
    TScriptInterface<UPCGGraphInterface> BoundaryGenerationGraph;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="BoundaryGeneration")
    TSoftObjectPtr<UMaterialInterface> OverrideBoundaryMaterial;


    // const bool IsDataAssetValid()
    // {
        
    // }
};