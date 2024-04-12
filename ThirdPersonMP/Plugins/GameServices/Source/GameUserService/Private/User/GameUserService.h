// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IGameService.h"
#include "IMessageBus.h"

class FGameUserServiceFactory
{
public:
	static TSharedRef<IGameService> Create(const TSharedPtr<IMessageBus, ESPMode::ThreadSafe>& ServiceBus);
}; 
