// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "OnlineRPGDemoSpectator.generated.h"

class SOnlineRPGDemoHUD;

UCLASS(config=Game)
class ONLINERPG_API AOnlineRPGDemoSpectator : public APlayerController
{
	GENERATED_UCLASS_BODY()

public:
	/** shooter in-game menu */
	TSharedPtr<class FOnlineRPGDemoPlaybackMenu> OnlineRPGDemoPlaybackMenu;

	virtual void SetupInputComponent() override;
	virtual void SetPlayer( UPlayer* Player ) override;
	virtual void Destroyed() override;

	void OnToggleInGameMenu();
	void OnIncreasePlaybackSpeed();
	void OnDecreasePlaybackSpeed();

	int32 PlaybackSpeed;

private:
	TSharedPtr<SOnlineRPGDemoHUD> DemoHUD;
};

