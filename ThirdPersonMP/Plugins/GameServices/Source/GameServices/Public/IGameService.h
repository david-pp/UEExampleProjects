// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IGameService
{
public:
	/** Virtual destructor. */
	virtual ~IGameService()
	{
	}

	virtual bool IsAvailable() const = 0;
};
