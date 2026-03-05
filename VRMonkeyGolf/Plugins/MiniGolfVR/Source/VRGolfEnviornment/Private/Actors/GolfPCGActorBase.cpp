// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/GolfPCGActorBase.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "PCGComponent.h"

// Sets default values
AGolfPCGActorBase::AGolfPCGActorBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 	RootComponent = Root;
	BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	BoundsBox->SetupAttachment(RootComponent);
	BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsBox->SetGenerateOverlapEvents(false);


	PCGComponent = CreateDefaultSubobject<UPCGComponent>(TEXT("PCG"));
	PCGComponent->bGenerateOnDropWhenTriggerOnDemand = true;
#if WITH_EDITOR
	PCGComponent->bRegenerateInEditor = true;
#endif // WITH_EDITOR

	//BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PCG"));
}

// Called when the game starts or when spawned
void AGolfPCGActorBase::BeginPlay()
{
	Super::BeginPlay();
	
}

void AGolfPCGActorBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateBoundsFromMinMax();
	UpdatePCGComponentSettings();
	UpdateSaveName();
}

#if WITH_EDITOR

void AGolfPCGActorBase::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdateBoundsFromMinMax();
	UpdatePCGComponentSettings();
	UpdateSaveName();
}

#endif

void AGolfPCGActorBase::UpdateBoundsFromMinMax()
{
	if (!BoundsBox)
		return;
	FVector Min, Max;
	GetBoundsMinMax(Min, Max);
	FVector Center = (Min + Max) / 2.0f;
	FVector Extent = (Max - Min) / 2.0f + BoundsPadding;
	BoundsBox->SetWorldLocation(Center);
	BoundsBox->SetBoxExtent(Extent);
	BoundsBox->SetVisibility(bShowBoundingBox);
}

void AGolfPCGActorBase::UpdatePCGComponentSettings()
{
	
}

void AGolfPCGActorBase::UpdateSaveName()
{
	GEN_AssetSaveName = FString::Printf(TEXT("%s_%s"), *AssetSaveName, *AssetSaveName);
}

