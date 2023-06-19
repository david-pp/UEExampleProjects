// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GDBlueprintLibrary.generated.h"


/**
 * 
 */
UCLASS()
class GASDOCUMENTATION_API UGDBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FGameplayAbilityTargetDataHandle MakeTargetDataHandleByLocationInfo(const FGameplayAbilityTargetingLocationInfo& Source,
		const FGameplayAbilityTargetingLocationInfo& Target);
	
	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static FGameplayAbilityTargetDataHandle MakeCustomTargetDataHandle(float Valule);

	UFUNCTION(BlueprintPure, Category = "Ability|TargetData")
	static float GetCustomTargetValueFromTargetData(const FGameplayAbilityTargetDataHandle& TargetData);
	
	UFUNCTION(BlueprintCallable, Category="Editor")
	static UGameplayEffect* GenerateGameEffects(TSubclassOf<UGameplayEffect> Class, FName EffectName, FGameplayTag AssetTag);
};
