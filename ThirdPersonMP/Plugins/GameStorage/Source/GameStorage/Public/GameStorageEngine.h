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

	/** Game Storage Namespace: default is empty */
	UPROPERTY()
	FString Namespace;

	UPROPERTY()
	bool bEnableRedisBackend = false;
	UPROPERTY()
	FString RedisAddress;
	UPROPERTY()
	int RedisPort;
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

USTRUCT()
struct FGameEntityFieldStorageKey : public FGameEntityStorageKey
{
	GENERATED_BODY()

	FGameEntityFieldStorageKey()
	{
	}

	FGameEntityFieldStorageKey(const FString& InType, const FString& InId, const FString& InField) : FGameEntityStorageKey(InType, InId), Field(InField)
	{
	}

	FGameEntityFieldStorageKey(const FGameEntityStorageKey& EntityKey, const FString& EntityField) : FGameEntityStorageKey(EntityKey.Type, EntityKey.Id), Field(EntityField)
	{
	}

	virtual FString GetFieldName() const override
	{
		return Field;
	}

	UPROPERTY()
	FString Field;
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
