// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameRpcServer.h"
#include "GameRpcClient.h"

class FGameRpcServer;
class FGameRpcClient;

/**
 * Base class for game service
 */
GAMESERVICES_API class IGameService
{
public:
	IGameService(const TSharedPtr<FGameRpcServer>& InRpcServer) : RpcServer(InRpcServer)
	{
	}

	IGameService(const TSharedPtr<FGameRpcClient>& InRpcClient) : RpcClient(InRpcClient)
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

	FName GetDebugName() const
	{
		if (RpcServer) return RpcServer->GetDebugName();
		if (RpcClient) return RpcClient->GetDebugName();
		return NAME_None;
	}

protected:
	TSharedPtr<FGameRpcServer> RpcServer;
	TSharedPtr<FGameRpcClient> RpcClient;
};

