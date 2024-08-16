// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RedisInterface.h"
#include "RedisTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "RedisAsyncAction.generated.h"

UCLASS()
class URedisClientDemo : public UObject, public IRedisInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static URedisClientDemo* CreateRedisClient(int MaxNum = 3, const FString& InIP = FString(TEXT("127.0.0.1")), int InPort = 6379, const FString& InPassword = FString(TEXT("")));

	virtual FRedisReply ExecCommand(const FString& InCommand) override;
	virtual TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand) override;
};

/**
 * Redis blueprint async action
 */
UCLASS()
class URedisAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static URedisAsyncAction* AsyncRedisCommand(UObject* WorldContextObject, const FString& Command);

	/** Execute the actual operation */
	virtual void Activate() override;

	/** called when completes */
	UPROPERTY(BlueprintAssignable)
	FOnRedisReply Completed;

protected:
	// Redis Command
	FString Command;
	// The world context object in which this call is taking place
	UObject* WorldContextObject;
};
