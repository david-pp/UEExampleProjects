// Fill out your copyright notice in the Description page of Project Settings.


#include "Items/GameItemUtility.h"

#include "Engine/AssetManager.h"

UGameItemAsset* UGameItemUtility::GenerateItemAsset(const FGameItemDefinition& ItemDefinition)
{
	return nullptr;
}

UGameItemAsset* UGameItemUtility::FindItemAssetByName(FPrimaryAssetType ItemType, FName ItemName)
{
	const UAssetManager* AssetManager = UAssetManager::GetIfValid();
	if (AssetManager)
	{
		TArray<FPrimaryAssetId> AssetIdList;
		AssetManager->GetPrimaryAssetIdList(ItemType, AssetIdList);
		for (FPrimaryAssetId& AssetId : AssetIdList)
		{
			UGameItemAsset* ItemAsset = AssetManager->GetPrimaryAssetObject<UGameItemAsset>(AssetId);
			if (ItemAsset && ItemAsset->ItemName.EqualTo(FText::FromName(ItemName)))
			{
				return ItemAsset;
			}
		}
	}
	
	return nullptr;
}

UGameItemAsset* UGameItemUtility::FindItemAssetById(FPrimaryAssetId AssetId)
{
	if (AssetId.IsValid())
	{
		const UAssetManager* AssetManager = UAssetManager::GetIfValid();
		if (AssetManager)
		{
			return AssetManager->GetPrimaryAssetObject<UGameItemAsset>(AssetId);
		}

		// return Cast<UGameItemAsset>(UKismetSystemLibrary::GetObjectFromPrimaryAssetId(AssetId));
	}

	return nullptr;
}
