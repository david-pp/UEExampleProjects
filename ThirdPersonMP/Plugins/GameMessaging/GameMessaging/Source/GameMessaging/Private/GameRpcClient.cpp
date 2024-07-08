// Fill out your copyright notice in the Description page of Project Settings.


#include "GameRpcClient.h"

#include "GameRpcServerLocator.h"
#include "IMessageRpcClient.h"
#include "IMessagingRpcModule.h"

FGameRpcClient::FGameRpcClient(const FString& InClientName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InMessageBus) : ClientName(InClientName)
{
	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));
	if (MessagingRpcModule)
	{
		RpcClient = MessagingRpcModule->CreateRpcClient(InClientName, InMessageBus);
		if (RpcClient)
		{
			// create a locator to find rpc server
			FString LocatorName = InClientName + TEXT("Locator");
			RpcServerLocator = IGameRpcServerLocator::Create(FName(LocatorName), ServiceKey, InMessageBus);
			if (RpcServerLocator)
			{
				RpcServerLocator->OnServerLocated().BindLambda([this]()
				{
					OnServerLocated(RpcServerLocator->GetServerAddress());
				});

				RpcServerLocator->OnServerLost().BindLambda([this]()
				{
					OnServerLost(RpcServerLocator->GetServerAddress());
				});
			}
		}
	}
}

FGameRpcClient::~FGameRpcClient()
{
	if (RpcClient)
	{
		RpcClient->Disconnect();
		RpcClient.Reset();
	}
}

bool FGameRpcClient::IsConnected() const
{
	if (RpcClient)
	{
		return RpcClient->IsConnected();
	}
	return false;
}

void FGameRpcClient::OnServerLocated(const FMessageAddress& ServerAddress)
{
	if (RpcClient)
	{
		RpcClient->Connect(ServerAddress);
	}

	UE_LOG(LogTemp, Warning, TEXT("RpcClient:%s - OnServerLocated ... %s"), *ClientName, *ServerAddress.ToString());
}

void FGameRpcClient::OnServerLost(const FMessageAddress& ServerAddress)
{
	if (RpcClient)
	{
		RpcClient->Disconnect();
	}

	UE_LOG(LogTemp, Warning, TEXT("RpcClient:%s - OnServerLost ... %s"), *ClientName, *ServerAddress.ToString());
}
