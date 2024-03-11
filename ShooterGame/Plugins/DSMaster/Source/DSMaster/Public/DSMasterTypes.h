// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMasterTypes.generated.h"

class ADSMasterBeaconHost;
class ADSMasterBeaconClient;

UENUM()
enum class EDSMasterMode : uint8
{
	None,
	/** Run as Master Server */
	Master,
	/** Run as Agent Server */
	Agent,
};

USTRUCT(BlueprintType)
struct FDSMasterHttpServerSettings
{
	GENERATED_BODY()
};

/**
 * Server Settings - Master/Agent
 */
USTRUCT(BlueprintType)
struct FDSMasterBeaconHostSettings
{
	GENERATED_BODY()

	/** Listen Port */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ListenPort = 9000;

	/** Running Mode */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDSMasterMode RunningMode = EDSMasterMode::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ADSMasterBeaconHost> DSMasterBeaconHostObjectClass;
};

UENUM()
enum class EDSMasterClientType : uint8
{
	/** Invalid*/
	None,
	/** DSAgent */
	Agent,
	/** Dedicated Server*/
	DedicatedServer,
	/** Other Game Services */
	GameService,
	/** Game Client directly */
	GameClient,
};

/**
 * Client Settings - Agent
 */
USTRUCT(BlueprintType)
struct FDSMasterClientSettings
{
	GENERATED_BODY()

	/** Master Server's Address */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MasterServerAddress;

	/** Master Client Type */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDSMasterClientType ClientType = EDSMasterClientType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<ADSMasterBeaconClient> DSMasterBeaconClientClass;
};
