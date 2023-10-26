// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameItemAsset.h"
#include "UObject/Object.h"
#include "GameTokenItemAsset.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UGameTokenItemAsset : public UGameItemAsset
{
	GENERATED_BODY()

public:
	UGameTokenItemAsset()
	{
		ItemType = UGameItemAsset::TokenItemType;
	}
};
