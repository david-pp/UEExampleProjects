// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameStorageEngine.h"
#include "GameStorageSerializer.h"
#include "TinyRedisInterface.h"

/**
 * 
 */
class GAMESTORAGE_API FGameStorageRedis : public IGameStorageEngine
{
public:
	FGameStorageRedis(const FGameStorageEngineSettings& InSettings);
	virtual ~FGameStorageRedis() override;

	bool Startup();
	void Shutdown();

	FString MakeRedisEntityKey(const FGameEntityStorageKey& EntityKey) const;

protected:
	bool SaveEntityToRedis(UObject* Entity, const FString& Key);
	bool LoadEntityFromRedis(UObject* Entity, const FString& Key);

public:
	virtual FString GetNamespace() const override
	{
		return Settings.Namespace;
	}

	virtual bool SaveEntity(UObject* Entity, const FString& Path) override;
	virtual bool LoadEntity(UObject* Entity, const FString& Path) override;
	virtual bool LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& PathPattern, UObject* Outer) override;
	virtual bool DeleteEntity(const FString& Path) override;

	virtual bool AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave) override;
	virtual bool AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad) override;

protected:
	FGameStorageEngineSettings Settings;
	IGameStorageSerializerPtr Serializer;

	ITinyRedisPtr TinyRedis;
};
