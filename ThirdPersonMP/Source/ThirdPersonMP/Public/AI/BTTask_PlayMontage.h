// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UObject/Object.h"
#include "BTTask_PlayMontage.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UBTTask_PlayMontage : public UBTTaskNode
{
	GENERATED_BODY()
	UBTTask_PlayMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(EditAnywhere, Category=ThirdMP)
	UAnimMontage* MontageToPlay;

	UPROPERTY(EditAnywhere, Category=ThirdMP)
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, Category=ThirdMP)
	FName StartSectionName = NAME_None;
};


UCLASS()
class THIRDPERSONMP_API UBTTask_StopMontage : public UBTTaskNode
{
	GENERATED_BODY()
	UBTTask_StopMontage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};


UCLASS()
class THIRDPERSONMP_API UBTDecorator_MontageRatioRange : public UBTDecorator
{
	GENERATED_BODY()

	UBTDecorator_MontageRatioRange(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** called when execution flow controller becomes active */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** called when execution flow controller becomes inactive */
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	UPROPERTY(Category = ThirdPersonMP, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RatioMin = 0.f;

	UPROPERTY(Category = ThirdPersonMP, EditAnywhere, meta = (ClampMax = "1.0", UIMax = "1.0"))
	float RatioMax = 1.0f;
};
