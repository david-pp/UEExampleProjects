// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/MyBeaconPlayerController.h"


// Sets default values
AMyBeaconPlayerController::AMyBeaconPlayerController()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AMyBeaconPlayerController::GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToEntry, ActorList);

	if (TestBeaconClient)
	{
		ActorList.Add(TestBeaconClient);
	}
}


