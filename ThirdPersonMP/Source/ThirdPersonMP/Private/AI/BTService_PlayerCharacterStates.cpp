// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_PlayerCharacterStates.h"

#include "AIController.h"
#include "AI/AIFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"

UBTService_PlayerCharacterStates::UBTService_PlayerCharacterStates(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "PlayerCharacterStatesService";

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;

	bCreateNodeInstance = true;
}

void UBTService_PlayerCharacterStates::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
}

void UBTService_PlayerCharacterStates::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}

void UBTService_PlayerCharacterStates::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	// Get Player Input (Global)
	UWorld* World = GetWorld();
	if (World)
	{
		AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		if (Character)
		{
			bool OldIsMoving = Character->bIsMoving;
			Character->bIsMoving = (Character->GetVelocity().Size() > FLT_MIN);

			// Trigger Move Branch
			if (Character->bIsMoving != OldIsMoving)
			{
				Character->OnMovingDelegates.Broadcast(OwnerComp);
			}
		}
	}

	// Get Owner's Anim Trigger ...
	AThirdPersonMPCharacter* OwnerCharacter = Cast<AThirdPersonMPCharacter>(UAIFunctionLibrary::GetBTCompOwnerCharacter(&OwnerComp));
	if (OwnerCharacter)
	{
		// Trigger Notify
		UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();
			if (MontageInstance)
			{
				float Position = MontageInstance->GetPosition();
				float Length = MontageInstance->Montage->GetPlayLength();
				float Ratio = Position / Length;

				OwnerCharacter->OnMontageAdvanced(OwnerComp, LastAnimMontageRatio, Ratio);
				LastAnimMontageRatio = Ratio;
			}
		}
	}
}
