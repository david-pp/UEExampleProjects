// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"

#include "Blueprint/UserWidget.h"

void UMyUISubSystem::Initialize(FSubsystemCollectionBase& Collection)
{
	UE_LOG(LogHello, Log, TEXT("UMyUISubSystem::Initialize"));
}

void UMyUISubSystem::Deinitialize()
{
	UE_LOG(LogHello, Log, TEXT("UMyUISubSystem::Deinitialize"));
}

UUserWidget* UMyUISubSystem::ShowWidget(TSubclassOf<UUserWidget> WidgetClass)
{
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetGameInstance(), WidgetClass);
	if (Widget)
	{
		Widget->AddToViewport();
	}

	return Widget;
}

void UMyGameInstance::Init()
{
	Super::Init();
	UE_LOG(LogHello, Log, TEXT("UMyGameInstance::Init"));
	
	UISystem = GetSubsystem<UMyUISubSystem>();
}

void UMyGameInstance::Shutdown()
{
	Super::Shutdown();
	UE_LOG(LogHello, Log, TEXT("UMyGameInstance::Shutdown"));
}

void UMyGameInstance::OnStart()
{
	Super::OnStart();

	UE_LOG(LogHello, Log, TEXT("UMyGameInstance::OnStart"));
}

TSubclassOf<AGameModeBase> UMyGameInstance::OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass,
	const FString& MapName, const FString& Options, const FString& Portal) const
{
	return Super::OverrideGameModeClass(GameModeClass, MapName, Options, Portal);
}
