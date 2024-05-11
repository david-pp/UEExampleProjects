// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "HelloGameSettings.generated.h"

UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Hello Game Settings"))
class HELLOUE_API UHelloGameSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/* Default slot name if UI doesn't specify any */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General")
	FString SaveSlotName;

	/* Soft path will be converted to content reference before use */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", AdvancedDisplay)
	TSoftObjectPtr<UDataTable> DummyTablePath;
};
