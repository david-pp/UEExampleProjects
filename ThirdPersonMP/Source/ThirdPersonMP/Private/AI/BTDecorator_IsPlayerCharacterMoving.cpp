// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTDecorator_IsPlayerCharacterMoving.h"

#include "Kismet/GameplayStatics.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"

UBTDecorator_IsPlayerCharacterMoving::UBTDecorator_IsPlayerCharacterMoving(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "IsPlayerCharacterMoving";
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
}

bool UBTDecorator_IsPlayerCharacterMoving::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	UWorld* World = GetWorld();
	if (World)
	{
		AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		if (Character)
		{
			return Character->bIsMoving;
		}
	}

	return false;
}

void UBTDecorator_IsPlayerCharacterMoving::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);
}

FString UBTDecorator_IsPlayerCharacterMoving::GetStaticDescription() const
{
	return Super::GetStaticDescription();
}

void UBTDecorator_IsPlayerCharacterMoving::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UWorld* World = GetWorld();
	if (World)
	{
		AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		if (Character)
		{
			DelegateHandle = Character->OnMovingDelegates.AddUObject(this, &UBTDecorator_IsPlayerCharacterMoving::OnMovingChange);
		}
	}
}

void UBTDecorator_IsPlayerCharacterMoving::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UWorld* World = GetWorld();
	if (World)
	{
		AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
		if (Character)
		{
			Character->OnMovingDelegates.Remove(DelegateHandle);
		}
	}
}

void UBTDecorator_IsPlayerCharacterMoving::OnMovingChange(UBehaviorTreeComponent& BehaviorComp)
{
	BehaviorComp.RequestExecution(this);	
}
