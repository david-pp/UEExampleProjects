// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStorageEngine.h"

#include "GameStorage.h"

IGameStorageEnginePtr IGameStorageEngine::GetDefault()
{
	auto Module = FGameStorageModule::Get();
	return Module ? Module->GetStorageEngine() : nullptr;
}
