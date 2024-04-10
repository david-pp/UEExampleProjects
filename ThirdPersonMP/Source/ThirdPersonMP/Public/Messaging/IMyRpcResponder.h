// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageRpcServer.h"
#include "MessageRpcServer.h"

DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<IMessageRpcServer>, FOnMyRpcLookup, const FString& /*ProductKey*/)


/**
 * Interface for My RPC responders.
 */
class IMyRpcResponder
{
public:
	static TSharedRef<IMyRpcResponder> Create(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus);

	/** Get a delegate that is executed when a look-up for an RPC server occurs. */
	virtual FOnMyRpcLookup& OnLookup() = 0;

public:

	/** Virtual destructor. */
	virtual ~IMyRpcResponder() { }
};

class FMyRpcServerImpl : public FMessageRpcServer
{
public:
	virtual void SendServerAddressTo(const FMessageAddress& Address) const
	{
		MessageEndpoint->Send(new FMyRpcServer(GetAddress().ToString()), Address);
	}

	virtual IMessageRpcServer* GetMessageServer()
	{
		return static_cast<IMessageRpcServer*>(this);
	}

public:
	virtual ~FMyRpcServerImpl() { }

	using FMessageRpcServer::FMessageRpcServer;
};
