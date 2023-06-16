// Copyright 2020 Dan Kestranek.


#include "GDBlueprintLibrary.h"

#include "Characters/Abilities/GDAbilitySystemComponent.h"

FGameplayAbilityTargetDataHandle UGDBlueprintLibrary::MakeTargetDataHandleByLocationInfo(const FGameplayAbilityTargetingLocationInfo& Source, const FGameplayAbilityTargetingLocationInfo& Target)
{
	FGameplayAbilityTargetDataHandle TargetDataHandle;

	FGameplayAbilityTargetData_LocationInfo* LocationInfo = new FGameplayAbilityTargetData_LocationInfo;
	LocationInfo->SourceLocation = Source;
	LocationInfo->TargetLocation = Target;
	TargetDataHandle.Add(LocationInfo);
	return TargetDataHandle;
}

FGameplayAbilityTargetDataHandle UGDBlueprintLibrary::MakeCustomTargetDataHandle(float Valule)
{
	FGameplayAbilityTargetDataHandle TargetDataHandle;

	FGameplayAbilityTargetData_Custom* Custom = new FGameplayAbilityTargetData_Custom;
	Custom->Value = Valule;

	TargetDataHandle.Add(Custom);
	return TargetDataHandle;
}

float UGDBlueprintLibrary::GetCustomTargetValueFromTargetData(const FGameplayAbilityTargetDataHandle& TargetData)
{
	int Index = 0;
	if (TargetData.Data.IsValidIndex(Index))
	{
		FGameplayAbilityTargetData* Data = TargetData.Data[Index].Get();

		FGameplayAbilityTargetData_Custom* Custom = static_cast<FGameplayAbilityTargetData_Custom*>(Data);
		if (Custom)
		{
			return Custom->Value;
		}
	}
	return 0.0f;
}
