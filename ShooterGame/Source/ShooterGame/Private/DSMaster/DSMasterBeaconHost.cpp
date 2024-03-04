// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMaster/DSMasterBeaconHost.h"
#include "DSMaster/DSMasterBeaconClient.h"

ADSMasterBeaconHost::ADSMasterBeaconHost(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ClientBeaconActorClass = ADSMasterBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

void ADSMasterBeaconHost::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	Super::OnClientConnected(NewClientActor, ClientConnection);

	UE_LOG(LogDSMaster, Log, TEXT("DSMasterClient Connected"));
}

void ADSMasterBeaconHost::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	UE_LOG(LogDSMaster, Log, TEXT("DSMasterClient Disconnected"));
	Super::NotifyClientDisconnected(LeavingClientActor);
}
