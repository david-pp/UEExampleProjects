// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameStoragePath.generated.h"

/**
 * Game Store Key for Query/Update/... (Type + ID )
 */
USTRUCT()
struct FGameStorageKey
{
	GENERATED_BODY()

	FGameStorageKey()
	{
	}

	FGameStorageKey(const FString& InType, const FString& InId) : Type(InType), Id(InId)
	{
	}

	bool IsValid() const
	{
		return Type.Len() > 0 && Id.Len() > 0;
	}

	FString ToString() const;

	bool ParseFromString(const FString& KeyString);

	/** Entity Type */
	UPROPERTY()
	FString Type;
	/** Entity ID (optional) */
	UPROPERTY()
	FString Id;
};

/**
 * Game Storage Path
 *
 * [Examples]
 *  - user:01
 *  - user:01/profile
 *  - user:01/character:001
 *  - user:01/character:002
 *  - game/news
 *  - game/title
 */
USTRUCT()
struct FGameStoragePath
{
	GENERATED_BODY()

public:
	FGameStoragePath();
	FGameStoragePath(FString InPath);

	void SetPath(const FString& InPath);
	void AppendPath(const FString& InPath);
	const FString& GetPath() const;
	bool IsValidPath() const;

	bool ParseEntityKeys(TArray<FGameStorageKey>& OutKeys) const;
	uint32 ParsePathTokens(TArray<FString>& OutPathTokens) const;

	/**
	 * Determines if the path is /
	 */
	bool IsRoot() const;

	/**
	 * Re-path this path sans the path of another
	 * MakeRelative(/a/b/c/d, /a/b) => /c/d  
	 *
	 * @param OtherPath The path to make relative against
	 */
	void MakeRelative(const FString& OtherPath);

	// TMap<> Comparer
	bool operator==(const FGameStoragePath& Other) const
	{
		bool bEqual = (0 == Path.Compare(Other.Path, ESearchCase::CaseSensitive));
		return bEqual;
	}

	friend uint32 GetTypeHash(const FGameStoragePath& InPath)
	{
		return GetTypeHash(InPath.GetPath());
	}

	FString ToRedisKey() const;
	FString ToFilePath(const FString& Ext = "") const;
	FString ToFlatFilePath() const;
	FString ToRESTFulPath() const;

private:
	void NormalizePath();

	/** The entity path string */
	FString Path = TEXT("/");
};
