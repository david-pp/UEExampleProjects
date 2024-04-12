// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IGameService;
class IMessageBus;
class IGameServiceLocator;

class IGameServiceEngine
{
public:
	/** Virtual destructor. */
	virtual ~IGameServiceEngine()
	{
	}

	template <typename ServiceType>
	TSharedPtr<ServiceType> GetServiceByName(const FString& ServiceName)
	{
		return StaticCastSharedPtr<ServiceType>(GetService(ServiceName));
	}

public:
	virtual void Init() = 0;
	virtual void Start() = 0;
	virtual void Stop() = 0;

	virtual TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetServiceBus() const = 0;

	virtual TSharedPtr<IGameServiceLocator> GetServiceLocator() const = 0;

	virtual TSharedPtr<IGameService> GetService(const FString& ServiceName) = 0;
};
