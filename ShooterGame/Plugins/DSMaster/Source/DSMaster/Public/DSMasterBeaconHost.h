// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMaster.h"
#include "OnlineBeaconHostObject.h"
#include "DSMasterBeaconHost.generated.h"

UCLASS()
class DSMASTER_API ADSMasterBeaconHost : public AOnlineBeaconHostObject
{
	GENERATED_BODY()

	ADSMasterBeaconHost(const FObjectInitializer& ObjectInitializer);

	//~ Begin AOnlineBeaconHostObject Interface
	virtual void OnClientConnected(class AOnlineBeaconClient* NewClientActor, class UNetConnection* ClientConnection) override;
	void NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor) override;
	//~ End AOnlineBeaconHostObject Interface 
};
