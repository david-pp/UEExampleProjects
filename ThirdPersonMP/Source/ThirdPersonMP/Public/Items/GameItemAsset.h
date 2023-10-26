// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameItemAsset.generated.h"

UCLASS(EditInlineNew, DefaultToInstanced, CollapseCategories, Abstract)
class UGameItemFragment : public UObject
{
	GENERATED_BODY()
public:
};

UCLASS(Blueprintable)
class UGameItemFragment_Stats : public UGameItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Stats)
	float Health;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Stats)
	float Mana;
};


UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced, CollapseCategories)
class UGameItemSkill : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Skill)
	FGameplayTag SkillTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Skill)
	FText SkillDescription;
};


UCLASS(Blueprintable)
class UGameItemFragment_Skills : public UGameItemFragment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SkillPoints)
	float SkillPonts;

	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category=ActiveSkill)
	UGameItemSkill* ActiveSkill;
	
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadWrite, Category=PassiveSkills)
	TArray<UGameItemSkill*> PassiveSkills;
};

/**
 * 
 */
UCLASS(BlueprintType)
class THIRDPERSONMP_API UGameItemAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** Static types for items */
	static const FPrimaryAssetType SimpleItemType;
	static const FPrimaryAssetType TokenItemType;
	static const FPrimaryAssetType WeaponItemType;

public:
	/** Constructor */
	UGameItemAsset() : Price(0), MaxCount(1), MaxLevel(1)
	{
	}

	/** Type of this item, set in native parent class */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Item)
	FPrimaryAssetType ItemType;

	/** User-visible short name */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	FText ItemName;

	/** User-visible long description */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	FText ItemDescription;

	/** Icon to display */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Item)
	FSlateBrush ItemIcon;

	/** Price in game */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, AssetRegistrySearchable, Category = Item)
	int32 Price;

	/** Maximum number of instances that can be in inventory at once, <= 0 means infinite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Max)
	int32 MaxCount;

	/** Returns if the item is consumable (MaxCount <= 0)*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Max)
	bool IsConsumable() const;

	/** Maximum level this item can be, <= 0 means infinite */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Max)
	int32 MaxLevel;

	/** Returns the logical name, equivalent to the primary asset id */
	UFUNCTION(BlueprintCallable, Category = Item)
	FString GetIdentifierString() const;

	/** Overridden to use saved type */
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;


	
	UPROPERTY(Instanced, EditAnywhere, BlueprintReadOnly, Category = Fragments)
	TArray<UGameItemFragment*> Fragments;

	UFUNCTION(BlueprintCallable, Category = Item)
	const UGameItemFragment* FindFragmentByClass(TSubclassOf<UGameItemFragment> FragmentClass) const;
};
