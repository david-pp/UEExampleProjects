// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameUserProxy.h"

#include "User/IGameUserService.h"
#include "GameUserMessages.h"
#include "IGameServiceRpcClient.h"
#include "IGameServiceRpcModule.h"
#include "IGameServicesModule.h"
#include "IMessageRpcClient.h"

class FGameUserProxy : public IGameUserService
{
public:
	virtual ~FGameUserProxy()
	{
		if (RpcClient)
		{
			RpcClient.Reset();
		}
	}

	virtual bool IsAvailable() const override
	{
		return RpcClient->IsConnected();
	}

public:
	virtual TAsyncResult<FGameUserDetails> GetUserDetails() override
	{
		UE_LOG(LogTemp, Warning, TEXT("GetUserDetails@Proxy -----"));
		return RpcClient->Call<FGameUserGetUserDetails>();
	}

private:
	FGameUserProxy()
	{
		IGameServiceEngine* ServiceEngine = IGameServicesModule::GetServiceEngine();
		IGameServiceRpcModule* ServiceRpcModule = IGameServiceRpcModule::Get();
		if (ServiceRpcModule)
		{
			RpcClient = ServiceRpcModule->CreateClient(TEXT("UserRpcClient"), TEXT("UserService"), ServiceEngine->GetServiceBus().ToSharedRef());
		}
	}

private:
	TSharedPtr<IGameServiceRpcClient> RpcClient;

	friend FGameUserProxyFactory;
};

TSharedRef<IGameService> FGameUserProxyFactory::Create()
{
	return MakeShareable(new FGameUserProxy);
}
