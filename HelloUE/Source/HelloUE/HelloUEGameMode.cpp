// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelloUEGameMode.h"
#include "HelloUECharacter.h"
#include "FloatingActor.h"
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
