// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageBus.h"
#include "Modules/ModuleInterface.h"

class IGameServiceRpcLocator;
class IGameServiceRpcResponder;
class IGameServiceRpcServer;
class IGameServiceRpcClient;

typedef TSharedRef<IMessageBus, ESPMode::ThreadSafe> FServiceMessageBusRef;
typedef TSharedPtr<IMessageBus, ESPMode::ThreadSafe> FServiceMessageBusPtr;

/**
 * Interface for the GameServiceRpc module.
 */
class IGameServiceRpcModule : public IModuleInterface
{
public:
	static IGameServiceRpcModule* Get()
	{
		return static_cast<IGameServiceRpcModule*>(FModuleManager::Get().LoadModule("GameServiceRpc"));
	}

	virtual TSharedRef<IGameServiceRpcLocator> CreateLocator(const FString& Name, const FString& ServiceKey, const FServiceMessageBusRef& ServiceBus) = 0;
	virtual TSharedRef<IGameServiceRpcResponder> CreateResponder(const FString& Name, const FServiceMessageBusRef& ServiceBus) = 0;

	virtual TSharedRef<IGameServiceRpcServer> CreateServer(const FString& ServerName, const FServiceMessageBusRef& ServiceBus) = 0;
	virtual TSharedRef<IGameServiceRpcClient> CreateClient(const FString& ClientName, const FString& ServiceKey, const FServiceMessageBusRef& ServiceBus) = 0;
};
