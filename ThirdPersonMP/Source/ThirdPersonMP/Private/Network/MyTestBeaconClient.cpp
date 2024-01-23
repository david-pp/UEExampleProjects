// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/MyTestBeaconClient.h"

#include "Net/UnrealNetwork.h"


// Sets default values
AMyTestBeaconClient::AMyTestBeaconClient()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

void AMyTestBeaconClient::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyTestBeaconClient, BeaconCounter);
}

void AMyTestBeaconClient::OnConnected()
{
	UE_LOG(LogTemp, Log, TEXT("AMyTestBeaconClient - OnConnected"))
	Super::OnConnected();
	OnConnectToHost();
}

void AMyTestBeaconClient::OnFailure()
{
	UE_LOG(LogTemp, Warning, TEXT("AMyTestBeaconClient - OnFailure"))
	Super::OnFailure();
}


void AMyTestBeaconClient::ConnectToHost(FString HostAddress)
{
	FURL ConnectURL(NULL, *HostAddress, TRAVEL_Absolute);
	if (InitClient(ConnectURL))
	{
		UE_LOG(LogTemp, Log, TEXT("AMyTestBeaconClient - ConnectToHost: %s."), *ConnectURL.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AMyTestBeaconClient - ConnectToHost: Failure to init client beacon with %s."), *ConnectURL.ToString());
	}
}

void AMyTestBeaconClient::ServerAddCounter_Implementation(int32 Value)
{
	BeaconCounter += Value;
	UE_LOG(LogTemp, Log, TEXT("AMyTestBeaconClient:%s - ServerAddCounter :%d"), *GetName(), BeaconCounter);

	ClientAddCounterDone(BeaconCounter);
	MulticastAddCounterDone(BeaconCounter);
}

void AMyTestBeaconClient::ClientAddCounterDone_Implementation(int32 CounterValue)
{
	UE_LOG(LogTemp, Log, TEXT("AMyTestBeaconClient:%s - ClientAddCounterDon :%d"), *GetName(), CounterValue);
}

void AMyTestBeaconClient::MulticastAddCounterDone_Implementation(int32 CounterValue)
{
	UE_LOG(LogTemp, Log, TEXT("AMyTestBeaconClient:%s - MulticastAddCounterDone :%d"), *GetName(), CounterValue);
}
