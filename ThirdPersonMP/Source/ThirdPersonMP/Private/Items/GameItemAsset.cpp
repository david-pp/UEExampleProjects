// Fill out your copyright notice in the Description page of Project Settings.

#include "Items/GameItemAsset.h"

const FPrimaryAssetType	UGameItemAsset::SimpleItemType = TEXT("Simple");
const FPrimaryAssetType	UGameItemAsset::TokenItemType = TEXT("Token");
const FPrimaryAssetType	UGameItemAsset::WeaponItemType = TEXT("Weapon");

bool UGameItemAsset::IsConsumable() const
{
	if (MaxCount <= 0)
	{
		return true;
	}
	return false;
}

FString UGameItemAsset::GetIdentifierString() const
{
	return GetPrimaryAssetId().ToString();
}

FPrimaryAssetId UGameItemAsset::GetPrimaryAssetId() const
{
	// This is a DataAsset and not a blueprint so we can just use the raw FName
	// For blueprints you need to handle stripping the _C suffix
	return FPrimaryAssetId(ItemType, GetFName());
}

const UGameItemFragment* UGameItemAsset::FindFragmentByClass(TSubclassOf<UGameItemFragment> FragmentClass) const
{
	if (FragmentClass)
	{
		for (auto Fragement : Fragments)
		{
			if (Fragement->IsA(FragmentClass))
			{
				return Fragement;
			}
		}
	}
	return nullptr;
}
