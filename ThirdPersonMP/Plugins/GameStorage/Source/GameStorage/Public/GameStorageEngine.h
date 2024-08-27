// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameStorageTypes.h"
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

	virtual FString GetNamespace() const = 0;

public:
	// Sync Entity API
	virtual bool SaveEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey) = 0;
	virtual bool LoadEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey) = 0;
	virtual bool LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType, UObject* Outer = GetTransientPackage()) = 0;
	virtual bool DeleteEntity(const FGameEntityStorageKey& EntityKey) = 0;

	UObject* LoadAndCreateEntity(const FGameEntityStorageKey& EntityKey, TSubclassOf<UObject> EntityClass, UObject* Outer = GetTransientPackage())
	{
		UObject* Entity = NewObject<UObject>(Outer, EntityClass);
		if (Entity && LoadEntity(Entity, EntityKey))
		{
			return Entity;
		}
		return nullptr;
	}

	template <typename EntityClass>
	EntityClass* LoadAndCreateEntity(const FGameEntityStorageKey& EntityKey, UObject* Outer = GetTransientPackage())
	{
		return Cast<EntityClass>(LoadAndCreateEntity(EntityKey, EntityClass::StaticClass(), Outer));
	}

	template <typename EntityClass>
	bool LoadEntities(TArray<EntityClass*>& Entities, const FString& EntityType)
	{
		TArray<UObject*> Objects;
		bool Result = LoadEntities(Objects, EntityClass::StaticClass(), EntityType);
		for (auto Object : Objects)
		{
			EntityClass* Entity = Cast<EntityClass>(Object);
			if (Entity)
			{
				Entities.Add(Entity);
			}
		}
		return Result;
	}


	virtual bool AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave = FNativeOnStorageEntitySaveDelegate()) = 0;
	virtual bool AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad = FNativeOnStorageEntityLoadDelegate()) = 0;
};
