// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
// #include "GameStorageEntity.generated.h"

class IGameStorageEngine;

/*
 * Usage 1: Save/Load by game storage engine (more freedom, more code)
 * class FGameUser : public IGameStorage {
 *	public:
 *		virtual bool SaveToStorage(IGameStorageEngine* StorageEngine) override {
 *			// ...
 *		}
 *		virtual bool LoadFromStorage(IGameStorageEngine* StorageEngine) override {
 *			Inventory = StorageEngine->Load(InventoryPath)
 *			UserProfile = StorageEngine->Load(ProfilePath)
 *			Characters = StorageEngine->LoadEntities(UserPath);
 *		}
 *	protected:
 *		FGameInventory Inventory;
 *		FUserProfile UserProfile;
 *		TArray<FGameCharacter> Characters;
 *  }
 *
 * Usage 2: Save/Load by serialize to memory
 * class FGameUser : public IGameStorage {
 *	public:
 *		virtual bool SaveToMemory(IGameStorageEngine* StorageEngine) override {
 *			// ...
 *		}
 *		virtual bool LoadFromMemory(const TArray<uint8>& InData) override {
 *			Archive(InData);
 *			Archive << Inventory;
 *			Archive << UserProfile;
 *			Archive << Characters;
 *		}
 *	protected:
 *		FGameInventory Inventory;
 *		FUserProfile UserProfile;
 *		TArray<FGameCharacter> Characters;
 *  }
 */

class GAMESTORAGE_API IGameStorage
{
public:
	virtual ~IGameStorage() = default;
	// virtual int64 GetStorageId() const = 0;
	// virtual FString GetStorageName() const = 0;
	// virtual FString GetStorageDisplayName() const = 0;

	virtual FString GetStoragePath() const = 0;

	virtual bool SaveToMemory(TArray<uint8>& OutData)
	{
		return false;
	}

	virtual bool LoadFromMemory(const TArray<uint8>& InData)
	{
		return false;
	}

	virtual bool SaveToStorage(IGameStorageEngine* StorageEngine)
	{
		TArray<uint8> OutData;
		if (SaveToMemory(OutData))
		{
			// StorageEngine->SaveBytes(OutData, GetStoragePath())
		}
		return true;
	}

	virtual bool LoadFromStorage(IGameStorageEngine* StorageEngine)
	{
		TArray<uint8> InData;
		// StorageEngine->LoadBytes(InData, GetStoragePath())
		return LoadFromMemory(InData);
	}
};

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
class GAMESTORAGE_API FGameStorageEntity : public IGameStorageEntity
{
public:
	virtual int32 GetEntityId() const override
	{
		return EntityId;
	}

	virtual FString GetEntityName() const override
	{
		return EntityName;
	}

	virtual EGameStorageEntityType GetEntityType() const override
	{
		return EntityType;
	}

	virtual FString GetEntityDisplayName() const override
	{
		return EntityName;
	}

	virtual bool GetEntityAttribute(const FString& AttrName, FString& OutAttrValue) const override
	{
		const FString* AttrPtr = EntityAttributes.Find(AttrName);
		if (AttrPtr)
		{
			OutAttrValue = *AttrPtr;
			return true;
		}
		return false;
	}

	virtual bool SetEntityAttribute(const FString& AttrName, const FString& InAttrValue) override
	{
		EntityAttributes.FindOrAdd(AttrName) = InAttrValue;
		return true;
	}

public:
	int32 EntityId;
	FString EntityName;
	EGameStorageEntityType EntityType;
	TMap<FString, FString> EntityAttributes;
};

// UCLASS()
// class UGameStorageEntity : public USaveGame
// {
// 	GENERATED_BODY()
//
// public:
// };


typedef TSharedPtr<IGameStorageEntity> IGameStorageEntityPtr;

DECLARE_MULTICAST_DELEGATE_TwoParams(FNativeOnStorageEntityLoad, IGameStorageEntityPtr Entity, const FString& ErrorMsg)
typedef FNativeOnStorageEntityLoad::FDelegate FNativeOnStorageEntityLoadDelegate;
DECLARE_MULTICAST_DELEGATE_TwoParams(FNativeOnStorageEntitySave, IGameStorageEntityPtr Entity, const FString& ErrorMsg)
typedef FNativeOnStorageEntitySave::FDelegate FNativeOnStorageEntitySaveDelegate;
