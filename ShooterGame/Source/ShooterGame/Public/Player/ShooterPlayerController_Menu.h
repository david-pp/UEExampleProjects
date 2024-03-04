// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "DSMasterBeaconClient.h"
#include "ShooterTypes.h"
#include "ShooterPlayerController_Menu.generated.h"


UCLASS()
class AShooterPlayerController_Menu : public APlayerController
{
	GENERATED_UCLASS_BODY()
	/** After game is initialized */
	virtual void PostInitializeComponents() override;

public:
	UPROPERTY(Transient)
	ADSMasterBeaconClient* DSMasterClient;

	UFUNCTION(BlueprintCallable, Category=DSMaster)
	bool ConnectToMasterServer(FString ServerAddress);

	UFUNCTION(BlueprintPure, Category=DSMaster)
	ADSMasterBeaconClient* GetDSMasterClient() const
	{
		return DSMasterClient;
	}
};
