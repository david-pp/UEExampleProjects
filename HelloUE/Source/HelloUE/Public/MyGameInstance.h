// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "HelloUE/HelloUE.h"
#include "MyGameInstance.generated.h"

UCLASS()
class UMyUISubSystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/* GameInstance::Init **/
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	/* GameInstance::Shutdown **/
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable)
	UUserWidget* ShowWidget(TSubclassOf<UUserWidget> WidgetClass);
};

/**
 * 
 */
UCLASS()
class HELLOUE_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	virtual void Shutdown() override;

protected:
	virtual void OnStart() override;

public:
	virtual TSubclassOf<AGameModeBase> OverrideGameModeClass(TSubclassOf<AGameModeBase> GameModeClass, const FString& MapName, const FString& Options, const FString& Portal) const override;
	
	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UMyUISubSystem* UISystem;
};
