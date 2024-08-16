// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RedisInterface.h"
#include "UObject/Object.h"
#include "RedisClient.generated.h"

/**
 * Redis Client (Wrapper for Redis Interface)
 */
UCLASS()
class URedisClient : public UObject, public IRedisInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static URedisClient* CreateRedisClient(const FString& InIP = FString(TEXT("127.0.0.1")), int InPort = 6379, const FString& InPassword = FString(TEXT("")), int PoolSize = 1);

public:
	virtual FRedisReply ExecCommand(const FString& InCommand) override;
	virtual TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand) override;

protected:
	IRedisInterfacePtr Redis;
};
