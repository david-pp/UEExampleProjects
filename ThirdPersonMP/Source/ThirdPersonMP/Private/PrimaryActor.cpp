// Fill out your copyright notice in the Description page of Project Settings.


#include "PrimaryActor.h"

const FPrimaryAssetType APrimaryActor::PrimaryActorType = FName(TEXT("Actor"));

// Sets default values
APrimaryActor::APrimaryActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void APrimaryActor::BeginPlay()
{
	Super::BeginPlay();
	
}

FPrimaryAssetId APrimaryActor::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryActorType, GetFName());
}

// Called every frame
void APrimaryActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

