// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_MoveTo.h"
#include "BehaviorTree/Tasks/BTTask_Wait.h"
#include "UObject/Object.h"
#include "BTTask_PlayerMoveTo.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UBTTask_PlayerMoveTo : public UBTTask_MoveTo
{
	GENERATED_BODY()
};


UCLASS()
class THIRDPERSONMP_API UBTTask_PlayerRotation : public UBTTask_Wait
{
	GENERATED_BODY()

	UBTTask_PlayerRotation(const FObjectInitializer& ObjectInitializer);
	
public:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual FString GetStaticDescription() const override;

	UPROPERTY(Category = Player, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RotateSpeed = 1.0f;
};
