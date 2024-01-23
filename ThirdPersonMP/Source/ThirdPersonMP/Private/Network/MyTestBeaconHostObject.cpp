// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/MyTestBeaconHostObject.h"

#include "Network/MyTestBeaconClient.h"

AMyTestBeaconHostObject::AMyTestBeaconHostObject(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	AMyTestBeaconHostObject* ArchType = Cast<AMyTestBeaconHostObject>(ObjectInitializer.GetArchetype());
	if (ArchType && ArchType->BeaconClientClass)
	{
		ClientBeaconActorClass = ArchType->BeaconClientClass;
	}
	else
	{
		ClientBeaconActorClass = AMyTestBeaconClient::StaticClass();
	}

	BeaconTypeName = ClientBeaconActorClass->GetName();
}

void AMyTestBeaconHostObject::OnClientConnected(AOnlineBeaconClient* NewClientActor, UNetConnection* ClientConnection)
{
	AOnlineBeaconHostObject::OnClientConnected(NewClientActor, ClientConnection);
}
