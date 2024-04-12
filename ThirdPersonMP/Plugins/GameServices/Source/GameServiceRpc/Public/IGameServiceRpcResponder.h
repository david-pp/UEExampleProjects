// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class IGameServiceRpcServer;

DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<IGameServiceRpcServer>, FOnGameServiceRpcLookup, const FString& /*ProductKey*/)


/**
 * Interface for GameService RPC responders.
 */
class IGameServiceRpcResponder
{
public:

	/** Get a delegate that is executed when a look-up for an RPC server occurs. */
	virtual FOnGameServiceRpcLookup& OnLookup() = 0;

public:

	/** Virtual destructor. */
	virtual ~IGameServiceRpcResponder() { }
};
