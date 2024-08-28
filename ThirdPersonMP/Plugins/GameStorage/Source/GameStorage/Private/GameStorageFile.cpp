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

FString FGameStorageFile::GetRootDir() const
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

FString FGameStorageFile::MakeFilePath(const FGameStoragePath& StoragePath) const
{
	if (Settings.SerializerType == EGameStorageSerializerType::Json)
	{
		return GetRootDir() / StoragePath.ToFilePath(TEXT("json"));
	}
	else if (Settings.SerializerType == EGameStorageSerializerType::Sav)
	{
		return GetRootDir() / StoragePath.ToFilePath(TEXT("sav"));
	}

	return TEXT("");
}

FString FGameStorageFile::MakeFilePath(const FGameStorageKey& EntityKey) const
{
	if (Settings.SerializerType == EGameStorageSerializerType::Json)
	{
		return GetRootDir() / EntityKey.Type / FString::Printf(TEXT("%s_%s.json"), *EntityKey.Type, *EntityKey.Id);
	}
	else if (Settings.SerializerType == EGameStorageSerializerType::Sav)
	{
		return GetRootDir() / EntityKey.Type / FString::Printf(TEXT("%s_%s.sav"), *EntityKey.Type, *EntityKey.Id);
	}

	return TEXT("");
}

bool FGameStorageFile::SaveObjectToFile(UObject* Object, const FString& FilePath)
{
	if (!Object) return false;
	if (FilePath.IsEmpty()) return false;

	TArray<uint8> SaveData;
	if (!Serializer->SaveObject(Object, SaveData))
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

bool FGameStorageFile::LoadObjectFromFile(UObject* Object, const FString& FilePath)
{
	if (!Object) return false;
	if (FilePath.IsEmpty()) return false;

	TArray<uint8> SaveData;
	if (!FFileHelper::LoadFileToArray(SaveData, *FilePath))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromFile - failed load from file : %s"), *FilePath);
		return false;
	}

	if (!Serializer->LoadObject(Object, SaveData))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntityFromFile - serialize failed : %s"), *FilePath);
		return false;
	}

	return true;
}

FString FGameStorageFile::GetNamespace() const
{
	return Settings.Namespace;
}

bool FGameStorageFile::SaveObject(UObject* Object, const FString& Path)
{
	FGameStoragePath StoragePath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntity - invalid path:%s"), *Path);
		return false;
	}

	const FString EntityFilePath = MakeFilePath(StoragePath);
	if (EntityFilePath.IsEmpty()) return false;

	return SaveObjectToFile(Object, EntityFilePath);
}

bool FGameStorageFile::LoadObject(UObject* Object, const FString& Path)
{
	FGameStoragePath StoragePath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - invalid path:%s"), *Path);
		return false;
	}

	const FString EntityFilePath = MakeFilePath(StoragePath);
	if (EntityFilePath.IsEmpty()) return false;

	return LoadObjectFromFile(Object, EntityFilePath);
}

bool FGameStorageFile::DeleteObject(const FString& Path)
{
	FGameStoragePath StoragePath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - invalid path:%s"), *Path);
		return false;
	}

	const FString EntityFilePath = MakeFilePath(StoragePath);
	if (EntityFilePath.IsEmpty()) return false;
	return IFileManager::Get().Delete(*EntityFilePath, true, false, true);
}

bool FGameStorageFile::LoadObjects(TArray<UObject*>& Objects, TSubclassOf<UObject> Class, const FString& PathPattern, UObject* Outer)
{
	if (Settings.SerializerType == EGameStorageSerializerType::None)
	{
		// do nothing
		return true;
	}

	// Entity Directory & Type
	//  - PathPattern : key1/key2/.../type:*
	FString EntityDir;
	FString EntityType;
	{
		// entity file directory is ? 
		FString DirectorPattern = PathPattern;
		DirectorPattern.RemoveFromEnd(TEXT(":*"));
		FGameStoragePath StoragePath(DirectorPattern);
		EntityDir = GetRootDir() / StoragePath.ToFilePath();

		// entity type is ?
		TArray<FGameStorageKey> Keys;
		StoragePath.ParseEntityKeys(Keys);
		if (Keys.Num() > 0)
		{
			EntityType = Keys.Last().Type;
		}
	}

	if (EntityDir.IsEmpty() || EntityType.IsEmpty())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntities - invalid path pattern:%s"), *PathPattern);
		return false;
	}

	IFileManager::Get().IterateDirectory(*EntityDir, [this, &Objects, Class, EntityType, Outer](const TCHAR* Pathname, bool bIsDirectory)
	{
		if (!bIsDirectory)
		{
			const FString Filename = FPaths::GetCleanFilename(Pathname);
			if (Settings.SerializerType == EGameStorageSerializerType::Json)
			{
				if (Filename.MatchesWildcard(FString::Printf(TEXT("%s_*.json"), *EntityType)))
				{
					UObject* Entity = NewObject<UObject>(Outer, Class);
					if (LoadObjectFromFile(Entity, Pathname))
					{
						Objects.Add(Entity);
					}
				}
			}
			else if (Settings.SerializerType == EGameStorageSerializerType::Sav)
			{
				if (Filename.MatchesWildcard(FString::Printf(TEXT("%s_*.sav"), *EntityType)))
				{
					UObject* Entity = NewObject<UObject>(Outer, Class);
					if (LoadObjectFromFile(Entity, Pathname))
					{
						Objects.Add(Entity);
					}
				}
			}
		}
		return true;
	});

	return true;
}


