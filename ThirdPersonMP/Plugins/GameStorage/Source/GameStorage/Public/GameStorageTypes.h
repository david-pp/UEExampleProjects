// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameStorageTypes.generated.h"

/**
 * 
 */
UENUM()
enum class EGameStorageSerializerType : uint8
{
	/** do nothing */
	None,
	/** as Sav */
	Sav,
	/** as Json */
	Json,
};

USTRUCT(BlueprintType)
struct FGameStorageEngineSettings
{
	GENERATED_BODY()

	/** Game Storage Namespace: default is empty */
	UPROPERTY()
	FString Namespace;
	/** Entity Serialization Type */
	UPROPERTY()
	EGameStorageSerializerType SerializerType = EGameStorageSerializerType::None;

	UPROPERTY()
	bool bEnableRedisBackend = false;
	UPROPERTY()
	FString RedisAddress;
	UPROPERTY()
	int RedisPort = 0;
	UPROPERTY()
	FString RedisPassword;
	UPROPERTY()
	bool bSaveEntityAsHash = false;
};
