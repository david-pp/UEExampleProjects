// Fill out your copyright notice in the Description page of Project Settings.


#include "ThirdMPGameInstance.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "ThirdMPGameplayDebugger.h"
#endif // WITH_GAMEPLAY_DEBUGGER

void UThirdMPGameInstance::OnStart()
{
	Super::OnStart();
}

void UThirdMPGameInstance::Init()
{
	Super::Init();

#if WITH_GAMEPLAY_DEBUGGER
	IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
	GameplayDebuggerModule.RegisterCategory("ThirdMP", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_ThirdMP::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 1);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void UThirdMPGameInstance::Shutdown()
{
	Super::Shutdown();
}

int32 UThirdMPGameInstance::AddLocalPlayer(ULocalPlayer* NewPlayer, int32 ControllerId)
{
	int32 Result = Super::AddLocalPlayer(NewPlayer, ControllerId);
	// GetGameViewportClient()->AddViewportWidgetForPlayer(NewPlayer, SAssignNew(MainHUDWidget, SMainHUDWidget), 0);
	return Result;
}

void UThirdMPGameInstance::OpenMainHUDWidget()
{
	if (!MainHUDWidget)
	{
		UGameViewportClient* GameViewportClient = GetGameViewportClient(); // GEngine->GameViewport; 
		if (GameViewportClient)
		{
			GameViewportClient->AddViewportWidgetContent(SAssignNew(MainHUDWidget, SMainHUDWidget));
		}
	}
}

void UThirdMPGameInstance::CloseMainHUDWidget()
{
	UGameViewportClient* GameViewportClient = GetGameViewportClient(); // GEngine->GameViewport; 
	if (GameViewportClient && MainHUDWidget)
	{
		GameViewportClient->RemoveViewportWidgetContent(MainHUDWidget.ToSharedRef());
		MainHUDWidget = nullptr;
	}
}
