// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class FGameRpcServer;
class FGameRpcClient;

/**
 * Base class for game service
 */
GAMESERVICES_API class IGameService
{
public:
	IGameService(TSharedPtr<FGameRpcServer> InRpcServer) : RpcServer(InRpcServer)
	{
	}

	IGameService(TSharedPtr<FGameRpcClient> InRpcClient) : RpcClient(InRpcClient)
	{
	}

	/** Virtual destructor. */
	virtual ~IGameService()
	{
		RpcServer = nullptr;
		RpcClient = nullptr;
	}

	virtual void OnCreate()
	{
	}

	virtual void OnDestroy()
	{
		RpcServer = nullptr;
		RpcClient = nullptr;
	}

	virtual bool IsAvailable() const
	{
		// 	return RpcClient->IsConnected();
		return RpcServer.IsValid() || RpcClient.IsValid();
	}

	bool IsService() const
	{
		return RpcServer.IsValid();
	}

	bool IsProxy() const
	{
		return RpcClient.IsValid();
	}

public:
	TSharedPtr<FGameRpcServer> GetRpcServer() const
	{
		return RpcServer;
	}

	TSharedPtr<FGameRpcClient> GetRpcClient() const
	{
		return RpcClient;
	}

protected:
	TSharedPtr<FGameRpcServer> RpcServer;
	TSharedPtr<FGameRpcClient> RpcClient;
};

/**
 * Base class for game service's proxy
 */
GAMESERVICES_API class IGameServiceProxy
{
public:
	/** Virtual destructor. */
	virtual ~IGameServiceProxy()
	{
	}

	virtual void OnCreate()
	{
	}

	virtual void OnDestroy()
	{
	}

	virtual bool IsAvailable() const
	{
		return true;
	}

public:


protected:
};
