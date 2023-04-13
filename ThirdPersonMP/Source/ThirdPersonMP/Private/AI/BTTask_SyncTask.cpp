// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/BTTask_SyncTask.h"

#include "Kismet/KismetSystemLibrary.h"

UBTTask_SyncTask::UBTTask_SyncTask(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "SyncTask";
	bNotifyTick = true;

	WaitTime = 5.0f;
	Result = EBTNodeResult::Succeeded;
}

EBTNodeResult::Type UBTTask_SyncTask::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTSyncTaskMemory* MyMemory = (FBTSyncTaskMemory*)NodeMemory;
	MyMemory->RemainingWaitTime = WaitTime;
	MyMemory->ExcuteFrame = UKismetSystemLibrary::GetFrameCount();

	if (EnableLog)
	{
		UE_LOG(LogTemp, Display, TEXT("SyncTask: %s, %lld"), *GetNodeName(), MyMemory->ExcuteFrame);
		// UE_LOG(LogTemp, Display, TEXT("SyncTask: %s, %lld"), *Super::GetStaticDescription(), MyMemory->ExcuteFrame);
	}
	
	if (FMath::IsNearlyZero(WaitTime))
	{
		return Result;
	}
	return EBTNodeResult::InProgress;
}

uint16 UBTTask_SyncTask::GetInstanceMemorySize() const
{
	return sizeof(FBTSyncTaskMemory);
}

void UBTTask_SyncTask::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	FBTSyncTaskMemory* MyMemory = (FBTSyncTaskMemory*)NodeMemory;
	if (MyMemory->RemainingWaitTime)
	{
		Values.Add(FString::Printf(TEXT("remaining: %ss"), *FString::SanitizeFloat(MyMemory->RemainingWaitTime)));
		Values.Add(FString::Printf(TEXT("frame: %lld"), MyMemory->ExcuteFrame));
	}
}

FString UBTTask_SyncTask::GetStaticDescription() const
{
	return FString::Printf(TEXT("%s %s"), *Super::GetStaticDescription(), *UBehaviorTreeTypes::DescribeNodeResult(Result.GetValue()));
}

void UBTTask_SyncTask::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTSyncTaskMemory* MyMemory = (FBTSyncTaskMemory*)NodeMemory;
	MyMemory->RemainingWaitTime -= DeltaSeconds;

	if (MyMemory->RemainingWaitTime <= 0.0f)
	{
		// continue execution from this node
		FinishLatentTask(OwnerComp, Result);
	}
}

EBTNodeResult::Type UBTTask_SyncTask::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTSyncTaskMemory* MyMemory = (FBTSyncTaskMemory*)NodeMemory;
	UE_LOG(LogTemp, Display, TEXT("AbortSyncTask: %s"), *GetNodeName());
	return Super::AbortTask(OwnerComp, NodeMemory);
}

#if WITH_EDITOR

FName UBTTask_SyncTask::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.Wait.Icon");
}

#endif	// WITH_EDITOR
