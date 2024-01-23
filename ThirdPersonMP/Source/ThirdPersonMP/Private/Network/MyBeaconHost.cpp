// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/MyBeaconHost.h"


// Sets default values
AMyBeaconHost::AMyBeaconHost()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyBeaconHost::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AMyBeaconHost::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}


