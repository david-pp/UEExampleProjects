// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGSpectatorPawn.h"

AOnlineRPGSpectatorPawn::AOnlineRPGSpectatorPawn(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bReplicates = false;
}

void AOnlineRPGSpectatorPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	check(PlayerInputComponent);
	
	PlayerInputComponent->BindAxis("MoveForward", this, &ADefaultPawn::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ADefaultPawn::MoveRight);
	PlayerInputComponent->BindAxis("MoveUp", this, &ADefaultPawn::MoveUp_World);
	PlayerInputComponent->BindAxis("Turn", this, &ADefaultPawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &ADefaultPawn::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &ADefaultPawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AOnlineRPGSpectatorPawn::LookUpAtRate);
}

void AOnlineRPGSpectatorPawn::LookUpAtRate(float Val)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Val * BaseLookUpRate * GetWorld()->GetDeltaSeconds() * CustomTimeDilation);
}
