// Copyright 2020 Dan Kestranek.


#include "Characters/Abilities/MMC_TestValue.h"

#include "Characters/Abilities/AttributeSets/GDAttributeSetBase.h"

static FGameplayEffectAttributeCaptureDefinition HealthDef;
static FGameplayEffectAttributeCaptureDefinition ManaDef;

UMMC_TestValue::UMMC_TestValue()
{
	HealthDef.AttributeToCapture = UGDAttributeSetBase::GetHealthAttribute();
	HealthDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	HealthDef.bSnapshot = false;

	ManaDef.AttributeToCapture = UGDAttributeSetBase::GetHealthAttribute();
	ManaDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Source;
	ManaDef.bSnapshot = true;

	RelevantAttributesToCapture.Add(HealthDef);
	RelevantAttributesToCapture.Add(ManaDef);
}

float UMMC_TestValue::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Health = 0.0f;
	GetCapturedAttributeMagnitude(HealthDef, Spec, EvaluateParameters, Health);

	float Mana = 0.0f;
	GetCapturedAttributeMagnitude(ManaDef, Spec, EvaluateParameters, Mana);

	// TODO: Clamp Health/Mana
	return Health + Mana;
}
