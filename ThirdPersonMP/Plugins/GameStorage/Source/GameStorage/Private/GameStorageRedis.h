// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameStorageEngine.h"
#include "GameStoragePath.h"
#include "GameStorageSerializer.h"
#include "TinyRedisInterface.h"

/**
 * Redis Backend
 */
class GAMESTORAGE_API FGameStorageRedis : public IGameStorageEngine
{
public:
	FGameStorageRedis(const FGameStorageEngineSettings& InSettings);
	virtual ~FGameStorageRedis() override;

	bool Startup();
	void Shutdown();

	FString MakeRedisKey(const FGameStorageKey& StorageKey) const;

protected:
	bool SaveObjectToRedis(UObject* Object, const FString& Key);
	bool LoadObjectFromRedis(UObject* Object, const FString& Key);

public:
	// ~ Impl IGameStorageEngine Begin
	virtual FString GetNamespace() const override;

	virtual bool SaveObject(UObject* Object, const FString& Path) override;
	virtual bool LoadObject(UObject* Object, const FString& Path) override;
	virtual bool LoadObjects(TArray<UObject*>& Objects, TSubclassOf<UObject> Class, const FString& PathPattern, UObject* Outer) override;
	virtual bool DeleteObject(const FString& Path) override;

	virtual bool AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave) override;
	virtual bool AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad) override;
	// ~ Impl IGameStorageEngine End

protected:
	FGameStorageEngineSettings Settings;
	IGameStorageSerializerPtr Serializer;

	ITinyRedisPtr TinyRedis;
};
