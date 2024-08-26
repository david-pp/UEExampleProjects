// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameStorageEntity.h"
#include "GameFramework/SaveGame.h"
#include "GameStorageEngine.generated.h"

class IGameStorageEngine;
class IGameStorageEntity;

typedef TSharedPtr<IGameStorageEngine> IGameStorageEnginePtr;

DECLARE_MULTICAST_DELEGATE_TwoParams(FNativeOnStorageEntityLoad, IGameStorageEntityPtr Entity, const FString& ErrorMsg)
typedef FNativeOnStorageEntityLoad::FDelegate FNativeOnStorageEntityLoadDelegate;
DECLARE_MULTICAST_DELEGATE_TwoParams(FNativeOnStorageEntitySave, IGameStorageEntityPtr Entity, const FString& ErrorMsg)
typedef FNativeOnStorageEntitySave::FDelegate FNativeOnStorageEntitySaveDelegate;


USTRUCT(BlueprintType)
struct FGameStorageEngineSettings
{
	GENERATED_BODY()

	UPROPERTY()
	FString RedisAddress;
	UPROPERTY()
	int RedisPort;
	UPROPERTY()
	FString RedisPassword;
};


USTRUCT()
struct FGameStorageEntityKey
{
	GENERATED_BODY()

	FGameStorageEntityKey()
	{
	}

	FGameStorageEntityKey(const FString& InType, const FString& InId) : Type(InType), Id(InId)
	{
	}

	FString ToString() const
	{
		return FString::Printf(TEXT("%s:%s"), *Type, *Id);
	}

	UPROPERTY()
	FString Type;
	UPROPERTY()
	FString Id;
};


UCLASS()
class UGameStorageEntity : public USaveGame
{
	GENERATED_BODY()

public:
};


// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UGameStorageEngine : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class GAMESTORAGE_API IGameStorageEngine
{
	GENERATED_BODY()

public:
	static IGameStorageEnginePtr GetDefault();

public:
	virtual bool AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave = FNativeOnStorageEntitySaveDelegate()) = 0;
	virtual bool AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad = FNativeOnStorageEntityLoadDelegate()) = 0;

	virtual bool SaveEntity(UObject* Entity, const FGameStorageEntityKey& EntityKey) = 0;
	virtual bool LoadEntity(UObject* Entity, const FGameStorageEntityKey& EntityKey) = 0;
	virtual bool LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType) = 0;
	virtual bool DeleteEntity(const FGameStorageEntityKey& EntityKey) = 0;

	virtual bool SaveEntityObject(UObject* EntityObject, const FGameStorageEntityKey& EntityKey, const FString& EntityObjName) { return false; }
	virtual bool LoadEntityObject(UObject* EntityObject, const FGameStorageEntityKey& EntityKey, const FString& EntityObjName) { return false; }
	virtual bool LoadEntityObjects(TArray<UObject*>& EntityObjects, TSubclassOf<UObject> EntityObjectClass, const FGameStorageEntityKey& EntityKey, const FString& EntityObjName) { return false; }
	virtual bool DeleteEntityObject(const FGameStorageEntityKey& EntityKey, const FString& EntityObjName) { return false; }


	template <typename EntityObjectType>
	bool LoadEntityObjects(TArray<EntityObjectType*>& EntityObjects, const FGameStorageEntityKey& EntityKey, const FString& EntityObjName)
	{
		TArray<UObject*> Objects;
		bool Result = LoadEntityObjects(Objects, EntityObjectType::StaticClass(), EntityKey, EntityObjName);
		for (auto Object : Objects)
		{
			EntityObjectType* ResultObject = Cast<EntityObjectType>(Object);
			if (ResultObject)
			{
				EntityObjects.Add(ResultObject);
			}
		}
		return Result;
	}
};
