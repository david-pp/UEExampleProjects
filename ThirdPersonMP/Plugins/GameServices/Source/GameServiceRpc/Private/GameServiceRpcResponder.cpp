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

	// IGameServiceRpcResponder interface
	virtual FOnGameServiceRpcLookup& OnLookup() override
	{
		return LookupDelegate;
	}

private:
	FGameServiceRpcResponderImpl(
		const FName& InName,
		const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& ServiceBus,
		const FString& InMyHostMacAddress,
		const FString& InMyHostUserId)
		: MyHostMacAddress(InMyHostMacAddress)
		, MyHostUserId(InMyHostUserId)
	{
		MessageEndpoint = FMessageEndpoint::Builder(InName, ServiceBus)
			.Handling<FGameServiceRpcLocateServer>(this, &FGameServiceRpcResponderImpl::HandleMessage);

		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Subscribe<FGameServiceRpcLocateServer>();
		}
	}

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

		// const FString ServiceKey = Message.ProductId.ToString() + Message.ProductVersion;
		const FString ServiceKey = Message.ServiceKey;
		TSharedPtr<IGameServiceRpcServer> Server = Servers.FindRef(ServiceKey);

		if (!Server.IsValid())
		{
			Server = LookupDelegate.Execute(ServiceKey);
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

TSharedRef<IGameServiceRpcResponder> FGameServiceRpcResponderFactory::Create(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus)
{
	// @todo: this need to use GetLoginId, but we need to deprecate this functionality over time.
	// eventually, when GetMacAddressString is removed from the codebase, this coude will need to be removed also.
	// In the meantime, it needs to handle BOTH the old Mac address and FPlatformMisc::GetLoginId as a way of recognizing the local machine.
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FString Identifier = FPlatformMisc::GetMacAddressString();
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	return MakeShareable(new FGameServiceRpcResponderImpl(FName(DebugName), MessageBus, Identifier, FPlatformProcess::UserName(false)));
}
