// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageRedis.h"

#include "GameStorage.h"
#include "GameStorageSerializer.h"
#include "JsonObjectConverter.h"
#include "TinyRedisModule.h"

FGameStorageRedis::FGameStorageRedis(const FGameStorageEngineSettings& InSettings) : Settings(InSettings)
{
	Serializer = IGameStorageSerializer::Create(Settings.SerializerType);
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
	FString FieldName = EntityKey.GetFieldName();
	if (Namespace.IsEmpty())
	{
		if (FieldName.IsEmpty())
		{
			return FString::Printf(TEXT("%s:%s"), *EntityKey.Type, *EntityKey.Id);
		}
		else
		{
			return FString::Printf(TEXT("%s:%s:%s"), *EntityKey.Type, *EntityKey.Id, *FieldName);
		}
	}
	else
	{
		if (FieldName.IsEmpty())
		{
			return FString::Printf(TEXT("%s:%s:%s"), *Namespace, *EntityKey.Type, *EntityKey.Id);
		}
		else
		{
			return FString::Printf(TEXT("%s:%s:%s:%s"), *Namespace, *EntityKey.Type, *EntityKey.Id, *FieldName);
		}
	}
}

bool FGameStorageRedis::SaveEntityToRedis(UObject* Entity, const FString& Key)
{
	if (!Entity || Key.IsEmpty()) return false;

	// Mapping entity to redis hash
	if (Settings.bSaveEntityAsHash)
	{
		ITinyRedisPipelinePtr Pipeline = TinyRedis->CreatePipeline();
		Pipeline->Start();
		for (TFieldIterator<FProperty> It(Entity->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Entity);

			// Property -> Json
			FString JsonString;
			{
				TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
				TSharedPtr<FJsonValue> JsonValue = FJsonObjectConverter::UPropertyToJsonValue(Property, PropertyValue);
				JsonObject->SetField(Property->GetName(), JsonValue);
				FJsonSerializer::Serialize(JsonObject, TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&JsonString));
			}

			Pipeline->Command<TinyRedis::HashSet>(Key, Property->GetName().ToLower(), JsonString);
		}
		FRedisPipelineReply Reply = Pipeline->Commit();
		return !Reply.HasError();
	}
	// Mapping entity to redis string
	else
	{
		TArray<uint8> SaveData;
		if (!Serializer->SaveObject(Entity, SaveData))
		{
			UE_LOG(LogGameStorage, Warning, TEXT("SaveEntityToRedis - seriaize failed: %s"), *Key);
			return false;
		}

		FRedisReply Reply = TinyRedis->SetBin(Key, SaveData);
		if (Reply.HasError())
		{
			UE_LOG(LogGameStorage, Warning, TEXT("SaveEntityToRedis - redis error:%s"), *Reply.ToDebugString());
			return false;
		}

		return Reply.IsStatusOK();
	}
}

bool FGameStorageRedis::LoadEntityFromRedis(UObject* Entity, const FString& Key)
{
	// Mapping entity to redis hash
	if (Settings.bSaveEntityAsHash)
	{
		ITinyRedisPipelinePtr Pipeline = TinyRedis->CreatePipeline();
		Pipeline->Start();
		for (TFieldIterator<FProperty> It(Entity->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Entity);

			Pipeline->Command<TinyRedis::HashGet>(Key, Property->GetName().ToLower())->OnReply.BindLambda([Property, PropertyValue](const FRedisReply& Reply)
			{
				// Json -> Property
				FString JsonString = Reply.String;
				TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
				TSharedPtr<FJsonObject> JsonObject;
				FJsonSerializer::Deserialize(JsonReader, JsonObject);
				if (JsonObject)
				{
					TSharedPtr<FJsonValue> JsonValue = JsonObject->TryGetField(Property->GetName());
					if (JsonValue)
					{
						FJsonObjectConverter::JsonValueToUProperty(JsonValue, Property, PropertyValue);
					}
				}
			});
		}
		FRedisPipelineReply Reply = Pipeline->Commit();
		return !Reply.HasError();
	}
	// Mapping entity to redis string
	else
	{
		FRedisReply Reply = TinyRedis->GetBin(Key);
		if (Reply.HasError())
		{
			UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromRedis - redis error:%s"), *Reply.ToDebugString());
			return false;
		}

		if (Reply.IsNil())
		{
			UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromRedis - not exist: %s"), *Key);
			return false;
		}

		if (!Serializer->LoadObject(Entity, Reply.BinArray))
		{
			UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromRedis - seriaize failed: %s"), *Key);
			return false;
		}

		return true;
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
	return SaveEntityToRedis(Entity, Key);
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
	return LoadEntityFromRedis(Entity, Key);
}

bool FGameStorageRedis::LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType, UObject* Outer)
{
	if (!TinyRedis) return false;

	FString KeyPattern = MakeRedisEntityKey(FGameEntityStorageKey(EntityType, TEXT("*")));

	TArray<FString> Keys = TinyRedis->GetKeys(KeyPattern);
	for (auto& Key : Keys)
	{
		UObject* Entity = NewObject<UObject>(Outer, EntityClass);
		if (LoadEntityFromRedis(Entity, Key))
		{
			Entities.Add(Entity);
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
