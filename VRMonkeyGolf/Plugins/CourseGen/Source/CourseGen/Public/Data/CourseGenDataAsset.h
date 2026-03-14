#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Data/ProceduralEmbed.h"
#include "PCGGraph.h"
#include "Materials/MaterialInterface.h"
#include "CourseGenDataAsset.generated.h"





// TODO: ADD a project settings to provide global default values

/**
 * UCourseGenDataAsset
 *
 *  Visual Configurations shared by all holes on a course.  Used in PCG and in CourseGenCore baking process.   A specific hole can override this, but we can also add a CourseActor
 *
 * V0 — minimal. Expanded in future passes.
 */
UCLASS(BlueprintType)
class COURSEGEN_API UCourseGenDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CourseGen|Materials")
	TSoftObjectPtr<UMaterialInterface> FlatGreenMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CourseGen|Materials")
    TSoftObjectPtr<UMaterialInterface> SlopedGreenMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseGen|Cup")
    FProceduralEmbed DefaultCupEmbed;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseGen|Cup")
    TSoftObjectPtr<UMaterialInterface> CupMaterial;




	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseGen|Generation", meta = (InlineEditConditionToggle))
    bool bUsePreProcessGreenGraph = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseGen|Generation", meta = (EditCondition = "bUsePreProcessGreenGraph", Tooltip = "In Dynamic Mesh (multiple) -> Output Dynamic Mesh.  used to pre-process the green dynamic mesh before it has green materials applied."))
    TScriptInterface<UPCGGraphInterface> PreProcessGreenGraph;


    
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseGen|Generation", meta = (InlineEditConditionToggle))
    bool bUsePostProcessGreenGraph = false;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "CourseGen|Generation", meta = (EditCondition = "bUsePostProcessGreenGraph", Tooltip = "In Dynamic Mesh (single) -> Output Dynamic Mesh.  Allows modifying the mesh before baking occurs."))
    TScriptInterface<UPCGGraphInterface> PostProcessGreenGraph;
};