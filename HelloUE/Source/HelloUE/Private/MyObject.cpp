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

void UMyObject::K2_HelloWorld()
{
	HelloWorld();
}

void UMyObject::HelloWorld_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("UMyObject-HelloWorld_Implementation"));
}

void UMyDerivedObject::HelloWorld_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("UMyDerivedObject-HelloWorld_Implementation"));
}

void UMyDerivedObject2::HelloWorld_Implementation()
{
	UE_LOG(LogTemp, Display, TEXT("UMyDerivedObject2-HelloWorld_Implementation"));
}
