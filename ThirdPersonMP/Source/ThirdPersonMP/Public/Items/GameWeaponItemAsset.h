// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameItemAsset.h"
#include "UObject/Object.h"
#include "GameWeaponItemAsset.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UGameWeaponItemAsset : public UGameItemAsset
{
	GENERATED_BODY()

public:
	UGameWeaponItemAsset()
	{
		ItemType = UGameItemAsset::WeaponItemType;
	}
};
