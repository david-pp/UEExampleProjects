// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayDebuggerCategory.h"

class FGameplayDebuggerCategory_ThirdMP : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_ThirdMP();
	virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

	static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
	
protected:
	struct FRepData
	{
		// Put all data you want to display here
		FString ActorName;

		void Serialize(FArchive& Ar);
	};

	FRepData DataPack;
};


#endif
