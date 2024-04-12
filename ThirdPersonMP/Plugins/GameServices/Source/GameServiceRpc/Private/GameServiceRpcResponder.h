// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageBus.h"

class IGameServiceRpcResponder;

class FGameServiceRpcResponderFactory
{
public:
	static TSharedRef<IGameServiceRpcResponder> Create(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus);
};
