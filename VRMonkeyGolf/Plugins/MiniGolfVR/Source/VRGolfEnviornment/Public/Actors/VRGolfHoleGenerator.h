// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "Actors/GolfPCGActorBase.h"
#include "VRGolfHoleGenerator.generated.h"



// ---------------------------------------------------------------------------
// Enums
// ---------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ESidesGenPolicy : uint8
{
	None					UMETA(DisplayName = "No Sides"),
	Full					UMETA(DisplayName = "Full Sides"),
	DefaultOn_ExcludeMarked	UMETA(DisplayName = "Sides Everywhere, Exclude Marked"),
	DefaultOff_IncludeMarked UMETA(DisplayName = "No Sides, Include Marked Only"),
};

UENUM(BlueprintType)
enum class EHoleGenMode : uint8
{
	Preview		UMETA(DisplayName = "Preview (Fast, Low-Res)"),
	Baked		UMETA(DisplayName = "Baked (Full Quality, Save to StaticMesh)"),
};

UCLASS()
class VRGOLFENVIORNMENT_API AVRGolfHoleGenerator  : public AGolfPCGActorBase
{
	GENERATED_BODY()
	

   

protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
public:

	// -----------------------------------------------------------------------
// Hole Identity
// -----------------------------------------------------------------------

/** Par for this hole — informational, passed to GameState. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hole")
int32 ParValue = 3;

/** Hole index within the course (1-based). */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hole")
int32 HoleIndex = 1;

/** World-space tee transform. The primary spline(s) originate near here. */
UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hole")
FTransform TeeTransform;



// -----------------------------------------------------------------------
// Generation Policy
// -----------------------------------------------------------------------

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
EHoleGenMode GenerationMode = EHoleGenMode::Preview;

UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
ESidesGenPolicy SidesPolicy = ESidesGenPolicy::DefaultOn_ExcludeMarked;
	
};
