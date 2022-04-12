// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MyStruct.generated.h"

class UMyObject;

/**
 * 
 */
USTRUCT(BlueprintType)
struct FMyStruct
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=My)
	float FloatValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=My)
	float IntValue;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=My)
	UMyObject* MyObject;
};
