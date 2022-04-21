// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncPrimeCountAction.generated.h"


// Our delegate to return our value
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPrimeProgressDelegate, FString, StringOut);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPrimeDoneDelegate);

/**
 * 
 */
UCLASS()
class HELLOUE_API UAsyncPrimeCountAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly="true"), Category="Async")
	static UAsyncPrimeCountAction* AsyncPrimeCount(int PrimeCount);
	
	virtual void Activate() override;

	// Max Prime count to calculate
	int32 PrimeCount = 0;

	// output pin 
	UPROPERTY(BlueprintAssignable)
	FPrimeProgressDelegate OnProgress;

	// output pin
	UPROPERTY(BlueprintAssignable)
	FPrimeDoneDelegate OnFinish;
};
