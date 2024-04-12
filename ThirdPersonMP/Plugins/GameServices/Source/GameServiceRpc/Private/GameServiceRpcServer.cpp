// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameServiceRpcServer.h"
#include "IGameServiceRpcServer.h"
#include "GameServiceRpcMessages.h"
#include "MessageEndpoint.h"
#include "MessageRpcServer.h"

class FGameServiceRpcServerImpl
	: public FMessageRpcServer
	, public IGameServiceRpcServer
{
public:

	virtual void SendServiceAddress(const FMessageAddress& Address) const
	{
		MessageEndpoint->Send(new FGameServiceRpcServer(GetAddress().ToString()), Address);
	}

	virtual IMessageRpcServer* GetMessageServer()
	{
		return static_cast<IMessageRpcServer*>(this);
	}

public:

	virtual ~FGameServiceRpcServerImpl() { }

	using FMessageRpcServer::FMessageRpcServer;

private:

	friend FGameServiceRpcServerFactory;
};

TSharedRef<IGameServiceRpcServer> FGameServiceRpcServerFactory::Create(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus)
{
	return MakeShareable(new FGameServiceRpcServerImpl(DebugName, MessageBus));
}
