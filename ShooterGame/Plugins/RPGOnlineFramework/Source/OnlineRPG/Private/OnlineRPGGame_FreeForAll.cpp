// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGGame_FreeForAll.h"

#include "OnlineRPGGameState.h"
#include "OnlineRPGPlayerState.h"

AOnlineRPGGame_FreeForAll::AOnlineRPGGame_FreeForAll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDelayedStart = true;
}

void AOnlineRPGGame_FreeForAll::DetermineMatchWinner()
{
	AOnlineRPGGameState const* const MyGameState = CastChecked<AOnlineRPGGameState>(GameState);
	float BestScore = MIN_flt;
	int32 BestPlayer = -1;
	int32 NumBestPlayers = 0;

	for (int32 i = 0; i < MyGameState->PlayerArray.Num(); i++)
	{
		const float PlayerScore = MyGameState->PlayerArray[i]->GetScore();
		if (BestScore < PlayerScore)
		{
			BestScore = PlayerScore;
			BestPlayer = i;
			NumBestPlayers = 1;
		}
		else if (BestScore == PlayerScore)
		{
			NumBestPlayers++;
		}
	}

	WinnerPlayerState = (NumBestPlayers == 1) ? Cast<AOnlineRPGPlayerState>(MyGameState->PlayerArray[BestPlayer]) : NULL;
}

bool AOnlineRPGGame_FreeForAll::IsWinner(AOnlineRPGPlayerState* PlayerState) const
{
	return PlayerState && !PlayerState->IsQuitter() && PlayerState == WinnerPlayerState;
}
