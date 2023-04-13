// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "UObject/Object.h"
#include "BTService_PlayerCharacterStates.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UBTService_PlayerCharacterStates : public UBTService
{
	GENERATED_BODY()

	UBTService_PlayerCharacterStates(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	
	/** allows adding random time to wait time */
	UPROPERTY(Category = Debug, EditAnywhere)
	bool EnableLog = false;
};
