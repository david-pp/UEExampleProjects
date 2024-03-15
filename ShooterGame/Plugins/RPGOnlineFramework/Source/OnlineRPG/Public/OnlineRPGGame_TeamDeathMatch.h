// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineRPGGameMode.h"

#include "OnlineRPGGame_TeamDeathMatch.generated.h"

class AOnlineRPGPlayerState;
class AOnlineRPGAIController;
class APlayerStart;

UCLASS()
class AOnlineRPGGame_TeamDeathMatch : public AOnlineRPGGameMode
{
	GENERATED_UCLASS_BODY()

	/** setup team changes at player login */
	void PostLogin(APlayerController* NewPlayer) override;

	/** initialize replicated game data */
	virtual void InitGameState() override;

	/** can players damage each other? */
	virtual bool CanDealDamage(AOnlineRPGPlayerState* DamageInstigator, AOnlineRPGPlayerState* DamagedPlayer) const override;

protected:

	/** number of teams */
	int32 NumTeams;

	/** best team */
	int32 WinnerTeam;

	/** pick team with least players in or random when it's equal */
	int32 ChooseTeam(AOnlineRPGPlayerState* ForPlayerState) const;

	/** check who won */
	virtual void DetermineMatchWinner() override;

	/** check if PlayerState is a winner */
	virtual bool IsWinner(AOnlineRPGPlayerState* PlayerState) const override;

	/** check team constraints */
	virtual bool IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const;

	/** initialization for bot after spawning */
	virtual void InitBot(AOnlineRPGAIController* AIC, int32 BotNum) override;	
};
