#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ProceduralSlot.h"

#include "CourseHoleDataAsset.generated.h"

/**
 * UCourseHoleDataAsset
 *
 * Lightweight runtime data contract for a single hole.
 * Written by ADynamicMeshBakeActor at cook time.
 * Read by ACourseHoleActor at BeginPlay.
 *
 * V0 — minimal. Expanded in future passes.
 */
UCLASS(BlueprintType)
class COURSEGEN_API UCourseHoleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Hole index within the course. Set by the bake actor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CourseHole")
	int32 HoleIndex = INDEX_NONE;

	/** All slot data for this hole — tee + cup candidates. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CourseHole")
	FCourseHoleSlotCollection SlotCollection;

	/** Baked static mesh asset produced by ADynamicMeshBakeActor. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="CourseHole")
	TSoftObjectPtr<UStaticMesh> BakedGreenMesh;

    

	bool IsValid() const
	{
		return HoleIndex != INDEX_NONE
			&& SlotCollection.IsValid()
			&& !BakedGreenMesh.IsNull();
	}
};