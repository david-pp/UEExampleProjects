// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "UObject/Object.h"
#include "BTTask_SyncTask.generated.h"


struct FBTSyncTaskMemory
{
	/** time left */
	float RemainingWaitTime;

	int64 ExcuteFrame;
};

UCLASS()
class UBTTask_SyncTask : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SyncTask(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual uint16 GetInstanceMemorySize() const override;
	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override;
	virtual FString GetStaticDescription() const override;

protected:
#if WITH_EDITOR
	virtual FName GetNodeIconName() const override;
#endif // WITH_EDITOR
	
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	
	/** allows adding random time to wait time */
	UPROPERTY(Category = Result, EditAnywhere)
	TEnumAsByte<EBTNodeResult::Type> Result;
	
	/** wait time in seconds */
	UPROPERTY(Category = Wait, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float WaitTime;


	/** allows adding random time to wait time */
	UPROPERTY(Category = Debug, EditAnywhere)
	bool EnableLog = false;
};
