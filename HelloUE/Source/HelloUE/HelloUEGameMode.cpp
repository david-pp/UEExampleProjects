// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelloUEGameMode.h"
#include "HelloUECharacter.h"
#include "FloatingActor.h"
#include "HelloGameSettings.h"
#include "MySaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AHelloUEGameMode::AHelloUEGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}

	SaveGameClass = UMySaveGame::StaticClass();
}

AFloatingActor* AHelloUEGameMode::SpawnMyActor(FName Name)
{
	UWorld* World = GetWorld();
	ACharacter* Character = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (Character)
	{
		FVector Location = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f;
		FRotator Rotator;
		AFloatingActor* Actor = World->SpawnActor<AFloatingActor>(Location, Rotator);
		if (Actor)
		{
			// BeginPlay之后才生效
			Actor->MyName = Name;
			return Actor;
		}
	}
	return nullptr;
}

AFloatingActor* AHelloUEGameMode::SpawnMyActor2(FName Name)
{
	UWorld* World = GetWorld();
	ACharacter* Character = UGameplayStatics::GetPlayerCharacter(World, 0);
	if (Character)
	{
		FVector Location = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f;
		FRotator Rotator;
		FTransform Transform;
		Transform.SetLocation(Location);
		Transform.SetRotation(Rotator.Quaternion());

		AFloatingActor* Actor = World->SpawnActorDeferred<AFloatingActor>(AFloatingActor::StaticClass(), Transform);
		if (Actor)
		{
			// BeginPlay之后才生效
			Actor->MyName = Name;
			// BeginPlay会在FinsihSpawning里面执行
			Actor->FinishSpawning(Transform);
			return Actor;
		}
	}
	return nullptr;
}

static FString SlotNameString = TEXT("HelloUE");

void AHelloUEGameMode::SaveGame(int32 UserIndex)
{
	UMySaveGame* SaveGame = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(SaveGameClass));
	if (SaveGame)
	{
		SaveGame->PlayerName = FString::Printf(TEXT("Player-%d"), UserIndex);
		SaveGame->UserIndex = UserIndex;
		SaveGame->SaveSlotName = SlotNameString;
		// SaveGame->AddString1 = TEXT("AddedValue")
		OnSave(SlotNameString, UserIndex, SaveGame);

		// Save the data immediately.
		if (UGameplayStatics::SaveGameToSlot(SaveGame, SlotNameString, UserIndex))
		{
			// Save succeeded.
		}
	}
}

void AHelloUEGameMode::LoadGame(int32 UserIndex)
{
	const UHelloGameSettings* GameSettings = GetDefault<UHelloGameSettings>();
	if (GameSettings)
	{
		// GameSettings->SaveSlotName;
	}
	
	// Retrieve and cast the USaveGame object to UMySaveGame.
	if (UMySaveGame* LoadedGame = Cast<UMySaveGame>(UGameplayStatics::LoadGameFromSlot(SlotNameString, 0)))
	{
		// The operation was successful, so LoadedGame now contains the data we saved earlier.
		// UE_LOG(LogTemp, Warning, TEXT("LOADED: %s,%d,%s, Add:%s"),
		// 	*LoadedGame->PlayerName, LoadedGame->UserIndex, *LoadedGame->SaveSlotName, *LoadedGame->AddString1);

		UE_LOG(LogTemp, Warning, TEXT("LOADED: %s,%d,%s"),
			*LoadedGame->PlayerName, LoadedGame->UserIndex, *LoadedGame->SaveSlotName);

		OnLoaded(SlotNameString, LoadedGame->UserIndex, LoadedGame);
	}
}

void AHelloUEGameMode::AsyncSaveGame(int32 UserIndex)
{
	if (UMySaveGame* SaveGameInstance = Cast<UMySaveGame>(UGameplayStatics::CreateSaveGameObject(UMySaveGame::StaticClass())))
	{
		// Set up the (optional) delegate.
		FAsyncSaveGameToSlotDelegate SavedDelegate;

		// USomeUObjectClass::SaveGameDelegateFunction is a void function that takes the following parameters: const FString& SlotName, const int32 UserIndex, bool bSuccess
		SavedDelegate.BindUObject(this, &AHelloUEGameMode::OnAsyncSaved);

		// Set data on the savegame object.
		SaveGameInstance->PlayerName = FString::Printf(TEXT("Player-%d"), UserIndex);

		// Start async save process.
		UGameplayStatics::AsyncSaveGameToSlot(SaveGameInstance, SlotNameString, UserIndex, SavedDelegate);
	}
}

void AHelloUEGameMode::AsyncLoadGame(int32 UserIndex)
{
	// Set up the delegate.
	UGameplayStatics::AsyncLoadGameFromSlot(SlotNameString, 0,
	                                        FAsyncLoadGameFromSlotDelegate::CreateLambda([this](const FString& SlotName, const int32 UserIndex, USaveGame* SaveGame)
	                                        {
		                                        this->OnAsyncLoaded(SlotName, UserIndex, Cast<UMySaveGame>(SaveGame));
	                                        }));
}
