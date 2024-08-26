// Fill out your copyright notice in the Description page of Project Settings.

#include "TinyRedisClient.h"
#include "TinyRedisModule.h"

UTinyRedisClient* UTinyRedisClient::CreateRedisClient(const FString& InIP, int InPort, const FString& InPassword, int PoolSize)
{
	ITinyRedisModule* Module = ITinyRedisModule::Get();
	if (!Module)
	{
		return nullptr;
	}

	ITinyRedisPtr Redis = Module->CreateRedisInterface(InIP, InPort, InPassword, PoolSize);
	if (!Redis)
	{
		return nullptr;
	}

	UTinyRedisClient* NewClient = NewObject<UTinyRedisClient>();
	NewClient->Redis = Redis;
	return NewClient;
}

void UTinyRedisClient::DumpRedisReply(const FRedisReply& Reply)
{
	UE_LOG(LogRedis, Warning, TEXT("%s"), *Reply.ToDebugString());
}

FRedisReply UTinyRedisClient::ExecCommand(ITinyRedisCommandPtr Command)
{
	return Redis->ExecCommand(Command);
}

TFuture<FRedisReply> UTinyRedisClient::AsyncExecCommand(ITinyRedisCommandPtr Command)
{
	return Redis->AsyncExecCommand(Command);
}
