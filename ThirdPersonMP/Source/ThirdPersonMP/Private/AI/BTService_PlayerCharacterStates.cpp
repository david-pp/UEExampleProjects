// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTService_PlayerCharacterStates.h"

#include "Kismet/GameplayStatics.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"

UBTService_PlayerCharacterStates::UBTService_PlayerCharacterStates(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "PlayerCharacterStatesService";

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
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

	// Get Controlled Owner
	AActor* QueryOwner = OwnerComp.GetOwner();
	AController* ControllerOwner = Cast<AController>(QueryOwner);
	if (ControllerOwner)
	{
		QueryOwner = ControllerOwner->GetPawn();
	}

	UWorld* World = GetWorld();
	if (World)
	{
		AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		if (Character)
		{
			bool OldIsMoving = Character->bIsMoving;
			Character->bIsMoving = (Character->GetVelocity().Size() > FLT_MIN);

			if (Character->bIsMoving != OldIsMoving)
			{
				Character->OnMovingDelegates.Broadcast(OwnerComp);
			}
		}
	}
}
