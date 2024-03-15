// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGGameState.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineGameMatchesInterface.h"
#include "OnlineRPGGameMode.h"
#include "OnlineRPGPlayerState.h"
#include "UnrealNetwork.h"

class AOnlineRPGPlayerState;

AOnlineRPGGameState::AOnlineRPGGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NumTeams = 0;
	RemainingTime = 0;
	bTimerPaused = false;
}

void AOnlineRPGGameState::GetLifetimeReplicatedProps( TArray< FLifetimeProperty > & OutLifetimeProps ) const
{
	Super::GetLifetimeReplicatedProps( OutLifetimeProps );

	DOREPLIFETIME( AOnlineRPGGameState, NumTeams );
	DOREPLIFETIME( AOnlineRPGGameState, RemainingTime );
	DOREPLIFETIME( AOnlineRPGGameState, bTimerPaused );
	DOREPLIFETIME( AOnlineRPGGameState, TeamScores );
}

void AOnlineRPGGameState::GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const
{
	OutRankedMap.Empty();

	//first, we need to go over all the PlayerStates, grab their score, and rank them
	TMultiMap<int32, AOnlineRPGPlayerState*> SortedMap;
	for(int32 i = 0; i < PlayerArray.Num(); ++i)
	{
		int32 Score = 0;
		AOnlineRPGPlayerState* CurPlayerState = Cast<AOnlineRPGPlayerState>(PlayerArray[i]);
		if (CurPlayerState && (CurPlayerState->GetTeamNum() == TeamIndex))
		{
			SortedMap.Add(FMath::TruncToInt(CurPlayerState->GetScore()), CurPlayerState);
		}
	}

	//sort by the keys
	SortedMap.KeySort(TGreater<int32>());

	//now, add them back to the ranked map
	OutRankedMap.Empty();

	int32 Rank = 0;
	for(TMultiMap<int32, AOnlineRPGPlayerState*>::TIterator It(SortedMap); It; ++It)
	{
		OutRankedMap.Add(Rank++, It.Value());
	}
	
}

void AOnlineRPGGameState::RequestFinishAndExitToMainMenu()
{
	if (AuthorityGameMode)
	{
		// we are server, tell the gamemode
		AOnlineRPGGameMode* const GameMode = Cast<AOnlineRPGGameMode>(AuthorityGameMode);
		if (GameMode)
		{
			GameMode->RequestFinishAndExitToMainMenu();
		}
	}
	else
	{
		// // we are client, handle our own business
		// UOnlineRPGGameInstance* GameInstance = Cast<UOnlineRPGGameInstance>(GetGameInstance());
		// if (GameInstance)
		// {
		// 	GameInstance->RemoveSplitScreenPlayers();
		// }
		//
		// AOnlineRPGPlayerController* const PrimaryPC = Cast<AOnlineRPGPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
		// if (PrimaryPC)
		// {
		// 	PrimaryPC->HandleReturnToMainMenu();
		// }
	}
}

void AOnlineRPGGameState::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void AOnlineRPGGameState::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}