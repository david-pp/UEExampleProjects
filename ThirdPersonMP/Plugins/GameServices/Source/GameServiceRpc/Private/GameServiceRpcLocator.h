// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IMessageBus;
class IGameServiceRpcLocator;

class FGameServiceRpcLocatorFactory
{
public:
	static TSharedRef<IGameServiceRpcLocator> Create(const FString& DebugName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus);
};
