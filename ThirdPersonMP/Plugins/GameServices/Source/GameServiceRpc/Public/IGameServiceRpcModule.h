// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
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

	virtual TSharedRef<IGameServiceRpcLocator> CreateLocator() = 0;
	virtual TSharedRef<IGameServiceRpcResponder> CreateResponder() = 0;
	virtual TSharedRef<IGameServiceRpcServer> CreateServer() = 0;
};
