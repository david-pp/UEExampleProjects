// Fill out your copyright notice in the Description page of Project Settings.

#include "RedisClient.h"
#include "RedisClientModule.h"

URedisClient* URedisClient::CreateRedisClient(const FString& InIP, int InPort, const FString& InPassword, int PoolSize)
{
	IRedisClientModule* Module = IRedisClientModule::Get();
	if (!Module)
	{
		return nullptr;
	}

	IRedisInterfacePtr Redis = Module->CreateRedisInterface(InIP, InPort, InPassword, PoolSize);
	if (!Redis)
	{
		return nullptr;
	}

	URedisClient* NewClient = NewObject<URedisClient>();
	NewClient->Redis = Redis;
	return NewClient;
}

FRedisReply URedisClient::ExecCommand(const FString& InCommand)
{
	FRedisReply Reply;
	if (Redis)
	{
		Reply = Redis->ExecCommand(InCommand);
	}
	return Reply;
}

TFuture<FRedisReply> URedisClient::AsyncExecCommand(const FString& InCommand)
{
	if (Redis)
	{
		return Redis->AsyncExecCommand(InCommand);
	}

	TPromise<FRedisReply> Promise;
	TFuture<FRedisReply> Future = Promise.GetFuture();
	return MoveTemp(Future);
}
