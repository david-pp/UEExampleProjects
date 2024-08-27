// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageFile.h"

#include "GameStorage.h"
#include "GameStorageSerializer.h"

FGameStorageFile::FGameStorageFile(const FGameStorageEngineSettings& InSettings) : Settings(InSettings)
{
	Serializer = IGameStorageSerializer::Create(Settings.SerializerType);
}

FGameStorageFile::~FGameStorageFile()
{
}

FString FGameStorageFile::GetRootDirPath() const
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

FString FGameStorageFile::MakeEntityFilePath(const FGameEntityStorageKey& EntityKey) const
{
	if (Settings.SerializerType == EGameStorageSerializerType::Json)
	{
		return GetRootDirPath() / EntityKey.Type / FString::Printf(TEXT("%s_%s.json"), *EntityKey.Type, *EntityKey.Id);
	}
	else if (Settings.SerializerType == EGameStorageSerializerType::Sav)
	{
		return GetRootDirPath() / EntityKey.Type / FString::Printf(TEXT("%s_%s.sav"), *EntityKey.Type, *EntityKey.Id);
	}

	return TEXT("");
}

bool FGameStorageFile::SaveEntityToFile(UObject* Entity, const FString& FilePath)
{
	if (!Entity) return false;
	if (FilePath.IsEmpty()) return false;

	TArray<uint8> SaveData;
	if (!Serializer->SaveObject(Entity, SaveData))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntityToFile - serialize failed: %s"), *FilePath);
		return false;
	}

	if (!FFileHelper::SaveArrayToFile(SaveData, *FilePath))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntityToFile - save file failed: %s"), *FilePath);
		return false;
	}

	return true;
}

bool FGameStorageFile::LoadEntityFromFile(UObject* Entity, const FString& FilePath)
{
	if (!Entity) return false;
	if (FilePath.IsEmpty()) return false;

	TArray<uint8> SaveData;
	if (!FFileHelper::LoadFileToArray(SaveData, *FilePath))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromFile - failed load from file : %s"), *FilePath);
		return false;
	}

	if (!Serializer->LoadObject(Entity, SaveData))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromFile - serialize failed : %s"), *FilePath);
		return false;
	}

	return true;
}

bool FGameStorageFile::SaveEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey)
{
	if (!EntityKey.IsValid())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntity - invalid key:%s"), *EntityKey.ToString());
		return false;
	}

	const FString EntityFilePath = MakeEntityFilePath(EntityKey);
	if (EntityFilePath.IsEmpty()) return false;

	return SaveEntityToFile(Entity, EntityFilePath);
}

bool FGameStorageFile::LoadEntity(UObject* Entity, const FGameEntityStorageKey& EntityKey)
{
	if (!EntityKey.IsValid())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - invalid key:%s"), *EntityKey.ToString());
		return false;
	}

	const FString EntityFilePath = MakeEntityFilePath(EntityKey);
	if (EntityFilePath.IsEmpty()) return false;

	return LoadEntityFromFile(Entity, EntityFilePath);
}

bool FGameStorageFile::LoadEntities(TArray<UObject*>& Entities, TSubclassOf<UObject> EntityClass, const FString& EntityType, UObject* Outer)
{
	if (Settings.SerializerType == EGameStorageSerializerType::None)
	{
		// do nothing
		return true;
	}

	FString EntityDir = GetRootDirPath() / EntityType;
	IFileManager::Get().IterateDirectory(*EntityDir, [this, &Entities, EntityClass, EntityType, Outer](const TCHAR* Pathname, bool bIsDirectory)
	{
		if (!bIsDirectory)
		{
			const FString Filename = FPaths::GetCleanFilename(Pathname);
			if (Settings.SerializerType == EGameStorageSerializerType::Json)
			{
				if (Filename.MatchesWildcard(FString::Printf(TEXT("%s_*.json"), *EntityType)))
				{
					UObject* Entity = NewObject<UObject>(Outer, EntityClass);
					if (LoadEntityFromFile(Entity, Pathname))
					{
						Entities.Add(Entity);
					}
				}
			}
			else if (Settings.SerializerType == EGameStorageSerializerType::Sav)
			{
				if (Filename.MatchesWildcard(FString::Printf(TEXT("%s_*.sav"), *EntityType)))
				{
					UObject* Entity = NewObject<UObject>(Outer, EntityClass);
					if (LoadEntityFromFile(Entity, Pathname))
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

bool FGameStorageFile::DeleteEntity(const FGameEntityStorageKey& EntityKey)
{
	FString EntityFilePath = MakeEntityFilePath(EntityKey);
	return IFileManager::Get().Delete(*EntityFilePath, true, false, true);
}

bool FGameStorageFile::AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave)
{
	return true;
}

bool FGameStorageFile::AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad)
{
	return true;
}
