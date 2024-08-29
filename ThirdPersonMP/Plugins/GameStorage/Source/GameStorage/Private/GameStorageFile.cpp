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
		UE_LOG(LogGameStorage, Warning, TEXT("SaveObjectToFile - serialize failed: %s"), *FilePath);
		return false;
	}

	if (!FFileHelper::SaveArrayToFile(SaveData, *FilePath))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveObjectToFile - save file failed: %s"), *FilePath);
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
		UE_LOG(LogGameStorage, Warning, TEXT("LoadObjectFromFile - failed load from file : %s"), *FilePath);
		return false;
	}

	if (!Serializer->LoadObject(Object, SaveData))
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadObjectFromFile - serialize failed : %s"), *FilePath);
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


bool FGameStorageFile::AsyncSaveObject(UObject* Object, const FString& Path, const FOnStorageObjectSaveDelegate& OnComplete)
{
	FGameStoragePath StoragePath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("SaveEntity - invalid path:%s"), *Path);
		return false;
	}

	const FString FilePath = MakeFilePath(StoragePath);
	if (FilePath.IsEmpty()) return false;

	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [This = AsShared(), Object, FilePath, OnComplete]()
	{
		FString Error;
		if (!This->SaveObjectToFile(Object, FilePath))
		{
			Error = FString::Printf(TEXT("Save object to file failed : %s"), *FilePath);
		}
		if (OnComplete.IsBound())
		{
			AsyncTask(ENamedThreads::GameThread, [Object, OnComplete, Error]()
			{
				OnComplete.ExecuteIfBound(Object, Error);
			});
		}
	});

	return true;
}

bool FGameStorageFile::AsyncLoadObject(UObject* Object, const FString& Path, const FOnStorageObjectLoadDelegate& OnComplete)
{
	FGameStoragePath StoragePath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - invalid path:%s"), *Path);
		return false;
	}

	const FString FilePath = MakeFilePath(StoragePath);
	if (FilePath.IsEmpty()) return false;

	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [This = AsShared(), Object, FilePath, OnComplete]()
	{
		FString Error;
		if (!This->LoadObjectFromFile(Object, FilePath))
		{
			Error = FString::Printf(TEXT("Load object to file failed : %s"), *FilePath);
		}
		if (OnComplete.IsBound())
		{
			AsyncTask(ENamedThreads::GameThread, [Object, OnComplete, Error]()
			{
				OnComplete.ExecuteIfBound(Object, Error);
			});
		}
	});

	return true;
}

bool FGameStorageFile::AsyncLoadObjects(TSubclassOf<UObject> Class, const FString& PathPattern, UObject* Outer, const FOnStorageObjectsLoadDelegate& OnComplete)
{
	if (Settings.SerializerType == EGameStorageSerializerType::None)
	{
		return true;
	}

	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [This = AsShared(),Class, PathPattern, Outer, OnComplete]()
	{
		FString Error;
		TArray<UObject*> Objects;
		if (!This->LoadObjects(Objects, Class, PathPattern, Outer))
		{
			Error = FString::Printf(TEXT("Load objects failed : %s"), *PathPattern);
		}

		if (OnComplete.IsBound())
		{
			// to prevent load object GC
			for (UObject* Object : Objects)
			{
				Object->AddToRoot();
			}
			
			AsyncTask(ENamedThreads::GameThread, [Objects, OnComplete, Error]()
			{
				OnComplete.ExecuteIfBound(Objects, Error);
				for (UObject* Object : Objects)
				{
					Object->RemoveFromRoot();
				}
			});
		}
	});

	return true;
}

bool FGameStorageFile::AsyncDeleteObject(const FString& Path, const FOnStorageObjectsDeleteDelegate& OnComplete)
{
	FGameStoragePath StoragePath(Path);
	if (!StoragePath.IsValidPath())
	{
		UE_LOG(LogGameStorage, Warning, TEXT("LoadEntity - invalid path:%s"), *Path);
		return false;
	}

	const FString FilePath = MakeFilePath(StoragePath);
	if (FilePath.IsEmpty()) return false;

	AsyncTask(ENamedThreads::AnyHiPriThreadNormalTask, [This = AsShared(), FilePath, Path, OnComplete]()
	{
		FString Error;
		if (!IFileManager::Get().Delete(*FilePath, true, false, true))
		{
			Error = FString::Printf(TEXT("delete object file failed : %s"), *FilePath);
		}
		if (OnComplete.IsBound())
		{
			AsyncTask(ENamedThreads::GameThread, [OnComplete, Path, Error]()
			{
				OnComplete.ExecuteIfBound(Path, Error);
			});
		}
	});

	return true;
}
