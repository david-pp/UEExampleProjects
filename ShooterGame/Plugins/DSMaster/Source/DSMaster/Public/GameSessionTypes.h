// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "GameSessionTypes.generated.h"

USTRUCT(BlueprintType)
struct DSMASTER_API FGameSessionAttribute
{
	GENERATED_BODY()

	/** Name of the session attribute */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Key;

	/** Type of value stored in the union */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ValueType;

	/** Attribute Value String */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Value;
};

/**
 * FOnlineSessionSettings -> FGameSessionSettings
 */
USTRUCT(BlueprintType)
struct DSMASTER_API FGameSessionDetailSettings
{
	GENERATED_BODY()

	/**
	 * The main indexed parameter for this session, can be any string.
	 *  eg. "Region:GameMode", "GameMode:Region:MapName"
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString BucketId;

	/** Number of total players allowed in the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumPublicConnections = 0;

	/** Permission level describing allowed access to the session when joining or searching for the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PermissionLevel;

	/** Are players allowed to send invites for the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bInvitesAllowed;
};

/**
 * FOnlineSession -> FGameSessionDetails
 *  - FOnlineSessionSettings -> Settings
 *		- FSessionSettings -> Attributes
 *  - SessionInfo -> Member Variables
 */
USTRUCT(BlueprintType)
struct DSMASTER_API FGameSessionDetails
{
	GENERATED_BODY()

	/** Session ID assigned by the backend service */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionId;

	/** Game server id */
	UPROPERTY()
	FString ServerGuid;
	/** Game server PID */
	UPROPERTY()
	uint32 ServerPID;

	/** IP address of this session as visible by the backend service */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HostAddress;

	/** Number of remaining open spaces on the session (NumPublicConnections - RegisteredPlayers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumOpenPublicConnections = 0;

	/** Current state of the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SessionState = 0;
	/** State debugging string (don't use in code)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionStateString;

	/** List of players registered in the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> RegisteredPlayers;

	/** The settings associated with this session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameSessionDetailSettings Settings;

	/** The settings associated with this session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameSessionAttribute> Attributes;

	bool operator ==(const FGameSessionDetails& Lhs) const
	{
		return SessionId == Lhs.SessionId;
	}

	void SetSessionState(EOnlineSessionState::Type StateValue)
	{
		SessionState = StateValue;
		SessionStateString = EOnlineSessionState::ToString(StateValue);
	}

	EOnlineSessionState::Type GetSessionState() const
	{
		return static_cast<EOnlineSessionState::Type>(SessionState);
	}

	FString GetSessionStateString() const
	{
		return EOnlineSessionState::ToString(GetSessionState());
	}
};

USTRUCT()
struct FGameSessionSearchResult
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FGameSessionDetails> Sessions;
};

USTRUCT()
struct FGameSessionUpdateReply
{
	GENERATED_BODY()

	UPROPERTY()
	FString ErrorMessage;

	bool IsSuccess() const
	{
		return ErrorMessage.IsEmpty();
	}
};

USTRUCT()
struct FGameSessionUpdateStateRequest
{
	GENERATED_BODY()

	/** Session ID */
	UPROPERTY()
	FString SessionId;

	/** Current state of the session */
	UPROPERTY()
	int32 SessionState = 0;

	void SetSessionState(EOnlineSessionState::Type StateValue)
	{
		SessionState = StateValue;
	}
	EOnlineSessionState::Type GetSessionState() const
	{
		return static_cast<EOnlineSessionState::Type>(SessionState);
	}
};

USTRUCT()
struct FGameSessionSetAttributeRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FGameSessionAttribute Attribute;
};
