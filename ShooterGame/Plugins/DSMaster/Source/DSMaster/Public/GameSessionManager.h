// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSessionTypes.h"
// #include "GameSessionManager.generated.h"

class DSMASTER_API FGameSessionManager
{
public:
	FGameSessionDetails* CreateGameSession(const FGameSessionDetails& SessionDetails)
	{
		FGuid Guid = FGuid::NewGuid();
		FString SessionId = Guid.ToString();

		FGameSessionDetails& SessionAdded = GameSessions.Add(SessionId, SessionDetails);
		SessionAdded.SessionId = SessionId;
		return &SessionAdded;
	}

	FGameSessionDetails* GetGameSession(const FString& SessionId)
	{
		if (SessionId.IsEmpty()) return nullptr;
		return GameSessions.Find(SessionId);
	}

	FGameSessionDetails* UpdateGameSession(const FString& SessionId, const FGameSessionDetails& NewSession)
	{
		GameSessions.Remove(SessionId);

		FGameSessionDetails& SessionAdded = GameSessions.Add(SessionId, NewSession);
		SessionAdded.SessionId = SessionId;
		return &SessionAdded;
	}

	bool DestroyGameSession(const FString& SessionId)
	{
		return GameSessions.Remove(SessionId) > 0;
	}

	FGameSessionDetails* FindOnePendingSession(const FString& BucketId);

public:
	TMap<FString, FGameSessionDetails> GameSessions;
};
