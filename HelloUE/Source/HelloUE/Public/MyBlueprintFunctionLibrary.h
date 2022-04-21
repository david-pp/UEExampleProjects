// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"

// Our delegate to return our value
DECLARE_DYNAMIC_DELEGATE_OneParam(FTestDelegate, FString, StringOut);
DECLARE_DYNAMIC_DELEGATE(FTestDoneDelegate);

/**
 * 
 */
UCLASS()
class HELLOUE_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()


	/** Executes every 1000th Prime count */
	UFUNCTION(BlueprintCallable, Category = "AsyncTest", meta=(DisplayName="AsyncTest", Keywords = "async"))
	static void AsyncPrimeCount(FTestDelegate Out, FTestDoneDelegate Done, int32 PrimeCount);
};
