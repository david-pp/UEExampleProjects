// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageSave.h"

#include "GameStorage.h"
#include "GameStorageSerialization.h"

FGameStorageSave::FGameStorageSave(const FGameStorageEngineSettings& InSettings) : Settings(InSettings)
{
}

FGameStorageSave::~FGameStorageSave()
{
}

FString FGameStorageSave::GetRootDirPath() const
{
	FString Namespace = GetNamespace();
	if (Namespace.IsEmpty())
	{
		return FPaths::ProjectSavedDir();
	}
	else
	{
		return FPaths::ProjectSavedDir() / Namespace;
	}
}

FString FGameStorageSave::MakeEntityFilePath(const FGameEntityStorageKey& EntityKey) const
{
	return GetRootDirPath() / EntityKey.Type / FString::Printf(TEXT("%s_%s.sav"), *EntityKey.Type, *EntityKey.Id);
}

bool FGameStorageSave::SaveEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey)
{
	if (!EntityKey.IsValid())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntity - invalid key:%s"), *EntityKey.ToString());
		return false;
	}

	TArray<uint8> SaveData;
	if (FGameStorageSerialization::SaveObjectToSav(Entity, SaveData))
	{
		FString EntityFilePath = MakeEntityFilePath(EntityKey);
		return FFileHelper::SaveArrayToFile(SaveData, *EntityFilePath);
	}

	return false;
}

bool FGameStorageSave::LoadEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey)
{
	if (!EntityKey.IsValid())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntity - invalid key:%s"), *EntityKey.ToString());
		return false;
	}

	FString EntityFilePath = MakeEntityFilePath(EntityKey);

	TArray<uint8> SaveData;
	if (FFileHelper::LoadFileToArray(SaveData, *EntityFilePath))
	{
		if (FGameStorageSerialization::LoadGameFromSav(Entity, SaveData))
		{
			return true;
		}
	}
	return true;
}

bool FGameStorageSave::LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType, UObject* Outer)
{
	FString EntityDir = GetRootDirPath() / EntityType;
	IFileManager::Get().IterateDirectory(*EntityDir, [&Entities, EntityClass, EntityType, Outer](const TCHAR* Pathname, bool bIsDirectory)
	{
		if (!bIsDirectory)
		{
			const FString Filename = FPaths::GetCleanFilename(Pathname);
			if (Filename.MatchesWildcard(FString::Printf(TEXT("%s_*"), *EntityType)))
			{
				TArray<uint8> SaveData;
				if (FFileHelper::LoadFileToArray(SaveData, Pathname))
				{
					UObject* Entity = NewObject<UObject>(Outer, EntityClass);
					if (Entity && FGameStorageSerialization::LoadGameFromSav(Entity, SaveData))
					{
						Entities.Add(Entity);
					}
				}
			}
		}
		return true;
	});

	return true;
}

bool FGameStorageSave::DeleteEntity(const FGameEntityStorageKey& EntityKey)
{
	FString EntityFilePath = MakeEntityFilePath(EntityKey);
	return IFileManager::Get().Delete(*EntityFilePath, true, false, true);
}

bool FGameStorageSave::AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave)
{
	return true;
}

bool FGameStorageSave::AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad)
{
	return true;
}
