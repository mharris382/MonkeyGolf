// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGComponent.h"
#include "Components/BoxComponent.h"
#include "Components/DynamicMeshComponent.h"
#include "GolfPCGActorBase.generated.h"


UCLASS()
class VRGOLFENVIORNMENT_API AGolfPCGActorBase : public AActor
{
	GENERATED_BODY()


public:

	// Sets default values for this actor's properties
	AGolfPCGActorBase();


protected:
	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	
public:

	// -----------------------------------------------------------------------
	// Scene Components
	// -----------------------------------------------------------------------

	/** Defines the working volume — PCG collects splines and marker actors inside this. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UBoxComponent> BoundsBox;

	/** Drives the entire generation pipeline. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UPCGComponent> PCGComponent;

	/**
	 * Preview-mode output. Populated by SpawnDynamicMeshActor PCG node in
	 * Preview mode; hidden / destroyed when baking to StaticMesh.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UDynamicMeshComponent> PreviewMeshComponent;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	int32 Seed = 1657634;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	FVector BoundsPadding = FVector(100.0f, 100.0f, 100.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Core")
	bool bShowBoundingBox = false;



	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core|Output")
	FString AssetSaveName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Core|Output")
	FDirectoryPath OutputDirectory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly , Category = "Core|Output", AdvancedDisplay)
	FString GEN_AssetSaveName;



protected:

	UFUNCTION(BlueprintNativeEvent, Category = "PCG|Bounds")
	void GetBoundsMinMax(FVector& OutMin, FVector& OutMax) const;
	void GetBoundsMinMax_Implementation(FVector& OutMin, FVector& OutMax) const
	{
		OutMin = FVector::ZeroVector;
		OutMax = FVector::ZeroVector;
	}

	


private:
	void UpdateBoundsFromMinMax();
	void UpdatePCGComponentSettings();
	void UpdateSaveName();

};
