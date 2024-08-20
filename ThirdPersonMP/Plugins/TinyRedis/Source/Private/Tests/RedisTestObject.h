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
	float Health;
	UPROPERTY()
	int32 Ammo;
	UPROPERTY()
	FVector Location;
};

inline FArchive& operator <<(FArchive& Ar, URedisTestObject& Obj)
{
	Ar << Obj.Health;
	Ar << Obj.Ammo;
	Ar << Obj.Location;
	return Ar;
}
