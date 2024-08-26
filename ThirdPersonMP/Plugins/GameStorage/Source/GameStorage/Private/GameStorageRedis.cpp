// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageRedis.h"

#include "TinyRedisModule.h"

FGameStorageRedis::FGameStorageRedis(const FGameStorageEngineSettings& InSettings) : Settings(InSettings)
{
	Startup();
}

FGameStorageRedis::~FGameStorageRedis()
{
	Shutdown();
}

bool FGameStorageRedis::Startup()
{
	ITinyRedisModule* RedisModule = ITinyRedisModule::Get();
	if (RedisModule)
	{
		TinyRedis = RedisModule->CreateRedisInterface(Settings.RedisAddress, Settings.RedisPort, Settings.RedisPassword, 1);
		if (TinyRedis)
		{
			return true;
		}
	}
	return false;
}

void FGameStorageRedis::Shutdown()
{
	TinyRedis.Reset();
}

bool FGameStorageRedis::SaveEntity(UObject* Entity, const FGameStorageEntityKey& EntityKey)
{
	return true;
}

bool FGameStorageRedis::LoadEntity(UObject* Entity, const FGameStorageEntityKey& EntityKey)
{
	return true;
}

bool FGameStorageRedis::LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType)
{
	return true;
}

bool FGameStorageRedis::DeleteEntity(const FGameStorageEntityKey& EntityKey)
{
	return true;
}

bool FGameStorageRedis::AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave)
{
	return true;
}

bool FGameStorageRedis::AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad)
{
	return true;
}
