// Fill out your copyright notice in the Description page of Project Settings.


#include "PrimaryData.h"

const FPrimaryAssetType PrimaryDataType = FName(TEXT("Data"));

FPrimaryAssetId UPrimaryData::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(PrimaryDataType, DataName);
}
