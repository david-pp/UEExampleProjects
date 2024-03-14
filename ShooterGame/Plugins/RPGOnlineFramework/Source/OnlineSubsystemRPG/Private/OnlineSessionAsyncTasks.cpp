// Fill out your copyright notice in the Description page of Project Settings.


#include "OnlineSessionAsyncTasks.h"

#include "OnlineSessionHelper.h"

bool FOnlineAsyncTaskRequestRPGGameSession::RequestGameSession()
{
	FString QueryParams = FOnlineSessionHelper::SessionSearchToQueryParams(SearchSettings.Get());
	UE_LOG_ONLINE_SESSION(Log, TEXT("RequestGameSession - %s"), *QueryParams);

	Subsystem->DSMasterClient.RequestOneGameSession(QueryParams, [this](FGameSessionSearchResult& Reply, bool bSucceeded)
	{
		if (bSucceeded)
		{
			// Find a Session : Init -> Done
			if (Reply.Sessions.Num() > 0)
			{
				SearchResult = Reply;
				SearchSettings->SearchState = EOnlineAsyncTaskState::Done;
				bWasSuccessful = true;
				RequestTaskState = ERequestGameSessionTaskState::Finished;
			}
			// DS Master is Launching a Server : Init -> TryFindSession
			else
			{
				TryFindSessionTime = FDateTime::UtcNow();
				RequestTaskState = ERequestGameSessionTaskState::TryFindSession;
			}
		}
	});

	SearchSettings->SearchState = EOnlineAsyncTaskState::InProgress;
	return true;
}

bool FOnlineAsyncTaskRequestRPGGameSession::TryFindGameSession()
{
	FString QueryParams = FOnlineSessionHelper::SessionSearchToQueryParams(SearchSettings.Get());
	UE_LOG_ONLINE_SESSION(Log, TEXT("TryFindGameSession - %s, Count=%d"), *QueryParams, TryFindSessionCount);

	Subsystem->DSMasterClient.RequestGameSessionFind(QueryParams, [this](FGameSessionSearchResult& Reply, bool bSucceeded)
	{
		if (bSucceeded)
		{
			// Find a Session : Init -> Done
			if (Reply.Sessions.Num() > 0)
			{
				SearchResult = Reply;
				SearchSettings->SearchState = EOnlineAsyncTaskState::Done;
				bWasSuccessful = true;
				RequestTaskState = ERequestGameSessionTaskState::Finished;
			}
			// Session is Not Ready : State -> TryFindSession
			else
			{
				TryFindSessionCount ++;
				TryFindSessionTime = FDateTime::UtcNow();
				RequestTaskState = ERequestGameSessionTaskState::TryFindSession;
			}
		}
	});

	return true;
}

void FOnlineAsyncTaskRequestRPGGameSession::Tick()
{
	switch (RequestTaskState)
	{
	// case ERequestGameSessionTaskState::Init:
	// 	{
	// 		if (RequestGameSession())
	// 		{
	// 			RequestTaskState = ERequestGameSessionTaskState::WaitForSessionRequest;
	// 		}
	// 		else
	// 		{
	// 			UE_LOG_ONLINE_SESSION(Warning, TEXT("RequestRPGGameSession - Init Failed"));
	// 			bWasSuccessful = false;
	// 			RequestTaskState = ERequestGameSessionTaskState::Finished;
	// 		}
	// 		break;
	// 	}
	case ERequestGameSessionTaskState::WaitForSessionRequest:
		{
			// check task timeout
			if (GetElapsedTime() > TimeoutSeconds)
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("RequestRPGGameSession - Task Timeout"));
				bWasSuccessful = false;
				RequestTaskState = ERequestGameSessionTaskState::Finished;
			}
			break;
		}
	case ERequestGameSessionTaskState::TryFindSession:
		{
			if (TryFindSessionCount > MaxTryFindSessionCount)
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("RequestRPGGameSession - Exceed limit Count"));
				bWasSuccessful = false;
				RequestTaskState = ERequestGameSessionTaskState::Finished;
				break;
			}

			if (NeedTryFindGameSessionAgain())
			{
				TryFindGameSession();
				RequestTaskState = ERequestGameSessionTaskState::WaitForFindSession;
			}
			break;
		}
	case ERequestGameSessionTaskState::WaitForFindSession:
		{
			// check task timeout
			if (GetElapsedTime() > TimeoutSeconds)
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("RequestRPGGameSession - Task Timeout"));
				bWasSuccessful = false;
				RequestTaskState = ERequestGameSessionTaskState::Finished;
			}
			break;
		}
	case ERequestGameSessionTaskState::Finished:
		{
			bIsComplete = true;
			break;
		}
	default:
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("RequestRPGGameSession - Unexpected state ending task."));
			bWasSuccessful = false;
			RequestTaskState = ERequestGameSessionTaskState::Finished;
			break;
		}
	}
}

void FOnlineAsyncTaskRequestRPGGameSession::Initialize()
{
	if (RequestGameSession())
	{
		RequestTaskState = ERequestGameSessionTaskState::WaitForSessionRequest;
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("RequestRPGGameSession - Init Failed"));
		bWasSuccessful = false;
		RequestTaskState = ERequestGameSessionTaskState::Finished;
	}
}

void FOnlineAsyncTaskRequestRPGGameSession::Finalize()
{
	// Update SearchSettings
	if (SearchSettings)
	{
		for (auto& SessionDetail : SearchResult.Sessions)
		{
			FOnlineSessionSearchResult& SearchResultAdded = SearchSettings->SearchResults.AddZeroed_GetRef();
			FOnlineSessionHelper::SetupOnlineSession(&SearchResultAdded.Session, SessionDetail);
		}
	}
}

void FOnlineAsyncTaskRequestRPGGameSession::TriggerDelegates()
{
	IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
	if (SessionInt.IsValid())
	{
		SessionInt->TriggerOnFindSessionsCompleteDelegates(bWasSuccessful);
	}
}
