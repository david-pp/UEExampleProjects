// Fill out your copyright notice in the Description page of Project Settings.

#include "RedisAsyncAction.h"
#include "RedisClientModule.h"

URedisAsyncAction* URedisAsyncAction::AsyncRedisCommand(UObject* WorldContextObject, const FString& Command)
{
	URedisAsyncAction* Action = NewObject<URedisAsyncAction>();
	Action->WorldContextObject = WorldContextObject;
	Action->Command = Command;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void URedisAsyncAction::Activate()
{
	IRedisInterfacePtr Redis = IRedisClientModule::GetRedis();
	if (Redis)
	{
		Redis->AsyncExecCommand(Command).Then([this](TFuture<FRedisReply> Future)
		{
			FRedisReply Reply;
			if (Future.IsReady())
			{
				Reply = Future.Get();
			}
			Completed.Broadcast(Reply);
			SetReadyToDestroy();
		});
	}
	else
	{
		// invalid redis interface
		SetReadyToDestroy();
	}
}
