// Copyright Epic Games, Inc. All Rights Reserved.

#include "DebugingMessages.h"
#include "IMessageContext.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
#include "IMyRpcResponder.h"
#include "MessageRpcServer.h"
#include "HAL/PlatformProcess.h"

class FMyRpcResponderImpl
	: public IMyRpcResponder
{
public:

	virtual ~FMyRpcResponderImpl() { }

public:

	// IMyRpcResponder interface
	virtual FOnMyRpcLookup& OnLookup() override
	{
		return LookupDelegate;
	}

	FMyRpcResponderImpl(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus,
		const FString& InMyHostMacAddress,
		const FString& InMyHostUserId)
		: MyHostMacAddress(InMyHostMacAddress)
		, MyHostUserId(InMyHostUserId)
	{
		MessageEndpoint = FMessageEndpoint::Builder(InName, InBus)
			.Handling<FMyRpcLocateServer>(this, &FMyRpcResponderImpl::HandleMessage);

		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Subscribe<FMyRpcLocateServer>();
		}
	}

private:

	void HandleMessage(const FMyRpcLocateServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
	{
		if (!LookupDelegate.IsBound())
		{
			return;
		}

		if (Message.HostMacAddress != MyHostMacAddress && Message.HostUserId != MyHostUserId)
		{
			return;
		}

		// const FString ProductKey = Message.ProductId.ToString() + Message.ProductVersion;
		const FString ProductKey = Message.ProductVersion;
		TSharedPtr<IMessageRpcServer> Server = Servers.FindRef(ProductKey);

		if (!Server.IsValid())
		{
			Server = LookupDelegate.Execute(ProductKey);
		}

		if (Server.IsValid())
		{
			FMyRpcServerImpl* RpcServer = static_cast<FMyRpcServerImpl*>(Server.Get());
			if (RpcServer)
			{
				RpcServer->SendServerAddressTo(Context->GetSender());
			}
		}
	}

private:

	const FString MyHostMacAddress;
	const FString MyHostUserId;

	/** A delegate that is executed when a look-up for an RPC server occurs. */
	FOnMyRpcLookup LookupDelegate;

	/** Message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	/** Holds the existing RPC servers. */
	TMap<FString, TSharedPtr<IMessageRpcServer>> Servers;
};

TSharedRef<IMyRpcResponder> IMyRpcResponder::Create(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus)
{
	// @todo: this need to use GetLoginId, but we need to deprecate this functionality over time.
	// eventually, when GetMacAddressString is removed from the codebase, this coude will need to be removed also.
	// In the meantime, it needs to handle BOTH the old Mac address and FPlatformMisc::GetLoginId as a way of recognizing the local machine.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FString Identifier = FPlatformMisc::GetMacAddressString();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	return MakeShareable(new FMyRpcResponderImpl(InName, InBus, Identifier, FPlatformProcess::UserName(false)));
}
