// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IGameServiceRpcResponder;

class FGameServiceRpcResponderFactory
{
public:
	static TSharedRef<IGameServiceRpcResponder> Create();
};
