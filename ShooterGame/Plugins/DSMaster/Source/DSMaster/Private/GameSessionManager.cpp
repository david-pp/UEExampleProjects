// Fill out your copyright notice in the Description page of Project Settings.


#include "GameSessionManager.h"

FGameSessionDetails* FGameSessionManager::FindOnePendingSession(const FString& BucketId)
{
	for (auto& Pair : GameSessions)
	{
		if (Pair.Value.Settings.BucketId == BucketId && Pair.Value.GetSessionState() == EOnlineSessionState::Pending)
		{
			return &Pair.Value;
		}
	}
	return nullptr;
}
