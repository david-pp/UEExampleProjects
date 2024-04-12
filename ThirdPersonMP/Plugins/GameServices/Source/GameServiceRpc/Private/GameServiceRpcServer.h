// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IGameServiceRpcServer;

class FGameServiceRpcServerFactory
{
public:
	static TSharedRef<IGameServiceRpcServer> Create();
};
