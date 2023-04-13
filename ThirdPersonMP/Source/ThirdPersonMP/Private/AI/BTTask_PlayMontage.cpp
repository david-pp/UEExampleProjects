// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PlayMontage.h"
#include "AIController.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"

UBTTask_PlayMontage::UBTTask_PlayMontage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = TEXT("PlayMontage");

	// instantiating to be able to use Timers
	bCreateNodeInstance = true;

	bNonBlocking = false;
	bNonStopMontage = true;
}

EBTNodeResult::Type UBTTask_PlayMontage::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	MyOwnerComp = &OwnerComp;

	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MontageToPlay && MyController && MyController->GetPawn())
	{
		USkeletalMeshComponent* SkelMesh = nullptr;
		ACharacter* const Character = Cast<ACharacter>(MyController->GetPawn());
		if (Character)
		{
			SkelMesh = Character->GetMesh();
		}
		else
		{
			SkelMesh = MyController->GetPawn()->FindComponentByClass<USkeletalMeshComponent>();
		}

		ACharacter* const MyCharacter = Cast<ACharacter>(MyController->GetPawn());
		if (MyCharacter && SkelMesh)
		{
			CachedSkelMesh = SkelMesh;

			MyCharacter->PlayAnimMontage(MontageToPlay, PlayRate, StartSectionName);

			UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				if (bNonBlocking == false)
				{
					BlendingOutDelegate.BindUObject(this, &UBTTask_PlayMontage::OnMontageBlendingOut);
					AnimInstance->Montage_SetBlendingOutDelegate(BlendingOutDelegate, MontageToPlay);

					MontageEndedDelegate.BindUObject(this, &UBTTask_PlayMontage::OnMontageEnded);
					AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, MontageToPlay);

					return EBTNodeResult::InProgress;
				}
				else
				{
					// we're done here, report success so that BT can pick next task
					return EBTNodeResult::Succeeded;
				}
			}
		}
	}

	return EBTNodeResult::Failed;
}

EBTNodeResult::Type UBTTask_PlayMontage::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	OnMontageInterrupted();

	if (bNonStopMontage == false)
		StopPlayingMontage();

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_PlayMontage::OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted)
{
}

void UBTTask_PlayMontage::OnMontageInterrupted()
{
}

void UBTTask_PlayMontage::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	FinishLatentTask(*MyOwnerComp, EBTNodeResult::Succeeded);
}

bool UBTTask_PlayMontage::StopPlayingMontage()
{
	if (CachedSkelMesh)
	{
		UAnimInstance* AnimInstance = CachedSkelMesh->GetAnimInstance();

		if (AnimInstance)
		{
			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveInstanceForMontage(MontageToPlay);
			if (MontageInstance)
			{
				static const float MONTAGE_PREDICTION_REJECT_FADETIME = 0.25f;

				MontageInstance->OnMontageBlendingOutStarted.Unbind();
				MontageInstance->OnMontageEnded.Unbind();
				MontageInstance->Stop(FAlphaBlend(MontageToPlay->BlendOut, MONTAGE_PREDICTION_REJECT_FADETIME));
				return true;
			}
		}
	}

	return false;
}


// -------------------------------

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

UBTDecorator_MontageRatioRange::UBTDecorator_MontageRatioRange(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
	NodeName = "MontageRatioRange";
	// FlowAbortMode = EBTFlowAbortMode::LowerPriority;

	// bNotifyBecomeRelevant = true;
	// bNotifyCeaseRelevant = true;
}

bool UBTDecorator_MontageRatioRange::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
                                                                uint8* NodeMemory) const
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

UBTDecorator_MontageRatioCheck::UBTDecorator_MontageRatioCheck(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
	NodeName = TEXT("MontageRatioCheck");
	bCreateNodeInstance = true;
	bNotifyTick = true;
}

