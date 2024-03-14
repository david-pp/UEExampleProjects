// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Http.h"
#include "JsonObjectConverter.h"
#include "OnlineSessionSettings.h"
#include "HTTPServer/Public/HttpServerRequest.h"
#include "GameSessionTypes.generated.h"

UENUM(BlueprintType)
enum class ERPGGameSessionAttributeDataType : uint8
{
	/** Means the data in the OnlineData value fields should be ignored */
	Empty,
	/** 32 bit integer */
	Int32,
	/** 32 bit unsigned integer */
	UInt32,
	/** 64 bit integer */
	Int64,
	/** 64 bit unsigned integer */
	UInt64,
	/** Double (8 byte) */
	Double,
	/** Unicode string */
	String,
	/** Float (4 byte) */
	Float,
	/** Binary data */
	Blob,
	/** bool data (1 byte) */
	Bool,
	/** Serialized json text */
	Json,
	MAX
};

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
struct ONLINESUBSYSTEMRPG_API FRPGGameSessionAttribute
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
struct ONLINESUBSYSTEMRPG_API FRPGGameSessionSettings
{
	GENERATED_BODY()

public:


public:
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
struct ONLINESUBSYSTEMRPG_API FRPGGameSessionDetails
{
	GENERATED_BODY()

public:
	// FOnlineSessionSettings -> Settings
	void SetupSettingsByOnlineSettings(const FOnlineSessionSettings& OnlineSettings);
	// FOnlineSessionSettings -> Attributes
	void SetupAttributesByOnlineSettings(const FOnlineSessionSettings& OnlineSettings);
	// Settings/Attributes -> FOnlineSessionSettings
	void SetupOnlineSessionSettings(FOnlineSessionSettings& OnlineSettings) const;

	// this -> FOnlineSession
	void SetupToOnlineSession(FOnlineSession& OnlineSession) const;

public:
	/** Session ID assigned by the backend service */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString SessionId;

	/** IP address of this session as visible by the backend service */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString HostAddress;

	/** Number of remaining open spaces on the session (NumPublicConnections - RegisteredPlayers */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 NumOpenPublicConnections = 0;

	/** The settings associated with this session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FRPGGameSessionSettings Settings;

	/** The settings associated with this session */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRPGGameSessionAttribute> Attributes;
};

/**
 * FNamedOnlineSession -> FActiveRPGGameSession
 */
USTRUCT(BlueprintType)
struct ONLINESUBSYSTEMRPG_API FActiveRPGGameSession
{
	GENERATED_BODY()

public:
	// FNamedOnlineSession -> this
	void SetupFromNamedOnlineSession(FNamedOnlineSession* OnlineSession);
	// this -> FNamedOnlineSession
	void SetupToNamedOnlineSession(FNamedOnlineSession* OnlineSession);

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

public:
	void SetupByOnlineSettings(const FOnlineSessionSettings& Settings);
	TSharedPtr<class FOnlineSessionSettings> CreateOnlineSessionSettings() const;
};

struct FHttpHelper
{
	static void ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload)
	{
		int32 StartIndex = OutTCHARPayload.Num();
		OutTCHARPayload.AddUninitialized(FUTF8ToTCHAR_Convert::ConvertedLength((ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR)) * sizeof(TCHAR));
		FUTF8ToTCHAR_Convert::Convert((TCHAR*)(OutTCHARPayload.GetData() + StartIndex), (OutTCHARPayload.Num() - StartIndex) / sizeof(TCHAR), (ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR));
	}
	
	template <typename OutStructType>
	static bool JsonHTTPBodyToUStruct(const TArray<uint8>& Body, OutStructType* OutStruct, int64 CheckFlags = 0, int64 SkipFlags = 0)
	{
		TArray<uint8> TCHARBody;
		FHttpHelper::ConvertToTCHAR(Body, TCHARBody);
		
		FMemoryReaderView BodyReader(TCHARBody);

		FString JsonString;
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(&BodyReader);
		if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
		{
			UE_LOG(LogJson, Warning, TEXT("JsonObjectStringToUStruct - Unable to parse json=[%s]"), *JsonString);
			return false;
		}
		if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), OutStruct, CheckFlags, SkipFlags))
		{
			UE_LOG(LogJson, Warning, TEXT("JsonObjectStringToUStruct - Unable to deserialize. json=[%s]"), *JsonString);
			return false;
		}
		return true;
	}
};


USTRUCT()
struct FHttpSessionSearchResult
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FRPGGameSessionDetails> Sessions;
};
