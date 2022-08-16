// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MySaveGame.h"
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

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Classes)
	TSubclassOf<UMySaveGame> SaveGameClass;
	
	/** SaveGame **/
	UFUNCTION(BlueprintCallable)
	void SaveGame(int32 UserIndex);

	UFUNCTION(BlueprintCallable)
	void LoadGame(int32 UserIndex);

	UFUNCTION(BlueprintImplementableEvent)
	void OnSave(const FString& SlotName, const int32 UserIndex, UMySaveGame* SaveGame);
	
	UFUNCTION(BlueprintImplementableEvent)
	void OnLoaded(const FString& SlotName, const int32 UserIndex, UMySaveGame* SaveGame);
	

	UFUNCTION(BlueprintCallable)
	void AsyncSaveGame(int32 UserIndex);

	UFUNCTION(BlueprintCallable)
	void AsyncLoadGame(int32 UserIndex);

	UFUNCTION(BlueprintImplementableEvent)
	void OnAsyncSaved(const FString& SlotName, const int32 UserIndex, bool IsSucess);

	UFUNCTION(BlueprintImplementableEvent)
	void OnAsyncLoaded(const FString& SlotName, const int32 UserIndex, UMySaveGame* SaveGame);
};
