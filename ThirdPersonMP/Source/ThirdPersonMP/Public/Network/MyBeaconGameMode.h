// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MyBeaconHost.h"
#include "MyTestBeaconHostObject.h"
#include "GameFramework/GameModeBase.h"
#include "MyBeaconGameMode.generated.h"

UCLASS()
class THIRDPERSONMP_API AMyBeaconGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyBeaconGameMode();

	virtual void StartPlay() override;

	UFUNCTION(BlueprintCallable)
	bool CreateBeaconHost();

	UPROPERTY(Transient)
	AMyBeaconHost* BeaconHost;
	
	UPROPERTY(Transient)
	AMyTestBeaconHostObject* TestBeaconHostObject;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Beacon)
	bool bCreateBeaconHostOnStart = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Beacon)
	TSubclassOf<AMyTestBeaconHostObject> TestBeaconHostObjectClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Beacon)
	int32 BeaconHostPort = 9999;
};
