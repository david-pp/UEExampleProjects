// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemTypes.h"
#include "IPAddress.h"

#ifndef RPG_SUBSYSTEM
#define RPG_SUBSYSTEM FName(TEXT("RPG"))
#endif


class FOnlineSubsystemRPG;

// from OnlineSubsystemTypes.h
TEMP_UNIQUENETIDSTRING_SUBCLASS(FUniqueNetIdRPG, RPG_SUBSYSTEM);

/** 
 * Implementation of session information
 */
class FOnlineSessionInfoRPG : public FOnlineSessionInfo
{
protected:
	/** Hidden on purpose */
	FOnlineSessionInfoRPG& operator=(const FOnlineSessionInfoRPG& Src)
	{
		return *this;
	}

public:
	/** Constructor */
	FOnlineSessionInfoRPG();

	FOnlineSessionInfoRPG(const FOnlineSessionInfoRPG& Src)
		: FOnlineSessionInfo(Src), HostAddr(Src.HostAddr), SessionId(Src.SessionId)
	{
	}

	/** 
	 * Initialize a RPG session info with the address of this machine
	 * and an id for the session
	 */
	void InitLAN(const FOnlineSubsystemRPG& Subsystem);
	void InitDS(const FOnlineSubsystemRPG& Subsystem);

	/** The ip & port that the host is listening on (valid for LAN/GameServer) */
	TSharedPtr<class FInternetAddr> HostAddr;
	/** Unique Id for this session */
	FUniqueNetIdRPG SessionId;

public:
	virtual ~FOnlineSessionInfoRPG()
	{
	}

	bool operator==(const FOnlineSessionInfoRPG& Other) const
	{
		return false;
	}

	virtual const uint8* GetBytes() const override
	{
		return NULL;
	}

	virtual int32 GetSize() const override
	{
		return sizeof(uint64) + sizeof(TSharedPtr<class FInternetAddr>);
	}

	virtual bool IsValid() const override
	{
		// LAN case
		return HostAddr.IsValid() && HostAddr->IsValid();
	}

	virtual FString ToString() const override
	{
		return SessionId.ToString();
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("HostIP: %s SessionId: %s"), HostAddr.IsValid() ? *HostAddr->ToString(true) : TEXT("INVALID"), *SessionId.ToDebugString());
	}

	virtual const FUniqueNetId& GetSessionId() const override
	{
		return SessionId;
	}
};
