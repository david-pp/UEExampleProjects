// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineSubsystemRPGTypes.h"
#include "NboSerializer.h"

/**
 * Serializes data in network byte order form into a buffer
 */
class FNboSerializeToBufferRPG : public FNboSerializeToBuffer
{
public:
	/** Default constructor zeros num bytes*/
	FNboSerializeToBufferRPG() :
		FNboSerializeToBuffer(512)
	{
	}

	/** Constructor specifying the size to use */
	FNboSerializeToBufferRPG(uint32 Size) :
		FNboSerializeToBuffer(Size)
	{
	}

	/**
	 * Adds RPG session info to the buffer
	 */
 	friend inline FNboSerializeToBufferRPG& operator<<(FNboSerializeToBufferRPG& Ar, const FOnlineSessionInfoRPG& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar << SessionInfo.SessionId;
		Ar << *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Adds RPG Unique Id to the buffer
	 */
	friend inline FNboSerializeToBufferRPG& operator<<(FNboSerializeToBufferRPG& Ar, const FUniqueNetIdRPG& UniqueId)
	{
		Ar << UniqueId.UniqueNetIdStr;
		return Ar;
	}
};

/**
 * Class used to write data into packets for sending via system link
 */
class FNboSerializeFromBufferRPG : public FNboSerializeFromBuffer
{
public:
	/**
	 * Initializes the buffer, size, and zeros the read offset
	 */
	FNboSerializeFromBufferRPG(uint8* Packet,int32 Length) :
		FNboSerializeFromBuffer(Packet,Length)
	{
	}

	/**
	 * Reads RPG session info from the buffer
	 */
 	friend inline FNboSerializeFromBufferRPG& operator>>(FNboSerializeFromBufferRPG& Ar, FOnlineSessionInfoRPG& SessionInfo)
 	{
		check(SessionInfo.HostAddr.IsValid());
		// Skip SessionType (assigned at creation)
		Ar >> SessionInfo.SessionId; 
		Ar >> *SessionInfo.HostAddr;
		return Ar;
 	}

	/**
	 * Reads RPG Unique Id from the buffer
	 */
	friend inline FNboSerializeFromBufferRPG& operator>>(FNboSerializeFromBufferRPG& Ar, FUniqueNetIdRPG& UniqueId)
	{
		Ar >> UniqueId.UniqueNetIdStr;
		return Ar;
	}
};
