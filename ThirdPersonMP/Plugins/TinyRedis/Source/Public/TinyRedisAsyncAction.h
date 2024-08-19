// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TinyRedisTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "TinyRedisAsyncAction.generated.h"

/**
 * Redis blueprint async action
 */
UCLASS()
class UTinyRedisAsyncAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UTinyRedisAsyncAction* AsyncRedisCommand(UObject* WorldContextObject, const FString& Command);
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UTinyRedisAsyncAction* RedisCommand(UObject* WorldContextObject, const FString& Command);

	/** Execute the actual operation */
	virtual void Activate() override;

	virtual void OnReply(const FRedisReply& Reply);

	/** called when completes */
	UPROPERTY(BlueprintAssignable)
	FOnRedisReply Completed;

protected:
	// Async Mode ?
	bool bAsyncMode = true;
	// Redis Command
	FString Command;
	// The world context object in which this call is taking place
	UObject* WorldContextObject;
	// Dump reply or not ? 
	bool bDumpReply = true;
};
