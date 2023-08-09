// Fill out your copyright notice in the Description page of Project Settings.


#include "ExampleArray.h"

#include "ThirdPersonMP/ThirdPersonMPCharacter.h"

void FExampleItemEntry::PreReplicatedRemove(const FExampleArray& InArraySerializer)
{
	UE_LOG(LogTemp, Error, TEXT("PreRemove: %d"), ExampleIntProperty);
	AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(InArraySerializer.OwnerCharacter);
	if (Character)
	{
		Character->DumpExampleArray();
	}
}

void FExampleItemEntry::PostReplicatedAdd(const FExampleArray& InArraySerializer)
{
	UE_LOG(LogTemp, Error, TEXT("PostAdd: %d"), ExampleIntProperty);

	AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(InArraySerializer.OwnerCharacter);
	if (Character)
	{
		Character->DumpExampleArray();
	}
}

void FExampleItemEntry::PostReplicatedChange(const FExampleArray& InArraySerializer)
{
	UE_LOG(LogTemp, Error, TEXT("PostChange: %d"), ExampleIntProperty);

	AThirdPersonMPCharacter* Character = Cast<AThirdPersonMPCharacter>(InArraySerializer.OwnerCharacter);
	if (Character)
	{
		Character->DumpExampleArray();
	}
}

FString FExampleItemEntry::GetDebugString()
{
	return TEXT("ExampleItem");
}
