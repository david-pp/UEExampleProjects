// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineRPGGameMode.h"

#include "OnlineRPGGame_FreeForAll.generated.h"

class AOnlineRPGPlayerState;

UCLASS()
class AOnlineRPGGame_FreeForAll : public AOnlineRPGGameMode
{
	GENERATED_UCLASS_BODY()

protected:

	/** best player */
	UPROPERTY(transient)
	AOnlineRPGPlayerState* WinnerPlayerState;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AOnlineRPGPlayerState* PlayerState) const override;
};
