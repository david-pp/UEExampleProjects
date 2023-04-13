// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "UObject/Object.h"
#include "BTDecorator_IsPlayerCharacterMoving.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UBTDecorator_IsPlayerCharacterMoving : public UBTDecorator
{
	GENERATED_BODY()

	UBTDecorator_IsPlayerCharacterMoving(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;
	/** called when execution flow controller becomes active */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** called when execution flow controller becomes inactive */
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;


	void OnMovingChange(UBehaviorTreeComponent& OwnerComp);
	
	FDelegateHandle DelegateHandle;
};
