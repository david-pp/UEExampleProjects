// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"

#include "GameGlobals.h"
#include "Engine/StreamableManager.h"
#include "HelloUE/HelloUE.h"

void UGameCheatManager::GrantItems()
{
	TArray<FSoftObjectPath> ItemsToStream;

	FStreamableManager& Streamable = UGameGlobals::Get().StreamableManager;

	for (int32 i = 0; i < ItemList.Num(); ++i)
	{
		ItemsToStream.AddUnique(ItemList[i].ToSoftObjectPath());
	}

	Streamable.RequestAsyncLoad(ItemsToStream, FStreamableDelegate::CreateUObject(this, &UGameCheatManager::GrantItemsDeferred));
}

void UGameCheatManager::GrantItemsDeferred()
{
	for (int32 i = 0; i < ItemList.Num(); ++i)
	{
		UGameItem* ItemData = ItemList[i].Get();
		if (ItemData)
		{
			UE_LOG(LogHello, Log, TEXT("Item-%d"), ItemData->Value);
		}
	}
}

void UGameCheatManager::DebugServerXXX(const FString& Msg)
{
	UE_LOG(LogHello, Log, TEXT("DebugServerXXX - %s"), *Msg);
}

AMyPlayerController::AMyPlayerController()
{
	CheatClass = UGameCheatManager::StaticClass();
}
