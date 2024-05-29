// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"

UCLASS(Blueprintable)
class UGameItem : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Value;
};

UCLASS(Blueprintable, BlueprintType)
class UGameCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void GrantItems();
	void GrantItemsDeferred();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<TSoftObjectPtr<UGameItem>> ItemList;

	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void DebugServerXXX(const FString& Msg);
};

/**
 * 
 */
UCLASS()
class HELLOUE_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMyPlayerController();
};
