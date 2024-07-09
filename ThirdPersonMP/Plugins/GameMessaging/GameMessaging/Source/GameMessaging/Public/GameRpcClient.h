// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IMessageContext.h"
#include "Async/AsyncResult.h"

class IMessageBus;
class IMessageRpcClient;
class IGameRpcServerLocator;

class GAMEMESSAGING_API FGameRpcClient
{
public:
	FGameRpcClient(const FString& InClientName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InMessageBus);

	virtual ~FGameRpcClient();

	virtual bool IsConnected() const;

	virtual TSharedPtr<IMessageRpcClient> GetRpcClient() const
	{
		return RpcClient;
	}

	virtual void OnServerLocated(const FMessageAddress& ServerAddress);
	virtual void OnServerLost(const FMessageAddress& ServerAddress);

	template <typename RpcType, typename... P>
	TAsyncResult<typename RpcType::FResult> Call(P... Params)
	{
		if (RpcClient)
		{
			return RpcClient->Call<RpcType, P...>(Params...);
		}
		return TAsyncResult<typename RpcType::FResult>();
	}

	FName GetDebugName() const
	{
		return FName(ClientName);
	}

protected:
	FString ClientName;
	TSharedPtr<IMessageRpcClient> RpcClient;
	TSharedPtr<IGameRpcServerLocator> RpcServerLocator;
};
