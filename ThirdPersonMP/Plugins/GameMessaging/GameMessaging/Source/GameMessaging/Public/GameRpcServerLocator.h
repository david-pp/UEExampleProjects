// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class IMessageBus;
struct FMessageAddress;

/** Defines interval at which the Game RPC server is being located (in seconds). */
#define GAME_RPC_LOCATE_INTERVAL 5.0
/** Defines the time after which Game RPC servers time out (in seconds). */
#define GAME_RPC_LOCATE_TIMEOUT 15.0

/**
 * Interface for Game RPC server locators.
 */
class IGameRpcServerLocator
{
public:
	static TSharedRef<IGameRpcServerLocator> Create(const FName& InName, const FString& ServerKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InBus);

	/**
	 * Get the address of the located Game RPC server.
	 *
	 * @return The RPC server's message address, or invalid if no server found.
	 * @see OnServerLocated, OnServerLost
	 */
	virtual const FMessageAddress& GetServerAddress() const = 0;

public:
	/**
	 * Get a delegate that is executed when an RPC server has been located.
	 *
	 * @return The delegate.
	 * @see GetServerAddress, OnServerLost
	 */
	virtual FSimpleDelegate& OnServerLocated() = 0;

	/**
	 * Get a delegate that is executed when an RPC server has been located.
	 *
	 * @return The delegate.
	 * @see GetServerAddress, OnServerLocated
	 */
	virtual FSimpleDelegate& OnServerLost() = 0;

	virtual ~IGameRpcServerLocator()
	{
	}
};
