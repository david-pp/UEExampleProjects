// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameRpcServer.h"

class IMessageBus;
DECLARE_DELEGATE_RetVal_OneParam(TSharedPtr<FGameRpcServer>, FOnGameRpcServerLookup, const FString& /*ServerKey*/)

/**
 * Interface for GameService RPC responders.
 */
class IGameRpcServerResponder
{
public:
	/** factory method to create a Server Responder */
	static TSharedRef<IGameRpcServerResponder> Create(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus);

public:
	/** Virtual destructor. */
	virtual ~IGameRpcServerResponder()
	{
	}

	/** Get a delegate that is executed when a look-up for an RPC server occurs. */
	virtual FOnGameRpcServerLookup& OnLookup() = 0;

	/** Add/Remove rpc server from lookup table */
	virtual void AddRpcServerToLookup(const FString& ServerKey, TSharedPtr<FGameRpcServer> RpcServer) = 0;
	virtual void RemoveRpcServerFromLookup(const FString& ServerKey) = 0;
};
