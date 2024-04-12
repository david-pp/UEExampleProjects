// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IGameServiceRpcModule.h"
#include "GameServiceRpcLocator.h"
#include "GameServiceRpcResponder.h"
#include "GameServiceRpcServer.h"

class FGameServiceRpcModule
	: public IGameServiceRpcModule
{
public:

	// IModuleInterface interface
	virtual void StartupModule() override { }
	virtual void ShutdownModule() override { }

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

public:

	virtual TSharedRef<IGameServiceRpcLocator> CreateLocator() override
	{
		return FGameServiceRpcLocatorFactory::Create();
	}

	virtual TSharedRef<IGameServiceRpcResponder> CreateResponder() override
	{
		return FGameServiceRpcResponderFactory::Create();
	}

	virtual TSharedRef<IGameServiceRpcServer> CreateServer() override
	{
		return FGameServiceRpcServerFactory::Create();
	}

};

IMPLEMENT_MODULE(FGameServiceRpcModule, GameServiceRpc);
