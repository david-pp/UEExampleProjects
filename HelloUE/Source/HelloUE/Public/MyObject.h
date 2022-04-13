// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "MyStruct.h"
#include "MyObject.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
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
	
	UFUNCTION(BlueprintCallable, meta=(DisplayName = "Hello"))
	void K2_HelloWorld();

	UFUNCTION(BlueprintNativeEvent)
	void HelloWorld();
	virtual void HelloWorld_Implementation();
};

UCLASS()
class UMyDerivedObject : public UMyObject
{
	GENERATED_BODY()
	
public:
	virtual void HelloWorld_Implementation() override;
};



UCLASS()
class UMyDerivedObject2 : public UMyObject
{
	GENERATED_BODY()
	
public:
	virtual void HelloWorld_Implementation() override;
};
