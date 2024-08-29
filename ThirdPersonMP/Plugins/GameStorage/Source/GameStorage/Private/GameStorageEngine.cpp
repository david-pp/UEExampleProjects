// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageEngine.h"

#include "GameStorage.h"

IGameStorageEnginePtr IGameStorageEngine::GetDefault()
{
	auto Module = FGameStorageModule::Get();
	return Module ? Module->GetStorageEngine() : nullptr;
}


UObject* IGameStorageEngine::LoadNewObject(const FString& Path, TSubclassOf<UObject> Class, UObject* Outer)
{
	UObject* Object = NewObject<UObject>(Outer, Class);
	if (Object && LoadObject(Object, Path))
	{
		return Object;
	}
	return nullptr;
}

bool IGameStorageEngine::AsyncSaveObject(UObject* Object, const FString& Path, const FOnStorageObjectSaveDelegate& OnComplete)
{
	return false;
}

bool IGameStorageEngine::AsyncLoadObject(UObject* Object, const FString& Path, const FOnStorageObjectLoadDelegate& OnComplete)
{
	return false;
}

bool IGameStorageEngine::AsyncLoadNewObject(const FString& Path, TSubclassOf<UObject> Class, UObject* Outer, const FOnStorageObjectLoadDelegate& OnComplete)
{
	UObject* Object = NewObject<UObject>(Outer, Class);
	if (Object)
	{
		return AsyncLoadObject(Object, Path, OnComplete);
	}

	return false;
}

bool IGameStorageEngine::AsyncLoadObjects(TSubclassOf<UObject> Class, const FString& PathPattern, UObject* Outer, const FOnStorageObjectsLoadDelegate& OnComplete)
{
	return false;
}

bool IGameStorageEngine::AsyncDeleteObject(const FString& Path, const FOnStorageObjectsDeleteDelegate& OnComplete)
{
	return false;
}
