// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGGame_TeamDeathMatch.h"

#include "OnlineRPGGameState.h"
#include "OnlineRPGPlayerState.h"

AOnlineRPGGame_TeamDeathMatch::AOnlineRPGGame_TeamDeathMatch(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 2;
	bDelayedStart = true;
}

void AOnlineRPGGame_TeamDeathMatch::PostLogin(APlayerController* NewPlayer)
{
	// Place player on a team before Super (VoIP team based init, findplayerstart, etc)
	AOnlineRPGPlayerState* NewPlayerState = CastChecked<AOnlineRPGPlayerState>(NewPlayer->PlayerState);
	const int32 TeamNum = ChooseTeam(NewPlayerState);
	NewPlayerState->SetTeamNum(TeamNum);

	Super::PostLogin(NewPlayer);
}

void AOnlineRPGGame_TeamDeathMatch::InitGameState()
{
	Super::InitGameState();

	AOnlineRPGGameState* const MyGameState = Cast<AOnlineRPGGameState>(GameState);
	if (MyGameState)
	{
		MyGameState->NumTeams = NumTeams;
	}
}

bool AOnlineRPGGame_TeamDeathMatch::CanDealDamage(AOnlineRPGPlayerState* DamageInstigator, class AOnlineRPGPlayerState* DamagedPlayer) const
{
	return DamageInstigator && DamagedPlayer && (DamagedPlayer == DamageInstigator || DamagedPlayer->GetTeamNum() != DamageInstigator->GetTeamNum());
}

int32 AOnlineRPGGame_TeamDeathMatch::ChooseTeam(AOnlineRPGPlayerState* ForPlayerState) const
{
	TArray<int32> TeamBalance;
	TeamBalance.AddZeroed(NumTeams);

	// get current team balance
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AOnlineRPGPlayerState const* const TestPlayerState = Cast<AOnlineRPGPlayerState>(GameState->PlayerArray[i]);
		if (TestPlayerState && TestPlayerState != ForPlayerState && TeamBalance.IsValidIndex(TestPlayerState->GetTeamNum()))
		{
			TeamBalance[TestPlayerState->GetTeamNum()]++;
		}
	}

	// find least populated one
	int32 BestTeamScore = TeamBalance[0];
	for (int32 i = 1; i < TeamBalance.Num(); i++)
	{
		if (BestTeamScore > TeamBalance[i])
		{
			BestTeamScore = TeamBalance[i];
		}
	}

	// there could be more than one...
	TArray<int32> BestTeams;
	for (int32 i = 0; i < TeamBalance.Num(); i++)
	{
		if (TeamBalance[i] == BestTeamScore)
		{
			BestTeams.Add(i);
		}
	}

	// get random from best list
	const int32 RandomBestTeam = BestTeams[FMath::RandHelper(BestTeams.Num())];
	return RandomBestTeam;
}

void AOnlineRPGGame_TeamDeathMatch::DetermineMatchWinner()
{
	AOnlineRPGGameState const* const MyGameState = Cast<AOnlineRPGGameState>(GameState);
	int32 BestScore = MIN_uint32;
	int32 BestTeam = -1;
	int32 NumBestTeams = 1;

	for (int32 i = 0; i < MyGameState->TeamScores.Num(); i++)
	{
		const int32 TeamScore = MyGameState->TeamScores[i];
		if (BestScore < TeamScore)
		{
			BestScore = TeamScore;
			BestTeam = i;
			NumBestTeams = 1;
		}
		else if (BestScore == TeamScore)
		{
			NumBestTeams++;
		}
	}

	WinnerTeam = (NumBestTeams == 1) ? BestTeam : NumTeams;
}

bool AOnlineRPGGame_TeamDeathMatch::IsWinner(AOnlineRPGPlayerState* PlayerState) const
{
	return PlayerState && !PlayerState->IsQuitter() && PlayerState->GetTeamNum() == WinnerTeam;
}

bool AOnlineRPGGame_TeamDeathMatch::IsSpawnpointAllowed(APlayerStart* SpawnPoint, AController* Player) const
{
	if (Player)
	{
		// AOnlineRPGTeamStart* TeamStart = Cast<AOnlineRPGTeamStart>(SpawnPoint);
		// AOnlineRPGPlayerState* PlayerState = Cast<AOnlineRPGPlayerState>(Player->PlayerState);
		//
		// if (PlayerState && TeamStart && TeamStart->SpawnTeam != PlayerState->GetTeamNum())
		// {
		// 	return false;
		// }
	}

	return Super::IsSpawnpointAllowed(SpawnPoint, Player);
}

void AOnlineRPGGame_TeamDeathMatch::InitBot(AOnlineRPGAIController* AIC, int32 BotNum)
{	
	// AOnlineRPGPlayerState* BotPlayerState = CastChecked<AOnlineRPGPlayerState>(AIC->PlayerState);
	// const int32 TeamNum = ChooseTeam(BotPlayerState);
	// BotPlayerState->SetTeamNum(TeamNum);		

	Super::InitBot(AIC, BotNum);
}