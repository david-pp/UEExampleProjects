// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TestBeaconHost.h"
#include "OnlineBeaconClient.h"
#include "MyTestBeaconHostObject.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API AMyTestBeaconHostObject : public ATestBeaconHost
{
	GENERATED_UCLASS_BODY()

	//~ Begin AOnlineBeaconHost Interface 
	virtual void OnClientConnected(class AOnlineBeaconClient* NewClientActor, class UNetConnection* ClientConnection) override;
	//~ End AOnlineBeaconHost Interface
	
public:
	/** Class reference for spawning client beacon actor */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AOnlineBeaconClient> BeaconClientClass;
};
