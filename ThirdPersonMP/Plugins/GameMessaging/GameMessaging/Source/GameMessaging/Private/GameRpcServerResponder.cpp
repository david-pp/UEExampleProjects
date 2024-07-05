// Fill out your copyright notice in the Description page of Project Settings.


#include "GameRpcServerResponder.h"

#include "GameMessages.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"

class FGameRpcServerResponder : public IGameRpcServerResponder
{
public:
	FGameRpcServerResponder(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& ServiceBus)
	{
		MyHostMacAddress = FPlatformMisc::GetMacAddressString();
		MyHostUserId = FPlatformProcess::UserName(false);

		MessageEndpoint = FMessageEndpoint::Builder(InName, ServiceBus).Handling<FGameRpcServerLocate>(this, &FGameRpcServerResponder::HandleMessage);
		if (MessageEndpoint.IsValid())
		{
			MessageEndpoint->Subscribe<FGameRpcServerLocate>();
		}
	}

	virtual ~FGameRpcServerResponder() override
	{
	}

	// IGameServiceRpcResponder interface
	virtual FOnGameRpcServerLookup& OnLookup() override
	{
		return LookupDelegate;
	}

	virtual void AddRpcServerToLookup(const FString& ServerKey, TSharedPtr<FGameRpcServer> RpcServer) override
	{
		RpcServers.FindOrAdd(ServerKey) = RpcServer;
	}

	virtual void RemoveRpcServerFromLookup(const FString& ServerKey) override
	{
		RpcServers.Remove(ServerKey);
	}

private:
	void HandleMessage(const FGameRpcServerLocate& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
	{
		// if (Message.HostMacAddress != MyHostMacAddress && Message.HostUserId != MyHostUserId)
		// {
		// 	return;
		// }

		// const FString ServiceKey = Message.ProductId.ToString() + Message.ProductVersion;
		const FString ServiceKey = Message.ServerKey;

		// Lookup server by Delegate
		if (LookupDelegate.IsBound())
		{
			TSharedPtr<FGameRpcServer> Server = LookupDelegate.Execute(ServiceKey);
			if (Server)
			{
				Server->SendRpcServerLocationTo(Context->GetSender());
				return;
			}
		}

		// Lookup server by RpcServer lookup table
		TSharedPtr<FGameRpcServer> Server = RpcServers.FindRef(ServiceKey);
		if (Server.IsValid())
		{
			Server->SendRpcServerLocationTo(Context->GetSender());
		}
	}

private:
	FString MyHostMacAddress;
	FString MyHostUserId;

	/** A delegate that is executed when a look-up for an RPC server occurs. */
	FOnGameRpcServerLookup LookupDelegate;

	/** Message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	/** Holds the existing RPC servers. */
	TMap<FString, TSharedPtr<FGameRpcServer>> RpcServers;
};

TSharedRef<IGameRpcServerResponder> IGameRpcServerResponder::Create(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus)
{
	return MakeShareable(new FGameRpcServerResponder(InName, MessageBus));
}
