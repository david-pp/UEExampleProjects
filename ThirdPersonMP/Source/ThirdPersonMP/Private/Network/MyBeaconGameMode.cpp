// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/MyBeaconGameMode.h"

#include "Network/MyBeaconHost.h"
#include "Network/MyTestBeaconClient.h"
#include "Network/MyTestBeaconHostObject.h"


// Sets default values
AMyBeaconGameMode::AMyBeaconGameMode()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
}

void AMyBeaconGameMode::StartPlay()
{
	Super::StartPlay();

	if (bCreateBeaconHostOnStart)
	{
		CreateBeaconHost();
	}
}

bool AMyBeaconGameMode::CreateBeaconHost()
{
	BeaconHost = GetWorld()->SpawnActor<AMyBeaconHost>(AMyBeaconHost::StaticClass());
	if (BeaconHost)
	{
		BeaconHost->ListenPort = BeaconHostPort;
		if (BeaconHost->InitHost())
		{
			// Set Beacon state : EBeaconState::AllowRequests
			BeaconHost->PauseBeaconRequests(false);

			UE_LOG(LogTemp, Warning, TEXT("Beacon - InitHost"))

			UClass* BeaconHostObjectClass = TestBeaconHostObjectClass ? TestBeaconHostObjectClass : AMyTestBeaconHostObject::StaticClass();
			TestBeaconHostObject = GetWorld()->SpawnActor<AMyTestBeaconHostObject>(BeaconHostObjectClass);
			if (TestBeaconHostObject)
			{
				BeaconHost->RegisterHost(TestBeaconHostObject);
				UE_LOG(LogTemp, Warning, TEXT("Beacon - RegisterHost"))
				return true;
			}
		}
	}

	return false;
}

void AMyBeaconGameMode::ConnectToServer(FString ServerAddress)
{
	UClass* BeaconClientClass = ServerBeaconClientClass ? ServerBeaconClientClass : AMyTestBeaconClient::StaticClass();
	ServerBeaconClient = GetWorld()->SpawnActor<AMyTestBeaconClient>(BeaconClientClass);
	if (ServerBeaconClient)
	{
		FURL ConnectURL(NULL, *ServerAddress, TRAVEL_Absolute);
		if (ServerBeaconClient->InitClient(ConnectURL))
		{
			UE_LOG(LogTemp, Log, TEXT("AMyBeaconGameMode - ConnectToServer: %s."), *ConnectURL.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("AMyBeaconGameMode - ConnectToServer: Failure to init client beacon with %s."), *ConnectURL.ToString());
		}
	}
}

void AMyBeaconGameMode::SayHelloToServerY()
{
	if (ServerBeaconClient)
	{
		ServerBeaconClient->HelloServerY();
	}
}
