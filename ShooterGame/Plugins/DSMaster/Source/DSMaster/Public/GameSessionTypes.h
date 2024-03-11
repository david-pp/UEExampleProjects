// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameSessionTypes.generated.h"

UENUM()
enum class ERPGGameSessionStateType
{
	/** An online session has not been created yet */
	NoSession,
	/** An online session is in the process of being created */
	Creating,
	/** Session has been created but the session hasn't started (pre match lobby) */
	Pending,
	/** Session has been asked to start (may take time due to communication with backend) */
	Starting,
	/** The current session has started. Sessions with join in progress disabled are no longer joinable */
	InProgress,
	/** The session is still valid, but the session is no longer being played (post match lobby) */
	Ending,
	/** The session is closed and any stats committed */
	Ended,
	/** The session is being destroyed */
	Destroying
};

USTRUCT(BlueprintType)
struct DSMASTER_API FRPGGameSessionAttribute
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
struct DSMASTER_API FRPGGameSessionSettings
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
struct DSMASTER_API FRPGGameSessionDetails
{
	GENERATED_BODY()

	/** Session ID assigned by the backend service */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionId;

	/** IP address of this session as visible by the backend service */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HostAddress;

	/** Number of remaining open spaces on the session (NumPublicConnections - RegisteredPlayers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumOpenPublicConnections = 0;

	/** Current state of the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERPGGameSessionStateType SessionState = ERPGGameSessionStateType::NoSession;

	/** List of players registered in the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> RegisteredPlayers;

	/** The settings associated with this session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRPGGameSessionSettings Settings;

	/** The settings associated with this session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRPGGameSessionAttribute> Attributes;

	bool operator ==(const FRPGGameSessionDetails& Lhs) const
	{
		return SessionId == Lhs.SessionId;
	}
};

/**
 * FNamedOnlineSession -> FActiveRPGGameSession (TODO: delete)
 */
USTRUCT(BlueprintType)
struct DSMASTER_API FActiveRPGGameSession
{
	GENERATED_BODY()

public:
	/** Name of the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionName;

	/** The User ID of the local user who created or joined the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString LocalOwnerId;

	/** Current state of the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERPGGameSessionStateType SessionState = ERPGGameSessionStateType::NoSession;

	/** List of players registered in the session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> RegisteredPlayers;

	/** Session details */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRPGGameSessionDetails SessionDetails;
};

USTRUCT()
struct FHttpSessionSearchResult
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRPGGameSessionDetails> Sessions;
};
