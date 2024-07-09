// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameServiceSettings.generated.h"

/**
 * Message Bus settings
 */
USTRUCT(BlueprintType)
struct FGameServiceMessageBusSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString MessageBusName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=GameService)
	bool bEnableTcpBridge = true;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=GameService)
	bool bEnableNatsBridge = false;

	/** Tcp Listen Server */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="bEnableTcpBridge"), Category=TcpTransport)
	FString TcpListenEndpoint;
	/** Tcp Servers to connect */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="bEnableTcpBridge"), Category=TcpTransport)
	TArray<FString> TcpConnectToEndpoints;

	/** Nats Server, such as : nats://127.0.0.1:4222 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="bEnableNatsBridge"), Category=NatsTransport)
	FString NatsServerURL;

	FGameServiceMessageBusSettings()
	{
		MessageBusName = TEXT("ServiceBus");
		TcpListenEndpoint = TEXT("127.0.0.1:5000");
		NatsServerURL = TEXT("nats://127.0.0.1:4222");
	}
};


/**
 * Game service settings for service engine
 */
USTRUCT(BlueprintType)
struct FGameServiceSettings
{
	GENERATED_BODY()

	/** Service/Proxy name */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ServiceName;

	/** Service/Proxy wildcard string to match */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString ServiceWildcard;

	/** Service/Proxy module */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ServiceModule;

	/** Load module & create service on start ?  */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bCreateServiceOnStart = false;
};


/**
 * Game service settings for service engine
 */
USTRUCT()
struct FGameServiceEngineSettings
{
	GENERATED_BODY()

	/** Service bus settings */
	UPROPERTY()
	FGameServiceMessageBusSettings ServiceBus;

	/** Game services register to engine */
	UPROPERTY()
	TArray<FGameServiceSettings> GameServices;

	/** Game service proxy register to engine */
	UPROPERTY()
	TArray<FGameServiceSettings> GameProxies;
};
