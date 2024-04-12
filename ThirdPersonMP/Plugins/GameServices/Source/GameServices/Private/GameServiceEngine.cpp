#include "GameServiceEngine.h"
#include "IGameServiceEngine.h"
#include "IGameServiceLocator.h"
#include "IGameServiceRpcLocator.h"
#include "IGameServiceRpcModule.h"
#include "IGameServicesModule.h"
#include "IGameServiceRpcResponder.h"
#include "IMessageRpcClient.h"
#include "IMessagingModule.h"
#include "IMessagingRpcModule.h"
#include "Misc/TypeContainer.h"
#include "User/IGameUserService.h"

void FGameServicesEngine::InitializeGameServices()
{
	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));
	IGameServiceRpcModule* ServiceRpcModule = static_cast<IGameServiceRpcModule*>(FModuleManager::Get().LoadModule("GameServiceRpc"));
	IGameServicesModule* ServiceModule = IGameServicesModule::Get();

	if (MessagingRpcModule && ServiceRpcModule && ServiceModule)
	{
		// Initialize Game services
		ServiceRpcClient = MessagingRpcModule->CreateRpcClient();

		// ServiceRpcLocator = ServiceRpcModule->CreateLocator();
		// {
		// 	ServiceRpcLocator->OnServerLocated().BindLambda([=]()
		// 	{
		// 		ServiceRpcClient->Connect(ServiceRpcLocator->GetServerAddress());
		// 	});
		// 	ServiceRpcLocator->OnServerLost().BindLambda([=]()
		// 	{
		// 		ServiceRpcClient->Disconnect();
		// 	});
		// }

		ServiceDependencies = MakeShareable(new FTypeContainer);
		{
			ServiceDependencies->RegisterInstance<IMessageRpcClient>(ServiceRpcClient.ToSharedRef());
		}

		ServiceLocator = ServiceModule->CreateLocator(ServiceDependencies.ToSharedRef());
		{
			ServiceLocator->Configure(TEXT("IGameUserService"), TEXT("*"), "GameServiceProxies");
		}
	}
	else
	{
		class FNullGameServiceLocator : public IGameServiceLocator
		{
			virtual void Configure(const FString& ServiceName, const FWildcardString& ProductId, const FName& ServiceModule) override
			{
			}

			virtual TSharedPtr<IGameService> GetService(const FString& ServiceName, const FString& ProductId) override
			{
				return nullptr;
			}
		};

		ServiceLocator = MakeShareable(new FNullGameServiceLocator());
	}
}

void FGameServicesEngine::InitializeGameServers()
{
	// Create Service Bus
	ServiceBus = IMessagingModule::Get().CreateBus(TEXT("ServiceBus"));
	if (!ServiceBus)
	{
		return;
	}

	IGameServiceRpcModule* ServiceRpcModule = IGameServiceRpcModule::Get();
	IGameServicesModule* ServiceModule = IGameServicesModule::Get();
	if (ServiceRpcModule && ServiceModule)
	{
		RpcResponder = ServiceRpcModule->CreateResponder(TEXT("ServiceResponder"), ServiceBus.ToSharedRef());
		if (RpcResponder)
		{
			// ServerKey = Type:Instance
			RpcResponder->OnLookup().BindLambda([this](const FString& ServerKey)
			{
				if (ServerKey == TEXT("UserService"))
				{
					auto UserService = ServiceLocator->GetService<IGameUserService>();
					if (UserService)
					{
						return UserService->GetRpcServer();
					}
				}
				return TSharedPtr<IGameServiceRpcServer>();
			});
		}
		
		ServiceDependencies = MakeShareable(new FTypeContainer);
		if (ServiceDependencies)
		{
			// TODO: add more dependencies
		}
		
		ServiceLocator = ServiceModule->CreateLocator(ServiceDependencies.ToSharedRef());
		if (ServiceLocator)
		{
			ServiceLocator->Configure(TEXT("IGameUserService"), TEXT("*"), "GameUserService");
			ServiceLocator->Configure(TEXT("GameUserProxy"), TEXT("*"), "GameServiceProxies");
		}
	}

	// TODO: remove
	Start();
}

void FGameServicesEngine::Start()
{
	TSharedPtr<IGameUserService> UserService = ServiceLocator->GetService<IGameUserService>();
	if (UserService)
	{
		
	}
}

void FGameServicesEngine::Stop()
{
	if (ServiceBus)
	{
		ServiceBus.Reset();
	}
}
