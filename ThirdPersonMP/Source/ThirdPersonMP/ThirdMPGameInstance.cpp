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
	GameplayDebuggerModule.RegisterCategory(
		"ThirdMP", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_ThirdMP::MakeInstance),
		EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 1);
	GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void UThirdMPGameInstance::Shutdown()
{
	Super::Shutdown();
}
