// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyTestBeaconClient.h"
#include "GameFramework/Actor.h"
#include "MyBeaconPlayerController.generated.h"

UCLASS()
class THIRDPERSONMP_API AMyBeaconPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyBeaconPlayerController();
	
	virtual void GetSeamlessTravelActorList(bool bToEntry, TArray<class AActor*>& ActorList) override;

	UPROPERTY(Transient, BlueprintReadWrite, EditAnywhere)
	AMyTestBeaconClient* TestBeaconClient;
};
