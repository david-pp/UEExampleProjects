// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageEngine.h"

#include "GameStorage.h"

IGameStorageEnginePtr IGameStorageEngine::GetDefault()
{
	auto Module = FGameStorageModule::Get();
	return Module ? Module->GetStorageEngine() : nullptr;
}


UObject* IGameStorageEngine::LoadAndCreateObject(const FString& Path, TSubclassOf<UObject> EntityClass, UObject* Outer)
{
	UObject* Entity = NewObject<UObject>(Outer, EntityClass);
	if (Entity && LoadObject(Entity, Path))
	{
		return Entity;
	}
	return nullptr;
}
