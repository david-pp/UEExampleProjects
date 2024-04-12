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

	virtual void ConnectTo(const FMessageAddress& Address) const
	{
		MessageEndpoint->Send(new FGameServiceRpcServer(GetAddress().ToString()), Address);
	}

	virtual IMessageRpcServer* GetMessageServer()
	{
		return static_cast<IMessageRpcServer*>(this);
	}

public:

	virtual ~FGameServiceRpcServerImpl() { }

private:

	FGameServiceRpcServerImpl()
		: FMessageRpcServer()
	{}

private:

	friend FGameServiceRpcServerFactory;
};

TSharedRef<IGameServiceRpcServer> FGameServiceRpcServerFactory::Create()
{
	return MakeShareable(new FGameServiceRpcServerImpl());
}
