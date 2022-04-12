// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MyStruct.h"
#include "MyObject.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class HELLOUE_API UMyObject : public UObject
{
	GENERATED_BODY()
	
	
public:
	UMyObject();

	virtual ~UMyObject() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Hello)
	int32 Value;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Hello)
	FMyStruct MyStruct;
};
