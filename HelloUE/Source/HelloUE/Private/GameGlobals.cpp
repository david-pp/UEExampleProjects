// Fill out your copyright notice in the Description page of Project Settings.


#include "GameGlobals.h"

UGameGlobals& UGameGlobals::Get()
{
	UGameGlobals* GameGlobals = Cast<UGameGlobals>(GEngine->GameSingleton);
	if (GameGlobals)
	{
		return *GameGlobals;
	}
	else
	{
		static UGameGlobals* Singleton = nullptr;
		if (!Singleton)
		{
			Singleton = NewObject<UGameGlobals>();
		}
		return *Singleton;
	}
}
