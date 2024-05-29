#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

class APlayerController;

class FGameplayDebuggerCategory_Generic : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_Generic();
	void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext);

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
