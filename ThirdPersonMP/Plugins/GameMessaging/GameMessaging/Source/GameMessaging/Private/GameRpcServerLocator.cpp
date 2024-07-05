// Fill out your copyright notice in the Description page of Project Settings.

#include "GameRpcServerLocator.h"

#include "GameMessages.h"
#include "IMessageContext.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"

class FGameRpcServerLocator : public IGameRpcServerLocator
{
public:
	FGameRpcServerLocator(const FName& InName, const FString& InServerKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus)
	{
		ServerKey = InServerKey;
		EngineVersion = FEngineVersion::Current().ToString();
		MacAddress = FPlatformMisc::GetMacAddressString();
		UserId = FPlatformProcess::UserName(false);
		LastServerResponse = FDateTime::MinValue();

		// locator endpoint
		MessageEndpoint = FMessageEndpoint::Builder(InName, InBus).Handling<FGameRpcServerLocation>(this, &FGameRpcServerLocator::HandleMessage);
		// locate rpc server tick 
		TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FGameRpcServerLocator::HandleTicker), GAME_RPC_LOCATE_INTERVAL);
	}

	virtual ~FGameRpcServerLocator()
	{
		FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	}

	// IGameServiceRpcLocator interface
	virtual const FMessageAddress& GetServerAddress() const override
	{
		return ServerAddress;
	}

	virtual FSimpleDelegate& OnServerLocated() override
	{
		return ServerLocatedDelegate;
	}

	virtual FSimpleDelegate& OnServerLost() override
	{
		return ServerLostDelegate;
	}

private:
	void HandleMessage(const FGameRpcServerLocation& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
	{
		LastServerResponse = FDateTime::UtcNow();

		FMessageAddress NewServerAddress;
		if (FMessageAddress::Parse(Message.ServerAddress, NewServerAddress) && (NewServerAddress != ServerAddress))
		{
			ServerAddress = NewServerAddress;
			ServerLocatedDelegate.ExecuteIfBound();
		}
	}

	bool HandleTicker(float DeltaTime)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FGameServiceRpcLocatorImple_HandleTicker);

		if (ServerAddress.IsValid() && ((FDateTime::UtcNow() - LastServerResponse).GetTotalSeconds() > GAME_RPC_LOCATE_TIMEOUT))
		{
			ServerLostDelegate.ExecuteIfBound();
			ServerAddress.Invalidate();
		}

		// Broadcast the locate message to all Responder
		auto Message = FMessageEndpoint::MakeMessage<FGameRpcServerLocate>();
		Message->ServerKey = ServerKey;
		Message->ProductVersion = EngineVersion;
		Message->HostMacAddress = MacAddress;
		Message->HostUserId = UserId;
		MessageEndpoint->Publish(Message, EMessageScope::Network);
		return true;
	}

private:
	FString ServerKey;
	FString EngineVersion;
	FString MacAddress;
	FString UserId;

	/** Time at which the RPC server last responded. */
	FDateTime LastServerResponse;

	/** Message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	/** The message address of the located RPC server, or invalid if no server available. */
	FMessageAddress ServerAddress;

	/** A delegate that is executed when an RPC server has been located. */
	FSimpleDelegate ServerLocatedDelegate;

	/** A delegate that is executed when the RPC server has been lost. */
	FSimpleDelegate ServerLostDelegate;

	/** Handle to the registered ticker. */
	FDelegateHandle TickerHandle;
};

TSharedRef<IGameRpcServerLocator> IGameRpcServerLocator::Create(const FName& InName, const FString& ServerKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus)
{
	return MakeShareable(new FGameRpcServerLocator(InName, ServerKey, InBus));
}
