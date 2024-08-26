// Fill out your copyright notice in the Description page of Project Settings.

#include "TinyRedisAsyncAction.h"
#include "TinyRedisModule.h"

UTinyRedisAsyncAction* UTinyRedisAsyncAction::AsyncRedisCommand(UObject* WorldContextObject, const FString& Command)
{
	UTinyRedisAsyncAction* Action = NewObject<UTinyRedisAsyncAction>();
	Action->bAsyncMode = true;
	Action->WorldContextObject = WorldContextObject;
	Action->Command = Command;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

UTinyRedisAsyncAction* UTinyRedisAsyncAction::RedisCommand(UObject* WorldContextObject, const FString& Command)
{
	UTinyRedisAsyncAction* Action = NewObject<UTinyRedisAsyncAction>();
	Action->bAsyncMode = false;
	Action->WorldContextObject = WorldContextObject;
	Action->Command = Command;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UTinyRedisAsyncAction::Activate()
{
	ITinyRedisPtr Redis = ITinyRedisModule::GetTinyRedis();
	if (Redis)
	{
		if (bAsyncMode)
		{
			Redis->AsyncExecCommand(Command).Then([this](TFuture<FRedisReply> Future)
			{
				FRedisReply Reply;
				if (Future.IsReady())
				{
					Reply = Future.Get();
				}

				OnReply(Reply);
			});
		}
		else
		{
			FRedisReply Reply = Redis->ExecCommand(Command);
			OnReply(Reply);
		}
	}
	else
	{
		FRedisReply Reply;
		Reply.Error = TEXT("invalid redis interface");
		OnReply(Reply);
	}
}

void UTinyRedisAsyncAction::OnReply(const FRedisReply& Reply)
{
	if (bDumpReply)
	{
		UE_LOG(LogRedis, Warning, TEXT("[%s] %s -> %s"), bAsyncMode ? TEXT("Async") : TEXT("Sync"), *Command, *Reply.ToDebugString());
	}

	Completed.Broadcast(Reply);
	SetReadyToDestroy();
}
