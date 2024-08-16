// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RedisTypes.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "RedisAsyncAction.generated.h"

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
