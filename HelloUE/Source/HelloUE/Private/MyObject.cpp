// Fill out your copyright notice in the Description page of Project Settings.


#include "MyObject.h"


UMyObject::UMyObject()
{
	UE_LOG(LogTemp, Display, TEXT("UMyObject-Contruct"));
}

UMyObject::~UMyObject()
{
	UE_LOG(LogTemp, Display, TEXT("UMyObject-Destroy: Value=%d"), Value);
}
