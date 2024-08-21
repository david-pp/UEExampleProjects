// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RedisTestObject.generated.h"

/**
 * 
 */
UCLASS()
class TINYREDIS_API URedisTestObject : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString TypeName;
	UPROPERTY()
	float Health;
	UPROPERTY()
	int32 Ammo;
	UPROPERTY()
	FVector Location;

public:
	void SerializeJson(FStructuredArchive& Ar);
};

inline FArchive& operator <<(FArchive& Ar, URedisTestObject& Obj)
{
	Ar << Obj.Health;
	Ar << Obj.Ammo;
	Ar << Obj.Location;
	return Ar;
}


UCLASS()
class TINYREDIS_API URedisTestObject_ModifyAdd : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString TypeName;
	UPROPERTY()
	float Health;
	/** Add */
	UPROPERTY()
	int32 Mana = 1024;
	UPROPERTY()
	int32 Ammo;
	UPROPERTY()
	FVector Location;
};

UCLASS()
class TINYREDIS_API URedisTestObject_ModifyRemove : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString TypeName;
	UPROPERTY()
	float Health;
	UPROPERTY()
	FVector Location;
};

