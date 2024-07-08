// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IMessageBus;
class IGameRpcServerLocator;
class IGameRpcServerResponder;

/**
 * Interface for the GameServiceRpc module.
 */
class IGameMessagingModule : public IModuleInterface
{
public:
	static IGameMessagingModule* Get()
	{
		return static_cast<IGameMessagingModule*>(FModuleManager::Get().LoadModule("GameMessaging"));
	}

	virtual TSharedRef<IGameRpcServerLocator> CreateLocator(const FName& InName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& ServiceBus) = 0;
	virtual TSharedRef<IGameRpcServerResponder> CreateResponder(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& ServiceBus) = 0;

	// virtual TSharedRef<IGameServiceRpcServer> CreateServer(const FString& ServerName, const FServiceMessageBusRef& ServiceBus) = 0;
	// virtual TSharedRef<IGameServiceRpcClient> CreateClient(const FString& ClientName, const FString& ServiceKey, const FServiceMessageBusRef& ServiceBus) = 0;
};