bool UBTDecorator_MontageRatioCheck::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp,
                                                                uint8* NodeMemory) const
{
	UBTDecorator_MontageRatioCheck* MutableThis = const_cast<UBTDecorator_MontageRatioCheck*>(this);

	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MyController && MyController->GetPawn())
	{
		ACharacter* const MyCharacter = Cast<ACharacter>(MyController->GetPawn());
		if (MyCharacter && MyCharacter->GetMesh())
		{
			UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance();
			if (AnimInstance)
			{
				FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();
				if (MontageInstance)
				{
					float Position = MontageInstance->GetPosition();
					float Length = MontageInstance->Montage->GetPlayLength();
					float CurrentRatio = Position / Length;

					bool Result = false;
					if (FMath::IsWithin(Ratio, LastAnimRatio, CurrentRatio))
					{
						Result = true;
					}
					MutableThis->LastAnimRatio = CurrentRatio;
					return Result;
				}
			}
		}
	}

	return false;
}

void UBTDecorator_MontageRatioCheck::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	// AAIController* const MyController = OwnerComp.GetAIOwner();
	// if (MyController && MyController->GetPawn())
	// {
	// 	ACharacter* const MyCharacter = Cast<ACharacter>(MyController->GetPawn());
	// 	if (MyCharacter && MyCharacter->GetMesh())
	// 	{
	// 		UAnimInstance* AnimInstance = MyCharacter->GetMesh()->GetAnimInstance();
	// 		if (AnimInstance)
	// 		{
	// 			FAnimMontageInstance* MontageInstance = AnimInstance->GetActiveMontageInstance();
	// 			if (MontageInstance)
	// 			{
	// 				float Position = MontageInstance->GetPosition();
	// 				float Length = MontageInstance->Montage->GetPlayLength();
	// 				float CurrentRatio = Position / Length;
	//
	// 				bool Result = false;
	// 				if (FMath::IsWithin(Ratio, LastAnimRatio, CurrentRatio))
	// 				{
	// 					Result = true;
	// 				}
	// 				LastAnimRatio = CurrentRatio;
	// 			}
	// 		}
	// 	}
	// }
}

/**
 * @brief 
 * @param ObjectInitializer ///
 */
UBTDecorator_MontageRatio::UBTDecorator_MontageRatio(const FObjectInitializer& ObjectInitializer) : Super(
	ObjectInitializer)
{
	NodeName = "MontageRatio";
	FlowAbortMode = EBTFlowAbortMode::LowerPriority;

	// Must or Use NodeMemory
	bCreateNodeInstance = true;

	bNotifyBecomeRelevant = true;
	bNotifyCeaseRelevant = true;
}

bool UBTDecorator_MontageRatio::CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const
{
	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MyController && MyController->GetPawn())
	{
		AThirdPersonMPCharacter* const MyCharacter = Cast<AThirdPersonMPCharacter>(MyController->GetPawn());
		if (MyCharacter)
		{
			FCharacterAnimNotifyDelegateInfo* Info = MyCharacter->OnAnimNotifyHandles.Find(DelegateHandle);
			if (Info && Info->bIsTriggered)
			{
				return true;
			}
		}
	}

	return false;
}

void UBTDecorator_MontageRatio::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MyController && MyController->GetPawn())
	{
		AThirdPersonMPCharacter* const MyCharacter = Cast<AThirdPersonMPCharacter>(MyController->GetPawn());
		if (MyCharacter)
		{
			FCharacterBTDelegate BTDelegate;
			DelegateHandle = BTDelegate.AddUObject(this, &UBTDecorator_MontageRatio::OnAnimNotify);

			FCharacterAnimNotifyDelegateInfo Info;
			Info.Handle = DelegateHandle;
			Info.BTDelegate = BTDelegate;
			Info.Ratio = Ratio;
			Info.bIsTriggered = false;
			MyCharacter->OnAnimNotifyHandles.Add(DelegateHandle, Info);
		}
	}
}

void UBTDecorator_MontageRatio::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* const MyController = OwnerComp.GetAIOwner();
	if (MyController && MyController->GetPawn())
	{
		AThirdPersonMPCharacter* const MyCharacter = Cast<AThirdPersonMPCharacter>(MyController->GetPawn());
		if (MyCharacter)
		{
			MyCharacter->OnAnimNotifyHandles.Remove(DelegateHandle);
		}
	}
}

void UBTDecorator_MontageRatio::OnAnimNotify(UBehaviorTreeComponent& BehaviorComp)
{
	BehaviorComp.RequestExecution(this);
}
