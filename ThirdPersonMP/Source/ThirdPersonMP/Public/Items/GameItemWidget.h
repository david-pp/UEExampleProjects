// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameItemAsset.h"
#include "Blueprint/UserWidget.h"
#include "UObject/Object.h"
#include "GameItemWidget.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UGameItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UGameItemAsset* ItemAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UGameItemAsset> ItemAsset2;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FSoftObjectPath ItemAsset3;
};
