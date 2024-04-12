#include "GameServiceRpcClient.h"

#include "IGameServiceRpcLocator.h"
#include "IGameServiceRpcModule.h"
#include "IMessageRpcClient.h"
#include "IMessagingRpcModule.h"

FGameServiceRpcClient::FGameServiceRpcClient(const FString& InClientName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& InMessageBus) : ClientName(InClientName)
{
	IGameServiceRpcModule* ServiceRpcModule = IGameServiceRpcModule::Get();
	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));
	if (ServiceRpcModule && MessagingRpcModule)
	{
		RpcClient = MessagingRpcModule->CreateRpcClient(InClientName, InMessageBus);
		if (RpcClient)
		{
			// create a locator to find rpc server
			RpcServerLocator = ServiceRpcModule->CreateLocator(InClientName + TEXT("Locator"), ServiceKey, InMessageBus);
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

FGameServiceRpcClient::~FGameServiceRpcClient()
{
	if (RpcClient)
	{
		RpcClient->Disconnect();
		RpcClient.Reset();
	}
}

bool FGameServiceRpcClient::IsConnected() const
{
	if (RpcClient)
	{
		return RpcClient->IsConnected();
	}
	return false;
}

void FGameServiceRpcClient::OnServerLocated(const FMessageAddress& ServerAddress)
{
	if (RpcClient)
	{
		RpcClient->Connect(ServerAddress);
	}

	UE_LOG(LogTemp, Warning, TEXT("RpcClient:%s - OnServerLocated ... %s"), *ClientName, *ServerAddress.ToString());
}

void FGameServiceRpcClient::OnServerLost(const FMessageAddress& ServerAddress)
{
	if (RpcClient)
	{
		RpcClient->Disconnect();
	}

	UE_LOG(LogTemp, Warning, TEXT("RpcClient:%s - OnServerLost ... %s"), *ClientName, *ServerAddress.ToString());
}
