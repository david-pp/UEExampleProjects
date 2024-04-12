// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameServiceRpcLocator.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Guid.h"
#include "Containers/Ticker.h"
#include "Misc/EngineVersion.h"
#include "IMessageContext.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
#include "GameServiceRpcDefines.h"
#include "IGameServiceRpcLocator.h"
#include "GameServiceRpcMessages.h"


class FGameServiceRpcLocatorImpl
	: public IGameServiceRpcLocator
{
public:

	virtual ~FGameServiceRpcLocatorImpl()
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

	void HandleMessage(const FGameServiceRpcServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
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

		if (ServerAddress.IsValid() && ((FDateTime::UtcNow() - LastServerResponse).GetTotalSeconds() > PORTAL_RPC_LOCATE_TIMEOUT))
		{
			ServerAddress.Invalidate();
			ServerLostDelegate.ExecuteIfBound();
		}

		auto Message = FMessageEndpoint::MakeMessage<FGameServiceRpcLocateServer>();
		Message->ServiceKey = ServiceKey;
		Message->ProductVersion = EngineVersion;
		Message->HostMacAddress = MacAddress;
		Message->HostUserId = UserId;
		MessageEndpoint->Publish(Message, EMessageScope::Network);
		return true;
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FGameServiceRpcLocatorImpl(const FName& InName, const FString& InServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus)
		: ServiceKey(InServiceKey)
		, EngineVersion(FEngineVersion::Current().ToString())
		, MacAddress(FPlatformMisc::GetMacAddressString())
		, UserId(FPlatformProcess::UserName(false))
		, LastServerResponse(FDateTime::MinValue())
	{
		MessageEndpoint = FMessageEndpoint::Builder(InName, InBus)
			.Handling<FGameServiceRpcServer>(this, &FGameServiceRpcLocatorImpl::HandleMessage);

		TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FGameServiceRpcLocatorImpl::HandleTicker), PORTAL_RPC_LOCATE_INTERVAL);
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

private:
	const FString ServiceKey;

	const FString EngineVersion;
	const FString MacAddress;
	const FString UserId;

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

	friend FGameServiceRpcLocatorFactory;
};

TSharedRef<IGameServiceRpcLocator> FGameServiceRpcLocatorFactory::Create(const FString& DebugName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus)
{
	return MakeShareable(new FGameServiceRpcLocatorImpl(FName(DebugName), ServiceKey, MessageBus));
}
