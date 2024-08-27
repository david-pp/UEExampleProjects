// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameStorageEngine.h"
#include "GameStorageSerializer.h"

/**
 * 
 */
class GAMESTORAGE_API FGameStorageFile : public IGameStorageEngine
{
public:
	FGameStorageFile(const FGameStorageEngineSettings& InSettings);
	virtual ~FGameStorageFile() override;

	FString GetRootDirPath() const;
	FString MakeEntityFilePath(const FGameEntityStorageKey& EntityKey) const;

protected:
	bool SaveEntityToFile(UObject* Entity, const FString& FilePath);
	bool LoadEntityFromFile(UObject* Entity, const FString& FilePath);

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
	IGameStorageSerializerPtr Serializer;
};
