// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameNatsMessages.generated.h"

#define NATS_PUBLIC_CHANNEL TEXT("GameNatsPublic")
#define NATS_CLIENT_PRIVATE_CHANNEL TEXT("GameNatsPrivate")
#define NATS_CLIENT_STATUS_CHANNEL TEXT("GameNatsClientStatus")

/** Defines the protocol version of the Nats message transport. */
namespace ENatsMessagingVersion
{
	enum Type
	{
		Initial,
		ChangedMessageLengthToInt32,

		// -----<new versions can be added before this line>-------------------------------------------------
		// - this needs to be the last line (see note below)
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1,
		// bump this when break need to break compatibility.
		OldestSupportedVersion = ChangedMessageLengthToInt32
	};
}


/** Defines a magic number for the the NATS message transport. */
#define NATS_MESSAGING_TRANSPORT_PROTOCOL_MAGIC 0x45504943

/** Header sent over the connection as soon as it's opened */
struct FGameNatsClientStatus
{
	uint32 MagicNumber = 0;
	uint32 Version = 0;

	FGuid NodeId;
	FString NodeName;

	uint32 Status = 0;
	FDateTime Timestamp;

	FGameNatsClientStatus()
		: MagicNumber(0), Version(0)
	{
		Timestamp = FDateTime::Now();
	}

	FGameNatsClientStatus(const FGuid& InNodeId)
		: MagicNumber(NATS_MESSAGING_TRANSPORT_PROTOCOL_MAGIC), Version(ENatsMessagingVersion::LatestVersion), NodeId(InNodeId)
	{
		Timestamp = FDateTime::Now();
	}

	bool IsValid() const
	{
		return MagicNumber == NATS_MESSAGING_TRANSPORT_PROTOCOL_MAGIC && Version == ENatsMessagingVersion::OldestSupportedVersion && NodeId.IsValid();
	}

	FGuid GetNodeId() const
	{
		return NodeId;
	}

	uint32 GetVersion() const
	{
		return Version;
	}

	uint32 GetStatus() const
	{
		return Status;
	}

	// Serializer
	friend FArchive& operator<<(FArchive& Ar, FGameNatsClientStatus& H)
	{
		return Ar << H.MagicNumber << H.Version << H.NodeId << H.Status << H.Timestamp << H.NodeName;
	}

	FString ToDebugString() const
	{
		if (IsValid())
		{
			return FString::Printf(TEXT("%s(%s) - status:%d"), *NodeName, *NodeId.ToString(), Status);
		}
		else
		{
			return FString::Printf(TEXT("%s(%s) - Invalid"), *NodeName, *NodeId.ToString());
		}
	}
};

USTRUCT()
struct FGameNatsMessage_NodeHeartBeat
{
	GENERATED_BODY()
};

