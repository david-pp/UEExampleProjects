// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
// #include "GameStorageEntity.generated.h"

// UENUM()
enum class EGameStorageEntityType : uint8
{
	User,
	Character,
	Group,
};


class GAMESTORAGE_API IGameStorageEntity
{
public:
	virtual ~IGameStorageEntity() = default;
	virtual int32 GetEntityId() const = 0;
	virtual FString GetEntityName() const = 0;
	virtual EGameStorageEntityType GetEntityType() const = 0;

	virtual FString GetEntityDisplayName() const = 0;

	virtual bool GetEntityAttribute(const FString& AttrName, FString& OutAttrValue) const = 0;
	virtual bool SetEntityAttribute(const FString& AttrName, const FString& InAttrValue) = 0;
};


/**
 * 
 */
// class GAMESTORAGE_API FGameStorageEntity : public IGameStorageEntity
// {
// public:
// 	virtual int32 GetEntityId() const override
// 	{
// 		return EntityId;
// 	}
//
// 	virtual FString GetEntityName() const override
// 	{
// 		return EntityName;
// 	}
//
// 	virtual EGameStorageEntityType GetEntityType() const override
// 	{
// 		return EntityType;
// 	}
//
// 	virtual FString GetEntityDisplayName() const override
// 	{
// 		return EntityName;
// 	}
//
// 	virtual bool GetEntityAttribute(const FString& AttrName, FString& OutAttrValue) const override
// 	{
// 		const FString* AttrPtr = EntityAttributes.Find(AttrName);
// 		if (AttrPtr)
// 		{
// 			OutAttrValue = *AttrPtr;
// 			return true;
// 		}
// 		return false;
// 	}
//
// 	virtual bool SetEntityAttribute(const FString& AttrName, const FString& InAttrValue) override
// 	{
// 		EntityAttributes.FindOrAdd(AttrName) = InAttrValue;
// 		return true;
// 	}
//
// public:
// 	int32 EntityId;
// 	FString EntityName;
// 	EGameStorageEntityType EntityType;
// 	TMap<FString, FString> EntityAttributes;
// };


typedef TSharedPtr<IGameStorageEntity> IGameStorageEntityPtr;
