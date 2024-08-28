// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameStorage.h"

#include "GameStorageRedis.h"
#include "GameStorageFile.h"

DEFINE_LOG_CATEGORY(LogGameStorage);

#define LOCTEXT_NAMESPACE "FGameStorageModule"

void FGameStorageModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	StorageEngineSettings.Namespace = TEXT("tinylab");
	StorageEngineSettings.SerializerType = EGameStorageSerializerType::Sav;

	StorageEngineSettings.bEnableRedisBackend = false;
	StorageEngineSettings.RedisAddress = TEXT("127.0.0.1");
	StorageEngineSettings.RedisPort = 6379;
	StorageEngineSettings.RedisPassword = TEXT("");
	StorageEngineSettings.bSaveEntityAsHash = false;
	GameStorageEngine = CreateStorageEngine(StorageEngineSettings);
}

void FGameStorageModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	GameStorageEngine.Reset();
}

IGameStorageEnginePtr FGameStorageModule::GetStorageEngine() const
{
	return GameStorageEngine;
}

IGameStorageEnginePtr FGameStorageModule::CreateStorageEngine(const FGameStorageEngineSettings& Settings)
{
	if (Settings.bEnableRedisBackend) // redis backend
	{
		IGameStorageEnginePtr StorageEngine = MakeShared<FGameStorageRedis>(Settings);
		return StorageEngine;
	}
	else // file system backend
	{
		IGameStorageEnginePtr StorageEngine = MakeShared<FGameStorageFile>(Settings);
		return StorageEngine;
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameStorageModule, GameStorage)
