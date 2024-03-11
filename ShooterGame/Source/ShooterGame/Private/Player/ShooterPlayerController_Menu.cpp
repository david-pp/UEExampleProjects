// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterGame.h"
#include "Player/ShooterPlayerController_Menu.h"

#include "DSMaster.h"
#include "DSMasterBeaconClient.h"
#include "ShooterStyle.h"


AShooterPlayerController_Menu::AShooterPlayerController_Menu(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void AShooterPlayerController_Menu::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	FShooterStyle::Initialize();
}

bool AShooterPlayerController_Menu::ConnectToMasterServer(FString ServerAddress)
{
	DSMasterClient = GetWorld()->SpawnActor<ADSMasterBeaconClient>(ADSMasterBeaconClient::StaticClass());
	if (DSMasterClient)
	{
		DSMasterClient->Settings.ClientType = EDSMasterClientType::GameClient;
		return DSMasterClient->ConnectToMasterServer(ServerAddress);
	}
	return false;
}
