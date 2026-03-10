#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
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

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CourseGen")
	TSoftObjectPtr<UMaterialInterface> FlatGreenMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CourseGen")
    TSoftObjectPtr<UMaterialInterface> SlopedGreenMaterial;

};