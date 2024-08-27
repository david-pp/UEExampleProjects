// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageRedis.h"

#include "GameStorage.h"
#include "GameStorageSerialization.h"
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

FString FGameStorageRedis::MakeRedisEntityKey(const FGameEntityStorageKey& EntityKey) const
{
	FString Namespace = GetNamespace();
	if (Namespace.IsEmpty())
	{
		return FString::Printf(TEXT("%s:%s"), *EntityKey.Type, *EntityKey.Id);
	}
	else
	{
		return FString::Printf(TEXT("%s:%s:%s"), *Namespace, *EntityKey.Type, *EntityKey.Id);
	}
}

bool FGameStorageRedis::SaveEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey)
{
	if (!TinyRedis) return false;
	if (!EntityKey.IsValid())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntity - invalid key:%s"), *EntityKey.ToString());
		return false;
	}

	FString Key = MakeRedisEntityKey(EntityKey);
	FBufferArchive ValueBuffer;
	FGameStorageSerialization::SaveObjectToJson(Entity, ValueBuffer);

	FRedisReply Reply = TinyRedis->SetBin(Key, ValueBuffer);
	return Reply.IsStatusOK();
}

bool FGameStorageRedis::LoadEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey)
{
	if (!TinyRedis) return false;
	if (!EntityKey.IsValid())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - invalid key:%s"), *EntityKey.ToString());
		return false;
	}

	FString Key = MakeRedisEntityKey(EntityKey);
	FRedisReply Reply = TinyRedis->GetBin(Key);
	if (Reply.HasError())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - redis error:%s"), *Reply.ToDebugString());
		return false;
	}

	if (Reply.IsNil())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - not exist: %s"), *Key);
		return false;
	}

	FGameStorageSerialization::LoadObjectFromJson(Entity, Reply.BinArray);
	return true;
}

bool FGameStorageRedis::LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType, UObject* Outer)
{
	if (!TinyRedis) return false;

	FString KeyPattern = MakeRedisEntityKey(FGameEntityStorageKey(EntityType, TEXT("*")));

	TArray<FString> Keys = TinyRedis->GetKeys(KeyPattern);
	for (auto& Key : Keys)
	{
		FRedisReply Reply = TinyRedis->GetBin(Key);
		if (Reply.IsValid())
		{
			UObject* Entity = NewObject<UObject>(Outer, EntityClass);
			if (Entity)
			{
				FGameStorageSerialization::LoadObjectFromJson(Entity, Reply.BinArray);
				Entities.Add(Entity);
			}
		}
	}

	return true;
}

bool FGameStorageRedis::DeleteEntity(const FGameEntityStorageKey& EntityKey)
{
	if (!TinyRedis) return false;

	FString Key = MakeRedisEntityKey(EntityKey);
	TinyRedis->DeleteKey(Key);
	return TinyRedis->DeleteKey(Key) > 0;
}


bool FGameStorageRedis::AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave)
{
	return true;
}

bool FGameStorageRedis::AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad)
{
	return true;
}
