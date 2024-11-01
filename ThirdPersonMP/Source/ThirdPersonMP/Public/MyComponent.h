﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MyComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class THIRDPERSONMP_API UMyComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UMyComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	UPROPERTY(Replicated)
	FString MyName;

	UPROPERTY(ReplicatedUsing=OnRep_StringValue, BlueprintReadWrite, Category=David)
	FString StringValue;

	UFUNCTION()
	void OnRep_StringValue(FString& OldValue);


	UPROPERTY(ReplicatedUsing=OnRep_StringArray, BlueprintReadWrite, Category=David)
	TArray<FString> StringArray;

	UFUNCTION()
	void OnRep_StringArray(TArray<FString>& OldValues);
	
};
