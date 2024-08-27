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


USTRUCT()
struct FGameEntityStorageKey
{
	GENERATED_BODY()

	FGameEntityStorageKey()
	{
	}

	FGameEntityStorageKey(const FString& InType, const FString& InId) : Type(InType), Id(InId)
	{
	}

	virtual ~FGameEntityStorageKey()
	{
	}

	bool IsValid() const
	{
		return Type.Len() > 0 && Id.Len() > 0;
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("%s:%s"), *Type, *Id);
	}

	virtual FString GetFieldName() const
	{
		return TEXT("");
	}

	UPROPERTY()
	FString Type;
	UPROPERTY()
	FString Id;
};

//
// USTRUCT()
// struct FGameEntityFieldStorageKey : public FGameEntityStorageKey
// {
// 	GENERATED_BODY()
//
// 	FGameEntityFieldStorageKey()
// 	{
// 	}
//
// 	FGameEntityFieldStorageKey(const FString& InType, const FString& InId, const FString& InField) : FGameEntityStorageKey(InType, InId), Field(InField)
// 	{
// 	}
//
// 	FGameEntityFieldStorageKey(const FGameEntityStorageKey& EntityKey, const FString& EntityField) : FGameEntityStorageKey(EntityKey.Type, EntityKey.Id), Field(EntityField)
// 	{
// 	}
//
// 	virtual FString GetFieldName() const override
// 	{
// 		return Field;
// 	}
//
// 	UPROPERTY()
// 	FString Field;
// };
