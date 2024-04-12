// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageRpcClient.h"
#include "IGameService.h"

class FGameUserProxyFactory
{
public:
	static TSharedRef<IGameService> Create();
}; 
