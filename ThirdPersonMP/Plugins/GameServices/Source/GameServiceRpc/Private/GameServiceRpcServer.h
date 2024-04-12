// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageBus.h"

class IGameServiceRpcServer;

class FGameServiceRpcServerFactory
{
public:
	static TSharedRef<IGameServiceRpcServer> Create(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus);
};
