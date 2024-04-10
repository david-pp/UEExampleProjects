// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RpcMessage.h"
#include "DebugingMessages.generated.h"

USTRUCT()
struct FDebugServiceHeartBeat
{
	GENERATED_USTRUCT_BODY()

	/** The name of the user who sent this ping. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString ServiceName;
};

USTRUCT()
struct FDebugServicePing
{
	GENERATED_USTRUCT_BODY()

	/** The name of the user who sent this ping. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString UserName;

	/** Default constructor. */
	FDebugServicePing()
	{
	}

	/** Creates and initializes a new instance. */
	FDebugServicePing(const FString& InUserName) : UserName(InUserName)
	{
	}
};

/**
 * Implements a message that is published in response to FSessionServicePing.
 */
USTRUCT()
struct FDebugServicePong
{
	GENERATED_USTRUCT_BODY()

	/** Indicates whether the pinging user is authorized to interact with this session. */
	UPROPERTY(EditAnywhere, Category="Message")
	bool Authorized = true;

	UPROPERTY(EditAnywhere, Category="Message")
	FString SericeName;
	UPROPERTY(EditAnywhere, Category="Message")
	FString UserName;

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


/*
 * Debug RPC
 */

USTRUCT(BlueprintType)
struct FMyResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	FString RetValString;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Message")
	int32 RetVal = 0;
};

USTRUCT()
struct FMyRpcRequest : public FRpcMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Message")
	FString Param1;

	UPROPERTY(EditAnywhere, Category="Message")
	int32 Param2 = 0;

	FMyRpcRequest()
	{
	}

	FMyRpcRequest(const FString& InParam1) : Param1(InParam1)
	{
	}

	FMyRpcRequest(const FString& InParam1, int32 InParam2) : Param1(InParam1), Param2(InParam2)
	{
	}
};

USTRUCT()
struct FMyRpcResponse : public FRpcMessage
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, Category="Message")
	FMyResult Result;
	
	FMyRpcResponse() {}
	FMyRpcResponse(const FMyResult& InResult) : Result(InResult)
	{
	}
};

DECLARE_RPC(FMyRpc, FMyResult)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMyRpcComplete, const FMyResult&, Result);


/**
 * RPC Server Location
 */


USTRUCT()
struct FMyRpcLocateServer
{
	GENERATED_USTRUCT_BODY()

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

	/** Default constructor. */
	FMyRpcLocateServer() { }

	/** Create and initialize a new instance. */
	FMyRpcLocateServer(const FGuid& InProductId, const FString& InProductVersion, const FString& InHostMacAddress, const FString& InHostUserId)
		: ProductId(InProductId)
		, ProductVersion(InProductVersion)
		, HostMacAddress(InHostMacAddress)
		, HostUserId(InHostUserId)
	{ }
};


USTRUCT()
struct FMyRpcServer
{
	GENERATED_USTRUCT_BODY()

	/** The RPC server's message address as a string. */
	UPROPERTY(EditAnywhere, Category="Message")
	FString ServerAddress;

	/** Default constructor. */
	FMyRpcServer() { }

	/** Create and initialize a new instance. */
	FMyRpcServer(const FString& InServerAddress)
		: ServerAddress(InServerAddress)
	{ }
};
