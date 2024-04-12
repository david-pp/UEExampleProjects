// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "GameServiceRpcClient.h"
#include "Modules/ModuleManager.h"
#include "IGameServiceRpcModule.h"
#include "GameServiceRpcLocator.h"
#include "GameServiceRpcResponder.h"
#include "GameServiceRpcServer.h"

class FGameServiceRpcModule : public IGameServiceRpcModule
{
public:
	// IModuleInterface interface
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

public:
	virtual TSharedRef<IGameServiceRpcLocator> CreateLocator(const FString& DebugName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus) override
	{
		return FGameServiceRpcLocatorFactory::Create(DebugName, ServiceKey, MessageBus);
	}

	virtual TSharedRef<IGameServiceRpcResponder> CreateResponder(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus) override
	{
		return FGameServiceRpcResponderFactory::Create(DebugName, MessageBus);
	}

	virtual TSharedRef<IGameServiceRpcServer> CreateServer(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus) override
	{
		return FGameServiceRpcServerFactory::Create(DebugName, MessageBus);
	}

	virtual TSharedRef<IGameServiceRpcClient> CreateClient(const FString& ClientName, const FString& ServiceKey, const FServiceMessageBusRef& ServiceBus) override
	{
		return MakeShared<FGameServiceRpcClient>(ClientName, ServiceKey, ServiceBus);
	}
};

IMPLEMENT_MODULE(FGameServiceRpcModule, GameServiceRpc);
