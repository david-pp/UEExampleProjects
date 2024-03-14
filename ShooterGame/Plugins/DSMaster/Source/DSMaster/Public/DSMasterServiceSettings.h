// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMasterServiceSettings.generated.h"

USTRUCT(BlueprintType)
struct FGameServerMapSettings
{
	GENERATED_BODY()
	
	/** Map Bucket */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MapBucket;
	/** Map name or full path */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MapName;
	/** Default game mode*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DefaultGameMode;

	/** Minimum number of server instances to keep running on each VM */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinInstances = 0;

	/** Minimum number of server instances to keep running on each VM  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxInstances = 10;
};

USTRUCT(BlueprintType)
struct FGameServerSettings
{
	GENERATED_BODY()

	/**
	 * Dedicated Server directory
	 *  - Default : [ProjectDir]/Binaries/[Platform]
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerDirectory;

	/**
	 * Dedicated Server Exe
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerName;

	/** From ServerPort to start launch Game Servers*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ServerPort = 7000;

	/** Game maps */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameServerMapSettings> ServerMaps;
};


USTRUCT(BlueprintType)
struct FDSMasterServiceSettings
{
	GENERATED_BODY()

	/** Game dedicated server settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameServerSettings GameServer;
};
