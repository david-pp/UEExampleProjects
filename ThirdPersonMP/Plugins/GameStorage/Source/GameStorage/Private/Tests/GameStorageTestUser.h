// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameStorageTestUser.generated.h"

UENUM()
enum class EGameStorageTestUserSexType : int32
{
	Unknown,
	Male,
	Female,
};

inline bool LexTryParseString(EGameStorageTestUserSexType& OutType, const TCHAR* Text)
{
	if (FCString::Strcmp(Text, TEXT("Male")) == 0)
	{
		OutType = EGameStorageTestUserSexType::Male;
		return true;
	}
	else if (FCString::Strcmp(Text, TEXT("Female")) == 0)
	{
		OutType = EGameStorageTestUserSexType::Female;
		return true;
	}
	OutType = EGameStorageTestUserSexType::Unknown;
	return false;
}

inline const TCHAR* LexToString(EGameStorageTestUserSexType Type)
{
	switch (Type)
	{
	case EGameStorageTestUserSexType::Male:
		return TEXT("Male");
	case EGameStorageTestUserSexType::Female:
		return TEXT("Female");
	default:
		return TEXT("Unknown");
	}
}

UCLASS()
class UGameStorageTestUserProfile : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString Email;
	UPROPERTY()
	FString Phone;
};

UCLASS()
class UGameStorageTestUserItem : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString ItemName;
	UPROPERTY()
	int32 ItemCount;
};

/**
 * 
 */
UCLASS()
class GAMESTORAGE_API UGameStorageTestUser : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FString UserName;
	UPROPERTY()
	int32 UserAge;
	UPROPERTY()
	EGameStorageTestUserSexType UserSex;

	UPROPERTY(Transient)
	UGameStorageTestUserProfile* Profile;

	UPROPERTY(Transient)
	TArray<UGameStorageTestUserItem*> Items;
};
