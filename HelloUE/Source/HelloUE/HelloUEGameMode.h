// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "HelloUEGameMode.generated.h"

class AFloatingActor;

UCLASS(Blueprintable)
class AHelloUEGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AHelloUEGameMode();

	UFUNCTION(BlueprintCallable)
	AFloatingActor* SpawnMyActor(FName Name);

	UFUNCTION(BlueprintCallable)
	AFloatingActor* SpawnMyActor2(FName Name);
};



