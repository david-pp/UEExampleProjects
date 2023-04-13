// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PlayMontage.h"
#include "AIController.h"
#include "GameFramework/Character.h"

UBTTask_PlayMontage::UBTTask_PlayMontage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = TEXT("PlayMontage");
}

EBTNodeResult::Type UBTTask_PlayMontage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MontageToPlay && MyController && MyController->GetPawn())
	{
		USkeletalMeshComponent* SkelMesh = nullptr;
		ACharacter* const MyCharacter = Cast<ACharacter>(MyController->GetPawn());
		if (MyCharacter)
		{
			MyCharacter->PlayAnimMontage(MontageToPlay, PlayRate, StartSectionName);
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

UBTTask_StopMontage::UBTTask_StopMontage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

EBTNodeResult::Type UBTTask_StopMontage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MyController && MyController->GetPawn())
	{
		ACharacter* const MyCharacter = Cast<ACharacter>(MyController->GetPawn());
		if (MyCharacter)
		{
			MyCharacter->StopAnimMontage();
		}
	}

	return Super::ExecuteTask(OwnerComp, NodeMemory);
}

UBTDecorator_MontageRatioRange::UBTDecorator_MontageRatioRange(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "MontageRatioRange";
	// FlowAbortMode = EBTFlowAbortMode::LowerPriority;

	// bNotifyBecomeRelevant = true;
	// bNotifyCeaseRelevant = true;
}

bool UBTDecorator_MontageRatioRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MyController && MyController->GetPawn())
	{
		ACharacter* const MyCharacter = Cast<ACharacter>(MyController->GetPawn());
		if (MyCharacter && MyCharacter->GetMesh())
		{
			UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				// UAnimMontage* Montage = AnimInstance->GetCurrentActiveMontage();
				// if (Montage)
				// {
				// 	
				// 	float Position = AnimInstance->Montage_GetPosition(Montage);
				// 	float Length = Montage->GetPlayLength();
				// 	float Ratio = Position / Length;
				//
				// 	return FMath::IsWithin(Ratio, RatioMin, RatioMax);
				// }

				FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();
				if (MontageInstance)
				{
					float Position = MontageInstance->GetPosition();
					float Length = MontageInstance->Montage->GetPlayLength();
					float Ratio = Position / Length;

					return FMath::IsWithin(Ratio, RatioMin, RatioMax);
				}
			}
		}
	}

	return false;
}

void UBTDecorator_MontageRatioRange::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);
}

void UBTDecorator_MontageRatioRange::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);
}
