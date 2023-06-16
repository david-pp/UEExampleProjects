// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "UObject/Object.h"
#include "MMC_TestValue.generated.h"

/**
 * 
 */
UCLASS()
class GASDOCUMENTATION_API UMMC_TestValue : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	UMMC_TestValue();
	
	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;
};
