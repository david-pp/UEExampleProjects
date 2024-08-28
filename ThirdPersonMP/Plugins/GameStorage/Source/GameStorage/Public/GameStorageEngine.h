// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameStorageEntity.h"
#include "GameStorageEngine.generated.h"

class IGameStorageEngine;
typedef TSharedPtr<IGameStorageEngine> IGameStorageEnginePtr;

// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UGameStorageEngine : public UInterface
{
	GENERATED_BODY()
};

/**
 * Game Storage Engine
 *
 * [Features]
 *  - Save/Load Bytes to storage
 *  - Save/Load UObject to storage
 *  - Save/Load IGameStorageEntity to storage
 *  - ...
 */
class GAMESTORAGE_API IGameStorageEngine
{
	GENERATED_BODY()

public:
	static IGameStorageEnginePtr GetDefault();

	virtual FString GetNamespace() const = 0;

public:
	//
	// Object -> Game Storage
	//  - Path : storage path string. examples: 
	//		user:01
	//		user:01/profile
	//		user:01/character:001
	//  - Path Pattern : for load objects. examples:
	//		user:*
	//		user:01/character:* 
	// 
	virtual bool SaveObject(UObject* Object, const FString& Path) = 0;
	virtual bool LoadObject(UObject* Object, const FString& Path) = 0;
	virtual bool LoadObjects(TArray<UObject*>& Objects, TSubclassOf<UObject> Class, const FString& PathPattern, UObject* Outer = GetTransientPackage()) = 0;
	virtual bool DeleteObject(const FString& Path) = 0;

	UObject* LoadAndCreateObject(const FString& Path, TSubclassOf<UObject> EntityClass, UObject* Outer = GetTransientPackage());

	template <typename ObjectClass>
	ObjectClass* LoadAndCreateObject(const FString& Path, UObject* Outer = GetTransientPackage())
	{
		return Cast<ObjectClass>(LoadAndCreateObject(Path, ObjectClass::StaticClass(), Outer));
	}

	template <typename ObjectClass>
	bool LoadObjects(TArray<ObjectClass*>& Entities, const FString& PathPattern, UObject* Outer = GetTransientPackage());

	// TODO: Async 
	virtual bool AsyncSaveEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntitySaveDelegate& OnSave = FNativeOnStorageEntitySaveDelegate()) = 0;
	virtual bool AsyncLoadEntity(IGameStorageEntityPtr Entity, const FNativeOnStorageEntityLoadDelegate& OnLoad = FNativeOnStorageEntityLoadDelegate()) = 0;
};


template <typename ObjectClass>
bool IGameStorageEngine::LoadObjects(TArray<ObjectClass*>& Entities, const FString& PathPattern, UObject* Outer)
{
	TArray<UObject*> Objects;
	bool Result = LoadObjects(Objects, ObjectClass::StaticClass(), PathPattern, Outer);
	for (auto Object : Objects)
	{
		ObjectClass* Entity = Cast<ObjectClass>(Object);
		if (Entity)
		{
			Entities.Add(Entity);
		}
	}
	return Result;
}
