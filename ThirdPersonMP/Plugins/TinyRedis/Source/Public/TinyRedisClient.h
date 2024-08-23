// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TinyRedisInterface.h"
#include "UObject/Object.h"
#include "TinyRedisClient.generated.h"

/**
 * Redis Client (Wrapper for Redis Interface)
 */
UCLASS()
class UTinyRedisClient : public UObject, public ITinyRedisInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = TinyRedis)
	static UTinyRedisClient* CreateRedisClient(const FString& InIP = FString(TEXT("127.0.0.1")), int InPort = 6379, const FString& InPassword = FString(TEXT("")), int PoolSize = 1);

	UFUNCTION(BlueprintCallable, Category = TinyRedis)
	static void DumpRedisReply(const FRedisReply& Reply);

public:
	// ~Interface ITinyRedisInterface
	virtual FRedisReply ExecCommand(ITinyRedisCommandPtr Command) override;
	virtual TFuture<FRedisReply> AsyncExecCommand(ITinyRedisCommandPtr Command) override;
	// ~Interface ITinyRedisInterface

protected:
	IRedisInterfacePtr Redis;
};
