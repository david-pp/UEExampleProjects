#include "GameplayDebuggerCategory_Generic.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"

FGameplayDebuggerCategory_Generic::FGameplayDebuggerCategory_Generic()
{
	SetDataPackReplication(&DataPack);
}

void FGameplayDebuggerCategory_Generic::CollectData(APlayerController* OwnerPC, AActor* DebugActor)
{
	if (OwnerPC && DebugActor)
	{
		FString LocalRoleString = UEnum::GetValueAsString(TEXT("Engine.ENetRole"), DebugActor->GetLocalRole());
		DataPack.ActorName = FString::Printf(TEXT("%s@%s"), *DebugActor->GetName(), *LocalRoleString);
	}
}

void FGameplayDebuggerCategory_Generic::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (!DataPack.ActorName.IsEmpty())
	{
		CanvasContext.Printf(TEXT("{yellow}Actor Name: {white}:%s"), *DataPack.ActorName);
	}
}

TSharedRef<FGameplayDebuggerCategory> FGameplayDebuggerCategory_Generic::MakeInstance()
{
	return MakeShared<FGameplayDebuggerCategory_Generic>();
}

void FGameplayDebuggerCategory_Generic::FRepData::Serialize(FArchive& Ar)
{
	Ar << ActorName;
}

#endif // WITH_GAMEPLAY_DEBUGGER
