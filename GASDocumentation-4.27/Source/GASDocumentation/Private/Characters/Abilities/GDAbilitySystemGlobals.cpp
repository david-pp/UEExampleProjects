// Copyright 2020 Dan Kestranek.


#include "Characters/Abilities/GDAbilitySystemGlobals.h"

FGameplayEffectContext* UGDAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FGameplayEffectContext();
}
