// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"
#include "OnlineSessionRPG.h"
#include "OnlineSessionSettings.h"
// #include "OnlineSessionAsyncTasks.generated.h"

class ONLINESUBSYSTEMRPG_API FOnlineAsyncTaskRequestRPGGameSession : public FOnlineAsyncTaskBasic<FOnlineSubsystemRPG>
{
public:
	FOnlineAsyncTaskRequestRPGGameSession(FOnlineSubsystemRPG* InSubsystem, TSharedPtr<class FOnlineSessionSearch> InSearchSettings) : FOnlineAsyncTaskBasic(InSubsystem), SearchSettings(InSearchSettings), RequestTaskState(ERequestGameSessionTaskState::Init)
	{
	}

	virtual ~FOnlineAsyncTaskRequestRPGGameSession()
	{
	}

	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("RequestRPGGameSession bWasSuccessful: %d"), WasSuccessful());
	}

	// AsyncTask Thread
	virtual void Tick() override;
	// GameThread
	virtual void Initialize() override;
	// GameThread
	virtual void Finalize() override;
	// GameThread
	virtual void TriggerDelegates() override;

protected:
	bool RequestGameSession();

	bool TryFindGameSession();

	bool NeedTryFindGameSessionAgain() const
	{
		FTimespan Elapsed = FDateTime::UtcNow() - TryFindSessionTime;
		return Elapsed > FTimespan::FromSeconds(TryFindSessionDelaySeconds);
	}

private:
	/** Search settings specified for the query */
	TSharedPtr<class FOnlineSessionSearch> SearchSettings;

	enum class ERequestGameSessionTaskState : uint8
	{
		Init,
		WaitForSessionRequest,
		TryFindSession,
		WaitForFindSession,
		Finished
	};

	ERequestGameSessionTaskState RequestTaskState;

	// Task timeout settings
	float TimeoutSeconds = 120;

	FDateTime TryFindSessionTime;
	float TryFindSessionDelaySeconds = 0.1;
	int32 TryFindSessionCount = 0;
	int32 MaxTryFindSessionCount = 100;

	FGameSessionSearchResult SearchResult;
};
