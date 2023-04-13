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

	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;


	UPROPERTY(EditAnywhere, Category=ThirdMP)
	UAnimMontage* MontageToPlay;

	UPROPERTY(EditAnywhere, Category=ThirdMP)
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, Category=ThirdMP)
	FName StartSectionName = NAME_None;

	/** if true the task will just trigger the animation and instantly finish. Fire and Forget. */
	UPROPERTY(Category = ThirdMP, EditAnywhere)
	uint32 bNonBlocking : 1;

	UPROPERTY(Category = ThirdMP, EditAnywhere)
	uint32 bNonStopMontage : 1;

public:
	void OnMontageBlendingOut(UAnimMontage* Montage, bool bInterrupted);

	void OnMontageInterrupted();

	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);


	UPROPERTY()
	UBehaviorTreeComponent* MyOwnerComp;

	UPROPERTY()
	USkeletalMeshComponent* CachedSkelMesh;

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	FOnMontageEnded MontageEndedDelegate;
	FDelegateHandle InterruptedHandle;


	bool StopPlayingMontage();
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


	UPROPERTY(Category = ThirdPersonMP, EditAnywhere, meta = (ClampMin = "0.0", UIMin = "0.0"))
	float RatioMin = 0.f;

	UPROPERTY(Category = ThirdPersonMP, EditAnywhere, meta = (ClampMax = "1.0", UIMax = "1.0"))
	float RatioMax = 1.0f;
};

UCLASS()
class THIRDPERSONMP_API UBTDecorator_MontageRatioCheck : public UBTDecorator
{
	GENERATED_BODY()

	UBTDecorator_MontageRatioCheck(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	UPROPERTY(Category = ThirdPersonMP, EditAnywhere, meta = (ClampMax = "1.0", UIMax = "1.0"))
	float Ratio = 1.0f;

	float LastAnimRatio = 0.0f;
};


UCLASS()
class THIRDPERSONMP_API UBTDecorator_MontageRatio : public UBTDecorator
{
	GENERATED_BODY()

	UBTDecorator_MontageRatio(const FObjectInitializer& ObjectInitializer);

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	/** called when execution flow controller becomes active */
	virtual void OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** called when execution flow controller becomes inactive */
	virtual void OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	void OnAnimNotify(UBehaviorTreeComponent& OwnerComp);

	UPROPERTY(Category = ThirdPersonMP, EditAnywhere, meta = (ClampMax = "1.0", UIMax = "1.0"))
	float Ratio = 1.0f;

protected:
	FDelegateHandle DelegateHandle;
};
