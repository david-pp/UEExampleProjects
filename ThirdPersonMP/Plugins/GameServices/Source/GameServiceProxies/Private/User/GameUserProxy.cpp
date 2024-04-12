// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameUserProxy.h"

#include "User/IGameUserService.h"
#include "GameUserMessages.h"
#include "IMessageRpcClient.h"

class FGameUserProxy : public IGameUserService
{
public:
	virtual ~FGameUserProxy()
	{
	}

	virtual bool IsAvailable() const override
	{
		return RpcClient->IsConnected();
	}

public:
	virtual TAsyncResult<FGameUserDetails> GetUserDetails() override
	{
		return RpcClient->Call<FGameUserGetUserDetails>();
	}

private:
	FGameUserProxy(const TSharedRef<IMessageRpcClient>& InRpcClient) : RpcClient(InRpcClient)
	{
	}

private:
	TSharedRef<IMessageRpcClient> RpcClient;

	friend FGameUserProxyFactory;
};

TSharedRef<IGameService> FGameUserProxyFactory::Create(const TSharedRef<IMessageRpcClient>& RpcClient)
{
	return MakeShareable(new FGameUserProxy(RpcClient));
}
