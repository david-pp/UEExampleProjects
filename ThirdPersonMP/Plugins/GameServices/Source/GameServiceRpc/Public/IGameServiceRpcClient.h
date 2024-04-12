// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "IMessageRpcClient.h"
#include "Async/AsyncResult.h"

class IMessageRpcClient;
struct FMessageAddress;

/**
 * Interface for GameService RPC Client
 */
class IGameServiceRpcClient
{
public:
	/** Virtual destructor. */
	virtual ~IGameServiceRpcClient()
	{
	}

	virtual bool IsConnected() const = 0;

	virtual TSharedPtr<IMessageRpcClient> GetRpcClient() const = 0;

	template <typename RpcType, typename... P>
	TAsyncResult<typename RpcType::FResult> Call(P... Params)
	{
		TSharedPtr<IMessageRpcClient> Client = GetRpcClient();
		if (Client)
		{
			return Client->Call<RpcType, P...>(Params...);
		}

		return TAsyncResult<typename RpcType::FResult>();
	}
};
