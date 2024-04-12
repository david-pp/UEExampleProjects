// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IGameServiceRpcServer;

class IGameService
{
public:
	/** Virtual destructor. */
	virtual ~IGameService()
	{
	}

	virtual bool IsAvailable() const = 0;

	virtual TSharedPtr<IGameServiceRpcServer> GetRpcServer() const
	{
		return nullptr;
	}
};
