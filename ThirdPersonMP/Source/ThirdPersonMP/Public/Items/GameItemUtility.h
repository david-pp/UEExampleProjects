// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameItemAsset.h"
#include "UObject/Object.h"
#include "GameItemUtility.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UGameItemUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = Item)
	static UGameItemAsset* GenerateItemAsset(const FGameItemDefinition& ItemDefinition);


	UFUNCTION(BlueprintCallable, Category = Item)
	static UGameItemAsset* FindItemAssetByName(FPrimaryAssetType ItemType, FName ItemName);

	UFUNCTION(BlueprintCallable, Category = Item)
	static UGameItemAsset* FindItemAssetById(FPrimaryAssetId AssetId);
};
