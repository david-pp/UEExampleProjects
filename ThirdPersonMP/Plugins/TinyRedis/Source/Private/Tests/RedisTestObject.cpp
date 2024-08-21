// Fill out your copyright notice in the Description page of Project Settings.


#include "RedisTestObject.h"

void URedisTestObject::SerializeJson(FStructuredArchive& Ar)
{
	FStructuredArchive::FRecord RootRecord = Ar.Open().EnterRecord();

	RootRecord << SA_VALUE(TEXT("TypeName"), TypeName);
	RootRecord << SA_VALUE(TEXT("Health"), Health);
	RootRecord << SA_VALUE(TEXT("Ammo"), Ammo);

	FStructuredArchive::FRecord PosRecord = RootRecord.EnterField(SA_FIELD_NAME(TEXT("Position"))).EnterRecord();
	PosRecord << SA_VALUE(TEXT("X"), Location.X);
	PosRecord << SA_VALUE(TEXT("Y"), Location.Y);
	PosRecord << SA_VALUE(TEXT("Z"), Location.Z);

	Ar.Close();
}
