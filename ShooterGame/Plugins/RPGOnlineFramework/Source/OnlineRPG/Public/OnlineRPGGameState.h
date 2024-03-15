// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameFramework/GameState.h"
#include "OnlineRPGGameState.generated.h"

/** ranked PlayerState map, created from the GameState */
typedef TMap<int32, TWeakObjectPtr<AOnlineRPGPlayerState> > RankedPlayerMap; 

UCLASS()
class ONLINERPG_API AOnlineRPGGameState : public AGameState
{
	GENERATED_UCLASS_BODY()

public:

	/** number of teams in current game (doesn't deprecate when no players are left in a team) */
	UPROPERTY(Transient, Replicated)
	int32 NumTeams;

	/** accumulated score per team */
	UPROPERTY(Transient, Replicated)
	TArray<int32> TeamScores;

	/** time left for warmup / match */
	UPROPERTY(Transient, Replicated)
	int32 RemainingTime;

	/** is timer paused? */
	UPROPERTY(Transient, Replicated)
	bool bTimerPaused;

	/** gets ranked PlayerState map for specific team */
	void GetRankedMap(int32 TeamIndex, RankedPlayerMap& OutRankedMap) const;	

	void RequestFinishAndExitToMainMenu();

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

protected:
	UPROPERTY(config)
	FString ActivityId;

	UPROPERTY(config)
	bool bEnableGameFeedback;
};
