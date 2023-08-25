// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "PrimaryData.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UPrimaryData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName DataName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int DataValue;
};
