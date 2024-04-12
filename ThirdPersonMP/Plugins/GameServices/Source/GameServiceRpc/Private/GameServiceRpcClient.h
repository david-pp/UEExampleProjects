#pragma once
#include "IGameServiceRpcClient.h"

class IMessageBus;
class IGameServiceRpcLocator;
class IMessageRpcClient;

class FGameServiceRpcClient : public IGameServiceRpcClient
{
public:
	FGameServiceRpcClient(const FString& InClientName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InMessageBus);

	virtual ~FGameServiceRpcClient() override;

	virtual bool IsConnected() const override;

	virtual TSharedPtr<IMessageRpcClient> GetRpcClient() const override
	{
		return RpcClient;
	}

	virtual void OnServerLocated(const FMessageAddress& ServerAddress);
	virtual void OnServerLost(const FMessageAddress& ServerAddress);

protected:
	FString ClientName;
	TSharedPtr<IMessageRpcClient> RpcClient;
	TSharedPtr<IGameServiceRpcLocator> RpcServerLocator;
};
