// Fill out your copyright notice in the Description page of Project Settings.

#include "ThirdMPGameplayDebugger.h"

#if WITH_GAMEPLAY_DEBUGGER

FGameplayDebuggerCategory_ThirdMP::FGameplayDebuggerCategory_ThirdMP()
{
	SetDataPackReplication(&DataPack);
}

void FGameplayDebuggerCategory_ThirdMP::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (OwnerPC)
	{
		DataPack.ActorName = OwnerPC->GetPawn()->GetName();
	}
}

void FGameplayDebuggerCategory_ThirdMP::DrawData(APlayerController* OwnerPC,
                                                 FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (!DataPack.ActorName.IsEmpty())
	{
		CanvasContext.Printf(TEXT("{yellow}Actor name: {white}%s"), *DataPack.ActorName);
	}
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_ThirdMP::MakeInstance()
{
	return MakeShareable(new FGameplayDebuggerCategory_ThirdMP);
}

void FGameplayDebuggerCategory_ThirdMP::FRepData::Serialize(FArchive& Ar)
{
	Ar << ActorName;
}

#endif
