// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageBus.h"

class IGameServiceEngine
{
public:
	/** Virtual destructor. */
	virtual ~IGameServiceEngine()
	{
	}

	virtual void Init() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetServiceBus() const = 0;
};

