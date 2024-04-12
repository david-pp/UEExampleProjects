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

public:

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

		// @todo sarge: implement actual product GUID
		// message is going to be deleted by FMemory::Free() (see FMessageContext destructor), so allocate it with Malloc
		void* Memory = FMemory::Malloc(sizeof(FGameServiceRpcLocateServer));
		MessageEndpoint->Publish(new(Memory) FGameServiceRpcLocateServer(FGuid(), EngineVersion, MacAddress, UserId), EMessageScope::Network);

		return true;
	}

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FGameServiceRpcLocatorImpl()
		: EngineVersion(FEngineVersion::Current().ToString())
		, MacAddress(FPlatformMisc::GetMacAddressString())
		, UserId(FPlatformProcess::UserName(false))
		, LastServerResponse(FDateTime::MinValue())
	{
		MessageEndpoint = FMessageEndpoint::Builder("FGameServiceRpcLocator")
			.Handling<FGameServiceRpcServer>(this, &FGameServiceRpcLocatorImpl::HandleMessage);

		TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FGameServiceRpcLocatorImpl::HandleTicker), PORTAL_RPC_LOCATE_INTERVAL);
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

private:

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


TSharedRef<IGameServiceRpcLocator> FGameServiceRpcLocatorFactory::Create()
{
	return MakeShareable(new FGameServiceRpcLocatorImpl());
}
