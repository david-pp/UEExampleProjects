// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageBus.h"
#include "Modules/ModuleInterface.h"

class IGameServiceRpcLocator;
class IGameServiceRpcResponder;
class IGameServiceRpcServer;

/**
 * Interface for the GameServiceRpc module.
 */
class IGameServiceRpcModule
	: public IModuleInterface
{
public:
	static IGameServiceRpcModule* Get()
	{
		return static_cast<IGameServiceRpcModule*>(FModuleManager::Get().LoadModule("GameServiceRpc"));
	}

	virtual TSharedRef<IGameServiceRpcLocator> CreateLocator(const FString& DebugName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus) = 0;
	virtual TSharedRef<IGameServiceRpcResponder> CreateResponder(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus) = 0;
	virtual TSharedRef<IGameServiceRpcServer> CreateServer(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus) = 0;
};
