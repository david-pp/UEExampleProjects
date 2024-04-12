// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IGameServiceRpcLocator;

class FGameServiceRpcLocatorFactory
{
public:
	static TSharedRef<IGameServiceRpcLocator> Create();
};
