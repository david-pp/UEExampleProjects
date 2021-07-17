// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelloUEGameMode.h"
#include "HelloUECharacter.h"
#include "UObject/ConstructorHelpers.h"

AHelloUEGameMode::AHelloUEGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPersonCPP/Blueprints/ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
