// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RpcMessage.h"
#include "UObject/Interface.h"
#include "GameMessages.generated.h"

/*
 * Endpoint heart beat
 */
USTRUCT()
struct FGameEndpointHeartBeat
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName EndpointName;
	// UPROPERTY()
	// FString EndpointAddress;
};

/**
 * Endpoint ping message
 */
USTRUCT()
struct FGameEndpointPing
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FName EndpointName;


	/** Default constructor. */
	FGameEndpointPing()
	{
	}

	/** Creates and initializes a new instance. */
	FGameEndpointPing(const FName& InEndpointName) : EndpointName(InEndpointName)
	{
	}
};

/**
 * Endpoint pong message
 */
USTRUCT()
struct FGameEndpointPong
{
	GENERATED_USTRUCT_BODY()

	/** Indicates whether the pinging user is authorized to interact with this session. */
	UPROPERTY(EditAnywhere, Category="Message")
	bool Authorized = true;

	UPROPERTY()
	FName EndpointName;

	/** Holds the application's build date. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString BuildDate;

	/** Holds the name of the device that the application is running on. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString DeviceName;

	/** Holds the application's instance identifier. */
	UPROPERTY(EditAnywhere, Category="Message")
	FGuid InstanceId;

	/** Holds the application's instance name. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString InstanceName;

	/** Holds the name of the platform that the application is running on. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString PlatformName;

	/** Holds the identifier of the session that the application belongs to. */
	UPROPERTY(EditAnywhere, Category="Message")
	FGuid SessionId;

	/** Holds the user defined name of the session. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString SessionName;

	/** Holds the name of the user that started the session. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString SessionOwner;

	/** Indicates whether the application is the only one in that session. */
	UPROPERTY(EditAnywhere, Category="Message")
	bool Standalone = true;
};


/**
 * Game rpc server locate message
 */
USTRUCT()
struct FGameRpcServerLocate
{
	GENERATED_USTRUCT_BODY()

	/** name/id of rpc server */
	UPROPERTY(EditAnywhere, Category="Message")
	FString ServerKey;

	/** The product's unique identifier. */
	UPROPERTY(EditAnywhere, Category="Message")
	FGuid ProductId;

	/** The product's version string. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString ProductVersion;

	/** The mac address of the host. */
	UPROPERTY(EditAnywhere, Category = "Message")
	FString HostMacAddress;

	/** The user identification for the host. */
	UPROPERTY(EditAnywhere, Category = "Message")
	FString HostUserId;
};

/**
 * Game rpc server result message
 */
USTRUCT()
struct FGameRpcServerLocation
{
	GENERATED_USTRUCT_BODY()

	/** The RPC server's message address as a string. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString ServerAddress;
};


/*
 * Game debug RPC:
 *  Request -> Response+Result
 */
USTRUCT(BlueprintType)
struct FGameRpcDebugResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	FString RetValString;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	int32 RetVal = 0;
};

USTRUCT(BlueprintType)
struct FGameRpcDebugRequest : public FRpcMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Message")
	FString Param1;
	UPROPERTY(EditAnywhere, Category="Message")
	int32 Param2 = 0;

	FGameRpcDebugRequest()
	{
	}

	FGameRpcDebugRequest(const FString& InParam1) : Param1(InParam1)
	{
	}

	FGameRpcDebugRequest(const FString& InParam1, int32 InParam2) : Param1(InParam1), Param2(InParam2)
	{
	}
};

USTRUCT(BlueprintType)
struct FGameRpcDebugResponse : public FRpcMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Message")
	FGameRpcDebugResult Result;

	FGameRpcDebugResponse()
	{
	}

	FGameRpcDebugResponse(const FGameRpcDebugResult& InResult) : Result(InResult)
	{
	}
};

DECLARE_RPC(FGameRpcDebug, FGameRpcDebugResult)

// ...
