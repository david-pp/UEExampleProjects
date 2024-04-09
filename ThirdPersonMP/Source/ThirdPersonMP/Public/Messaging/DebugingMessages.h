// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	FDebugServicePing() { }

	/** Creates and initializes a new instance. */
	FDebugServicePing(const FString& InUserName)
		: UserName(InUserName)
	{ }
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