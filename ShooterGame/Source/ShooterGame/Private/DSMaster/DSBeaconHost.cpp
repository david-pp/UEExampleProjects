// Fill out your copyright notice in the Description page of Project Settings.

#include "DSMaster/DSBeaconHost.h"
#include "DSMaster/DSBeaconClient.h"

ADSBeaconHost::ADSBeaconHost(const FObjectInitializer& ObjectInitializer)
{
	ClientBeaconActorClass = ADSBeaconClient::StaticClass();
	BeaconTypeName = ClientBeaconActorClass->GetName();
}

void ADSBeaconHost::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	Super::OnClientConnected(NewClientActor, ClientConnection);
	UE_LOG(LogDSMaster, Log, TEXT("DSClient Connected"));
}

void ADSBeaconHost::NotifyClientDisconnected(AOnlineBeaconClient* LeavingClientActor)
{
	UE_LOG(LogDSMaster, Log, TEXT("DSClient Disconnected"));
	Super::NotifyClientDisconnected(LeavingClientActor);
}
