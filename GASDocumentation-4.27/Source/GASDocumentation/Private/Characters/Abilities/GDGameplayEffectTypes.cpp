// Copyright 2020 Dan Kestranek.


#include "Characters/Abilities/GDGameplayEffectTypes.h"

bool FGDGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	return Super::NetSerialize(Ar, Map, bOutSuccess) && TargetData.NetSerialize(Ar, Map, bOutSuccess);
}
