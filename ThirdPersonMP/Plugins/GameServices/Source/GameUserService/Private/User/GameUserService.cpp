// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameUserService.h"

#include "User/IGameUserService.h"
#include "GameUserMessages.h"
#include "IGameServiceRpcModule.h"
#include "IGameServiceRpcResponder.h"
#include "IGameServiceRpcServer.h"
#include "IMessageBus.h"
#include "IMessageRpcClient.h"
#include "IMessageRpcServer.h"

class FGameUserService : public IGameUserService
{
public:
	virtual ~FGameUserService()
	{
	}

	virtual bool IsAvailable() const override
	{
		return true;
	}

	virtual TSharedPtr<IGameServiceRpcServer> GetRpcServer() const override
	{
		return RpcServer;
	}

	virtual TAsyncResult<FGameUserDetails> GetUserDetails() override
	{
		FGameUserDetails UserDetails;
		UserDetails.DisplayName = FText::FromString(TEXT("David"));
		return TAsyncResult<FGameUserDetails>(UserDetails);
	}

public:
	friend FGameUserServiceFactory;

	FGameUserService(const TSharedPtr<IMessageBus, ESPMode::ThreadSafe> ServiceBus)
	{
		IGameServiceRpcModule* ServiceRpcModule = IGameServiceRpcModule::Get();
		if (ServiceRpcModule)
		{
			RpcServer = ServiceRpcModule->CreateServer(TEXT("UserService"), ServiceBus.ToSharedRef());
			if (RpcServer && RpcServer->GetMessageServer())
			{
				IMessageRpcServer* MessageRpcServer = RpcServer->GetMessageServer();
				if (MessageRpcServer)
				{
					MessageRpcServer->RegisterHandler<FGameUserGetUserDetails>(this, &FGameUserService::HandleGetUserDetails);
				}
			}
		}
	}

	TAsyncResult<FGameUserDetails> HandleGetUserDetails(const FGameUserGetUserDetailsRequest& Request)
	{
		return GetUserDetails();
	}

protected:
	// TSharedPtr<IMessageBus, ESPMode::ThreadSafe> ServiceBus;
	TSharedPtr<IGameServiceRpcServer> RpcServer;
	TSharedPtr<IGameServiceRpcResponder> RpcResponder;
};

TSharedRef<IGameService> FGameUserServiceFactory::Create(const TSharedPtr<IMessageBus, ESPMode::ThreadSafe>& ServiceBus)
{
	return MakeShareable(new FGameUserService(ServiceBus));
}
