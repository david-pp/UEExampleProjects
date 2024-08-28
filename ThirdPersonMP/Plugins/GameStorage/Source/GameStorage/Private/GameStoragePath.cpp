// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStoragePath.h"

// ------------ FGameStoragePath ---------------- 

FString FGameStorageKey::ToString() const
{
	if (Id.IsEmpty())
	{
		return FString::Printf(TEXT("%s"), *Type);
	}
	else
	{
		return FString::Printf(TEXT("%s:%s"), *Type, *Id);
	}
}

bool FGameStorageKey::ParseFromString(const FString& KeyString)
{
	if (KeyString.IsEmpty()) return false;

	TArray<FString> OutTokens;
	KeyString.ParseIntoArray(OutTokens, TEXT(":"), true);
	if (OutTokens.Num() > 0) Type = OutTokens[0];
	if (OutTokens.Num() > 1) Id = OutTokens[1];
	return true;
}

// ------------ FGameStoragePath ---------------- 

FGameStoragePath::FGameStoragePath()
{
}

FGameStoragePath::FGameStoragePath(FString InPath)
	: Path(MoveTemp(InPath))
{
	NormalizePath();
}

const FString& FGameStoragePath::GetPath() const
{
	return Path;
}

bool FGameStoragePath::ParseEntityKeys(TArray<FGameStorageKey>& OutKeys) const
{
	TArray<FString> KeyStrings;
	Path.ParseIntoArray(KeyStrings, TEXT("/"), true);

	for (FString& KeyString : KeyStrings)
	{
		FGameStorageKey Key;
		if (Key.ParseFromString(KeyString))
		{
			OutKeys.Add(Key);
		}
	}
	return true;
}

uint32 FGameStoragePath::ParsePathTokens(TArray<FString>& OutPathTokens) const
{
	return Path.ParseIntoArray(OutPathTokens, TEXT("/"), true);
}

void FGameStoragePath::SetPath(const FString& NewPath)
{
	Path = NewPath;
	NormalizePath();
}

void FGameStoragePath::AppendPath(const FString& InPath)
{
	Path /= InPath;
	NormalizePath();
}

bool FGameStoragePath::IsValidPath() const
{
	// if (IsRoot())
	// {
	// 	return false;
	// }
	// if (!Path.StartsWith(TEXT("/")))
	// {
	// 	return false;
	// }

	auto IsInvalidUriChar = [](TCHAR C)
	{
		return (C <= 32) || (C >= 127) || (C == TEXT(' ')) || (C == TEXT('.')) || (C == TEXT(',')) || (C == TEXT('<')) || (C == TEXT('>')) || (C == TEXT(']')) || (C == TEXT('[')) || (C == TEXT('}')) || (C == TEXT('{')) || (C == TEXT('#')) || (C == TEXT('|')) || (C == TEXT('^')) || (C == TEXT('\\'));
	};

	return (INDEX_NONE == Path.FindLastCharByPredicate(IsInvalidUriChar));
}

bool FGameStoragePath::IsRoot() const
{
	return Path == TEXT("/");
}

void FGameStoragePath::MakeRelative(const FString& OtherPath)
{
	const bool bAllowShrinking = false;

	if (OtherPath == TEXT("/"))
	{
		return;
	}

	if (Path == OtherPath)
	{
		Path.RemoveAt(1, Path.Len() - 1, bAllowShrinking);
	}

	if (Path.StartsWith(OtherPath))
	{
		Path.RemoveAt(0, OtherPath.Len(), bAllowShrinking);
	}
}


void FGameStoragePath::NormalizePath()
{
	if (!IsRoot() && Path.EndsWith(TEXT("/")))
	{
		Path.RemoveFromEnd(TEXT("/"));
	}
}

FString FGameStoragePath::ToRedisKey() const
{
	FString Key;
	TArray<FGameStorageKey> EntityKeys;
	if (ParseEntityKeys(EntityKeys))
	{
		for (int I = 0; I < EntityKeys.Num(); ++I)
		{
			if (I > 0)
			{
				Key += TEXT(":");
			}
			Key += EntityKeys[I].ToString();
		}
	}
	return Key;
}

FString FGameStoragePath::ToFilePath(const FString& Ext) const
{
	FString FilePath;
	TArray<FGameStorageKey> EntityKeys;
	if (ParseEntityKeys(EntityKeys))
	{
		for (int I = 0; I < EntityKeys.Num(); ++I)
		{
			FGameStorageKey& Key = EntityKeys[I];
			FilePath /= Key.Type;
			if (Key.Id.Len() > 0)
			{
				FilePath /= FString::Printf(TEXT("%s_%s"), *Key.Type, *Key.Id);
			}
		}
	}

	if (!Ext.IsEmpty())
	{
		FilePath += TEXT(".");
		FilePath += Ext;
	}

	return FilePath;
}

FString FGameStoragePath::ToFlatFilePath() const
{
	FString FilePath;
	TArray<FGameStorageKey> EntityKeys;
	if (ParseEntityKeys(EntityKeys))
	{
		for (int I = 0; I < EntityKeys.Num(); ++I)
		{
			if (I > 0)
			{
				FilePath += TEXT("_");
			}

			FGameStorageKey& Key = EntityKeys[I];
			if (Key.Id.Len() > 0)
			{
				FilePath += FString::Printf(TEXT("%s_%s"), *Key.Type, *Key.Id);
			}
			else
			{
				FilePath += *Key.Type;
			}
		}
	}
	return FilePath;
}

FString FGameStoragePath::ToRESTFulPath() const
{
	FString FilePath;
	TArray<FGameStorageKey> EntityKeys;
	if (ParseEntityKeys(EntityKeys))
	{
		for (int I = 0; I < EntityKeys.Num(); ++I)
		{
			FGameStorageKey& Key = EntityKeys[I];
			FilePath /= Key.Type;
			if (Key.Id.Len() > 0)
			{
				FilePath /= Key.Id;
			}
		}
	}
	return FilePath;
}
