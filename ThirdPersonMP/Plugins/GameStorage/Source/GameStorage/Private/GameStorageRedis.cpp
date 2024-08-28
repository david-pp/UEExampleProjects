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

FString FGameStorageRedis::MakeRedisKey(const FGameStorageKey& StorageKey) const
{
	FString Namespace = GetNamespace();
	if (Namespace.IsEmpty())
	{
		return FString::Printf(TEXT("%s:%s"), *StorageKey.Type, *StorageKey.Id);
	}
	else
	{
		return FString::Printf(TEXT("%s:%s:%s"), *Namespace, *StorageKey.Type, *StorageKey.Id);
	}
}

bool FGameStorageRedis::SaveObjectToRedis(UObject* Object, const FString& Key)
{
	if (!Object || Key.IsEmpty()) return false;

	// Mapping entity to redis hash
	if (Settings.bSaveEntityAsHash)
	{
		ITinyRedisPipelinePtr Pipeline = TinyRedis->CreatePipeline();
		Pipeline->Start();
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Object);

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
		if (!Serializer->SaveObject(Object, SaveData))
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

bool FGameStorageRedis::LoadObjectFromRedis(UObject* Object, const FString& Key)
{
	// Mapping entity to redis hash
	if (Settings.bSaveEntityAsHash)
	{
		ITinyRedisPipelinePtr Pipeline = TinyRedis->CreatePipeline();
		Pipeline->Start();
		for (TFieldIterator<FProperty> It(Object->GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Object);

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

		if (!Serializer->LoadObject(Object, Reply.BinArray))
		{
			UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromRedis - seriaize failed: %s"), *Key);
			return false;
		}

		return true;
	}
}

FString FGameStorageRedis::GetNamespace() const
{
	return Settings.Namespace;
}

bool FGameStorageRedis::SaveObject(UObject* Object, const FString& Path)
{
	if (!TinyRedis) return false;

	FGameStoragePath StoragePath(GetNamespace());
	StoragePath.AppendPath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveObject - invalid path:%s"), *Path);
		return false;
	}

	FString Key = StoragePath.ToRedisKey();
	return SaveObjectToRedis(Object, Key);
}

bool FGameStorageRedis::LoadObject(UObject* Object, const FString& Path)
{
	if (!TinyRedis) return false;

	FGameStoragePath StoragePath(GetNamespace());
	StoragePath.AppendPath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadObject - invalid path:%s"), *Path);
		return false;
	}

	FString Key = StoragePath.ToRedisKey();
	return LoadObjectFromRedis(Object, Key);
}

bool FGameStorageRedis::LoadObjects(TArray<UObject*>& Objects, TSubclassOf<UObject> Class, const FString& PathPattern, UObject* Outer)
{
	if (!TinyRedis) return false;

	FGameStoragePath StoragePath(GetNamespace());
	StoragePath.AppendPath(PathPattern);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadObjects - invalid path:%s"), *PathPattern);
		return false;
	}

	FString KeyPattern = StoragePath.ToRedisKey(); // Path:*
	TArray<FString> Keys = TinyRedis->GetKeys(KeyPattern);
	for (auto& Key : Keys)
	{
		UObject* Entity = NewObject<UObject>(Outer, Class);
		if (LoadObjectFromRedis(Entity, Key))
		{
			Objects.Add(Entity);
		}
	}

	return true;
}

bool FGameStorageRedis::DeleteObject(const FString& Path)
{
	if (!TinyRedis) return false;

	FGameStoragePath StoragePath(GetNamespace());
	StoragePath.AppendPath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("DeleteObject - invalid path:%s"), *Path);
		return false;
	}

	FString Key = StoragePath.ToRedisKey();
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
