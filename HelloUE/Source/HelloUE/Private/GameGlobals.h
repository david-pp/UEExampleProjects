// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "UObject/Object.h"
#include "GameGlobals.generated.h"

/**
 * Global Singleton - before asset manager contruction
 */
UCLASS()
class HELLOUE_API UGameGlobals : public UObject
{
	GENERATED_BODY()

public:
	static UGameGlobals& Get();
	
	FStreamableManager StreamableManager;
};
