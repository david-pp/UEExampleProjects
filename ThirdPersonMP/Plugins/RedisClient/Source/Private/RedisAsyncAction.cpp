// Fill out your copyright notice in the Description page of Project Settings.

#include "RedisAsyncAction.h"
#include "RedisClientModule.h"

URedisClientDemo* URedisClientDemo::CreateRedisClient(int MaxNum, const FString& InIP, int InPort, const FString& InPassword)
{
	URedisClientDemo* NewClient = NewObject<URedisClientDemo>();
	return NewClient;
}

FRedisReply URedisClientDemo::ExecCommand(const FString& InCommand)
{
	FRedisReply Reply;
	IRedisInterfacePtr Redis = IRedisClientModule::GetRedis();
	if (Redis)
	{
		Reply = Redis->ExecCommand(InCommand);
	}
	return Reply;
}

TFuture<FRedisReply> URedisClientDemo::AsyncExecCommand(const FString& InCommand)
{
	IRedisInterfacePtr Redis = IRedisClientModule::GetRedis();
	if (Redis)
	{
		return Redis->AsyncExecCommand(InCommand);
	}

	TPromise<FRedisReply> Promise;
	TFuture<FRedisReply> Future = Promise.GetFuture();
	return MoveTemp(Future);
}

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
