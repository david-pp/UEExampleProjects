// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IMessageRpcServer;
struct FMessageAddress;

/**
 * Interface for GameService RPC server
 */
class IGameServiceRpcServer
{
public:

	/**
	 * Sends a FGameServiceRpcServer response to the specified address, effectively
	 * allowing the two services to now communicate with each other directly.
	 */
	virtual void SendServiceAddress(const FMessageAddress& Address) const = 0;

	virtual IMessageRpcServer* GetMessageServer() = 0;

public:

	/** Virtual destructor. */
	virtual ~IGameServiceRpcServer() { }
};
