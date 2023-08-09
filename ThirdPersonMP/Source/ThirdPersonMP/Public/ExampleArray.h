// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/NetSerialization.h"
#include "ExampleArray.generated.h"

/** Step 1: Make your struct inherit from FFastArraySerializerItem */
USTRUCT(BlueprintType)
struct FExampleItemEntry : public FFastArraySerializerItem
{
	GENERATED_USTRUCT_BODY()

	// Your data:
	UPROPERTY(BlueprintReadWrite, Category=Example)
	int32 ExampleIntProperty;

	UPROPERTY(BlueprintReadWrite, Category=Example)
	float ExampleFloatProperty;

	UPROPERTY(BlueprintReadWrite, Category=Example)
	FString ExampleString;

	/** 
	 * Optional functions you can implement for client side notification of changes to items; 
	 * Parameter type can match the type passed as the 2nd template parameter in associated call to FastArrayDeltaSerialize
	 * 
	 * NOTE: It is not safe to modify the contents of the array serializer within these functions, nor to rely on the contents of the array 
	 * being entirely up-to-date as these functions are called on items individually as they are updated, and so may be called in the middle of a mass update.
	 */
	void PreReplicatedRemove(const struct FExampleArray& InArraySerializer);
	void PostReplicatedAdd(const struct FExampleArray& InArraySerializer);
	void PostReplicatedChange(const struct FExampleArray& InArraySerializer);

	// Optional: debug string used with LogNetFastTArray logging
	FString GetDebugString();
};

/** Step 2: You MUST wrap your TArray in another struct that inherits from FFastArraySerializer */
USTRUCT()
struct FExampleArray : public FFastArraySerializer
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<FExampleItemEntry> Items; /** Step 3: You MUST have a TArray named Items of the struct you made in step 1. */

	UPROPERTY(NotReplicated)
	ACharacter* OwnerCharacter;
	
	/** Step 4: Copy this, replace example with your names */
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FExampleItemEntry, FExampleArray>(Items, DeltaParms, *this);
	}
};

/** Step 5: Copy and paste this struct trait, replacing FExampleArray with your Step 2 struct. */
template <>
struct TStructOpsTypeTraits<FExampleArray> : public TStructOpsTypeTraitsBase2<FExampleArray>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
