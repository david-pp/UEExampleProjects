// Copyright Epic Games, Inc. All Rights Reserved.

#include "DebugingMessages.h"
#include "IMyRpcLocator.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Guid.h"
#include "Containers/Ticker.h"
#include "Misc/EngineVersion.h"
#include "IMessageContext.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
#include "IMyRpcLocator.h"

class FMyRpcLocatorImpl
	: public IMyRpcLocator
{
public:

	virtual ~FMyRpcLocatorImpl()
	{
		FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	}

public:

	// IMyRpcLocator interface

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

	void HandleMessage(const FMyRpcServer& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
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
        QUICK_SCOPE_CYCLE_COUNTER(STAT_FMyRpcLocatorImple_HandleTicker);

		if (ServerAddress.IsValid() && ((FDateTime::UtcNow() - LastServerResponse).GetTotalSeconds() > 15.0))
		{
			ServerAddress.Invalidate();
			ServerLostDelegate.ExecuteIfBound();
		}

		// @todo sarge: implement actual product GUID
		// message is going to be deleted by FMemory::Free() (see FMessageContext destructor), so allocate it with Malloc
		void* Memory = FMemory::Malloc(sizeof(FMyRpcLocateServer));
		MessageEndpoint->Publish(new(Memory) FMyRpcLocateServer(FGuid(), ServerName, MacAddress, UserId), EMessageScope::Network);

		return true;
	}

public:
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	FMyRpcLocatorImpl(const FName& InName, FString InServerName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus)
		: ServerName(InServerName), EngineVersion(FEngineVersion::Current().ToString())
		, MacAddress(FPlatformMisc::GetMacAddressString())
		, UserId(FPlatformProcess::UserName(false))
		, LastServerResponse(FDateTime::MinValue())
	{
		MessageEndpoint = FMessageEndpoint::Builder(InName, InBus)
			.Handling<FMyRpcServer>(this, &FMyRpcLocatorImpl::HandleMessage);

		TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FMyRpcLocatorImpl::HandleTicker), 5.0);
	}
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
	
private:
	FString ServerName;
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
};


TSharedRef<IMyRpcLocator> IMyRpcLocator::Create(const FName& InName, FString ServerName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus)
{
	return MakeShareable(new FMyRpcLocatorImpl(InName, ServerName, InBus));
}
