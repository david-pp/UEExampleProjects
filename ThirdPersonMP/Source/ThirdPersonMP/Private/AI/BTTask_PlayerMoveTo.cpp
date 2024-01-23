// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_PlayerMoveTo.h"

#include "AIController.h"
#include "AI/AIFunctionLibrary.h"
#include "GameFramework/Character.h"


UBTTask_PlayerRotation::UBTTask_PlayerRotation(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "PlayerRotation";
	WaitTime = 5.0f;
	bNotifyTick = true;
}

EBTNodeResult::Type UBTTask_PlayerRotation::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTWaitTaskMemory* MyMemory = (FBTWaitTaskMemory*)NodeMemory;
	MyMemory->RemainingWaitTime = FMath::FRandRange(FMath::Max(0.0f, WaitTime - RandomDeviation), (WaitTime + RandomDeviation));


	return EBTNodeResult::InProgress;
}

void UBTTask_PlayerRotation::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTWaitTaskMemory* MyMemory = (FBTWaitTaskMemory*)NodeMemory;
	MyMemory->RemainingWaitTime -= DeltaSeconds;

	ACharacter* Character = UAIFunctionLibrary::GetBTCompOwnerCharacter(&OwnerComp);
	if (Character)
	{
		FRotator Rotator(0.f, RotateSpeed, 0);
		Character->AddActorLocalRotation(Rotator);
	}

	if (MyMemory->RemainingWaitTime <= 0.0f)
	{
		// continue execution from this node
		FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
	}
}

FString UBTTask_PlayerRotation::GetStaticDescription() const
{
	if (FMath::IsNearlyZero(RandomDeviation))
	{
		return FString::Printf(TEXT("%s: %.1fs"), *Super::GetStaticDescription(), WaitTime);
	}
	else
	{
		return FString::Printf(TEXT("%s: %.1f+-%.1fs"), *Super::GetStaticDescription(), WaitTime, RandomDeviation);
	}
}
