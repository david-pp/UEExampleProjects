// Fill out your copyright notice in the Description page of Project Settings.


#include "GameStoragePath.h"


FGameEntityStoragePath::FGameEntityStoragePath()
{
}

FGameEntityStoragePath::FGameEntityStoragePath(FString InPath)
	: Path(MoveTemp(InPath))
{
	NormalizePath();
}

const FString& FGameEntityStoragePath::GetPath() const
{
	return Path;
}

bool FGameEntityStoragePath::ParseEntityKeys(TArray<FGameEntityStorageKey>& OutKeys) const
{
	TArray<FString> KeyStrings;
	Path.ParseIntoArray(KeyStrings, TEXT("/"), true);

	for (FString& KeyString : KeyStrings)
	{
		FGameEntityStorageKey Key;
		if (Key.ParseFromString(KeyString))
		{
			OutKeys.Add(Key);
		}
	}
	return true;
}

uint32 FGameEntityStoragePath::ParsePathTokens(TArray<FString>& OutPathTokens) const
{
	return Path.ParseIntoArray(OutPathTokens, TEXT("/"), true);
}

void FGameEntityStoragePath::SetPath(const FString& NewPath)
{
	Path = NewPath;
	NormalizePath();
}

void FGameEntityStoragePath::AppendPath(const FString& InPath)
{
	Path /= InPath;
	NormalizePath();
}

bool FGameEntityStoragePath::IsValidPath() const
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

bool FGameEntityStoragePath::IsRoot() const
{
	return Path == TEXT("/");
}

void FGameEntityStoragePath::MakeRelative(const FString& OtherPath)
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


void FGameEntityStoragePath::NormalizePath()
{
	if (!IsRoot() && Path.EndsWith(TEXT("/")))
	{
		Path.RemoveFromEnd(TEXT("/"));
	}
}

FString FGameEntityStoragePath::ToRedisKey() const
{
	FString Key;
	TArray<FGameEntityStorageKey> EntityKeys;
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

FString FGameEntityStoragePath::ToFilePath(const FString& Ext) const
{
	FString FilePath;
	TArray<FGameEntityStorageKey> EntityKeys;
	if (ParseEntityKeys(EntityKeys))
	{
		for (int I = 0; I < EntityKeys.Num(); ++I)
		{
			FGameEntityStorageKey& Key = EntityKeys[I];
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

FString FGameEntityStoragePath::ToFlatFilePath() const
{
	FString FilePath;
	TArray<FGameEntityStorageKey> EntityKeys;
	if (ParseEntityKeys(EntityKeys))
	{
		for (int I = 0; I < EntityKeys.Num(); ++I)
		{
			if (I > 0)
			{
				FilePath += TEXT("_");
			}

			FGameEntityStorageKey& Key = EntityKeys[I];
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

FString FGameEntityStoragePath::ToRESTFulPath() const
{
	FString FilePath;
	TArray<FGameEntityStorageKey> EntityKeys;
	if (ParseEntityKeys(EntityKeys))
	{
		for (int I = 0; I < EntityKeys.Num(); ++I)
		{
			FGameEntityStorageKey& Key = EntityKeys[I];
			FilePath /= Key.Type;
			if (Key.Id.Len() > 0)
			{
				FilePath /= Key.Id;
			}
		}
	}
	return FilePath;
}
