﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameStorageEngine.h"
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

public:
	virtual FString GetNamespace() const override
	{
		return Settings.Namespace;
	}

	virtual bool SaveEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey) override;
	virtual bool LoadEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey) override;
	virtual bool LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType, UObject* Outer) override;
	virtual bool DeleteEntity(const FGameEntityStorageKey& EntityKey) override;

	virtual bool AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave) override;
	virtual bool AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad) override;

protected:
	FGameStorageEngineSettings Settings;

	ITinyRedisPtr TinyRedis;
};
