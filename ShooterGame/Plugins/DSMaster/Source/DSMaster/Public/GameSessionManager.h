// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "GameSessionTypes.h"
// #include "GameSessionManager.generated.h"

class DSMASTER_API FGameSessionManager
{
public:
	FRPGGameSessionDetails* CreateGameSession(const FRPGGameSessionDetails& SessionDetails)
	{
		FGuid Guid = FGuid::NewGuid();
		FString SessionId = Guid.ToString();

		FRPGGameSessionDetails& SessionAdded = GameSessions.Add(SessionId, SessionDetails);
		SessionAdded.SessionId = SessionId;
		return &SessionAdded;
	}

	FRPGGameSessionDetails* GetGameSession(const FString& SessionId)
	{
		if (SessionId.IsEmpty()) return nullptr;
		return GameSessions.Find(SessionId);
	}

	FRPGGameSessionDetails* UpdateGameSession(const FString& SessionId, const FRPGGameSessionDetails& NewSession)
	{
		GameSessions.Remove(SessionId);

		FRPGGameSessionDetails& SessionAdded = GameSessions.Add(SessionId, NewSession);
		SessionAdded.SessionId = SessionId;
		return &SessionAdded;
	}

	bool DestroyGameSession(const FString& SessionId)
	{
		return GameSessions.Remove(SessionId) > 0;
	}

public:
	TMap<FString, FRPGGameSessionDetails> GameSessions;
};
