// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameServiceRpcResponder.h"
#include "IMessageContext.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
#include "IGameServiceRpcResponder.h"
#include "HAL/PlatformProcess.h"
#include "IGameServiceRpcServer.h"
#include "GameServiceRpcMessages.h"

class FGameServiceRpcResponderImpl
	: public IGameServiceRpcResponder
{
public:

	virtual ~FGameServiceRpcResponderImpl() { }

public:

	// IGameServiceRpcResponder interface
	virtual FOnGameServiceRpcLookup& OnLookup() override
	{
		return LookupDelegate;
	}

private:

	FGameServiceRpcResponderImpl(
		const FString& InMyHostMacAddress,
		const FString& InMyHostUserId)
		: MyHostMacAddress(InMyHostMacAddress)
		, MyHostUserId(InMyHostUserId)
	{
		MessageEndpoint = FMessageEndpoint::Builder("FGameServiceRpcResponder")
			.Handling<FGameServiceRpcLocateServer>(this, &FGameServiceRpcResponderImpl::HandleMessage);

		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Subscribe<FGameServiceRpcLocateServer>();
		}
	}

private:

	void HandleMessage(const FGameServiceRpcLocateServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
	{
		if (!LookupDelegate.IsBound())
		{
			return;
		}

		if (Message.HostMacAddress != MyHostMacAddress && Message.HostUserId != MyHostUserId)
		{
			return;
		}

		const FString ProductKey = Message.ProductId.ToString() + Message.ProductVersion;
		TSharedPtr<IGameServiceRpcServer> Server = Servers.FindRef(ProductKey);

		if (!Server.IsValid())
		{
			Server = LookupDelegate.Execute(ProductKey);
		}

		if (Server.IsValid())
		{
			Server->ConnectTo(Context->GetSender());
		}
	}


private:

	const FString MyHostMacAddress;
	const FString MyHostUserId;

	/** A delegate that is executed when a look-up for an RPC server occurs. */
	FOnGameServiceRpcLookup LookupDelegate;

	/** Message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	/** Holds the existing RPC servers. */
	TMap<FString, TSharedPtr<IGameServiceRpcServer>> Servers;

	friend FGameServiceRpcResponderFactory;
};

TSharedRef<IGameServiceRpcResponder> FGameServiceRpcResponderFactory::Create()
{
	// @todo: this need to use GetLoginId, but we need to deprecate this functionality over time.
	// eventually, when GetMacAddressString is removed from the codebase, this coude will need to be removed also.
	// In the meantime, it needs to handle BOTH the old Mac address and FPlatformMisc::GetLoginId as a way of recognizing the local machine.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FString Identifier = FPlatformMisc::GetMacAddressString();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	return MakeShareable(new FGameServiceRpcResponderImpl(Identifier, FPlatformProcess::UserName(false)));
}
