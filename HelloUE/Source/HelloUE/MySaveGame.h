// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "MySaveGame.generated.h"

/**
 * 
 */
UCLASS()
class HELLOUE_API UMySaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
	FString PlayerName;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
	int32 UserIndex;
	
	// UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = Basic)
	// FString AddString1;
	
	UMySaveGame();
};
