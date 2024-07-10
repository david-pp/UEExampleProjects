#include "GameServiceEngine.h"
#include "GameMessaging.h"
#include "IGameServiceLocator.h"
#include "IGameServicesModule.h"
#include "IMessagingModule.h"
#include "JsonObjectConverter.h"
#include "MessageBridgeBuilder.h"
#include "Containers/Ticker.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"
#include "Misc/App.h"
#include "Misc/FileHelper.h"
#include "Misc/TypeContainer.h"
#include "User/IGameUserService.h"

#include "GameNatsMessageTransport.h"
#include "GameTcpMessageTransport.h"

bool FGameServicesEngine::Init()
{
	// Load config
	const FString ServiceConfigFile = GetSettingFileName();
	const FString FilePath = FPaths::ProjectConfigDir() / ServiceConfigFile;
	if (!FPaths::FileExists(FilePath)) // not exist then create the config file
	{
		SaveSettingToJsonFile(ServiceConfigFile);
	}

	if (!LoadSettingFromJsonFile(ServiceConfigFile))
	{
		UE_LOG(LogGameServices, Error, TEXT("FGameServicesEngine::Init - load config file failed : %s"), *FilePath);
		return false;
	}

	// Create service bus
	if (!InitServiceBus(EngineSettings.ServiceBus))
	{
		UE_LOG(LogGameServices, Error, TEXT("FGameServicesEngine::Init - init service bus failed"));
		return false;
	}

	// Init services/proxies
	if (!InitServices())
	{
		UE_LOG(LogGameServices, Error, TEXT("FGameServicesEngine::Init - init services/proxies failed"));
		return false;
	}

	// Init rpc server responder
	if (!InitRpcServerResponder())
	{
		UE_LOG(LogGameServices, Error, TEXT("FGameServicesEngine::Init - init rpc server responder failed"));
		return false;
	}

	UE_LOG(LogGameServices, Log, TEXT("FGameServicesEngine::Init - success"));
	return true;
}

void FGameServicesEngine::Start()
{
	// Start tick 
	UE_LOG(LogGameServices, Log, TEXT("FGameServicesEngine::Start - Tick"));
	TickerHandle = FTicker::GetCoreTicker().AddTicker(TEXT("GameServiceEngine"), 0.0f, [this](float DeltaTime)
	{
		this->Tick();
		return true;
	});

	UE_LOG(LogGameServices, Log, TEXT("FGameServicesEngine::Start - Services/Proxies"));

	// Services
	for (const FGameServiceSettings& ServiceSettings : EngineSettings.GameServices)
	{
		if (ServiceLocator && ServiceSettings.bCreateServiceOnStart)
		{
			auto GameService = ServiceLocator->GetServiceByName<IGameService>(ServiceSettings.ServiceName.ToString(), ServiceSettings.ServiceWildcard);
			if (GameService)
			{
				UE_LOG(LogGameServices, Log, TEXT("FGameServicesEngine::Start - GameService : %s, sucess"), *GameService->GetDebugName().ToString());
			}
			else
			{
				UE_LOG(LogGameServices, Warning, TEXT("FGameServicesEngine::Start - GameService : %s, failed"), *ServiceSettings.ServiceName.ToString());
			}
		}
	}

	// Proxies
	for (const FGameServiceSettings& ServiceSettings : EngineSettings.GameProxies)
	{
		if (ProxyLocator && ServiceSettings.bCreateServiceOnStart)
		{
			auto GameProxy = ProxyLocator->GetServiceByName<IGameService>(ServiceSettings.ServiceName.ToString(), ServiceSettings.ServiceWildcard);
			if (GameProxy)
			{
				UE_LOG(LogGameServices, Log, TEXT("FGameServicesEngine::Start - GameProxy : %s, sucess"), *GameProxy->GetDebugName().ToString());
			}
			else
			{
				UE_LOG(LogGameServices, Warning, TEXT("FGameServicesEngine::Start - GameProxy : %s, failed"), *ServiceSettings.ServiceName.ToString());
			}
		}
	}

	// dump engine services/proxies
	DumpServices();
}

void FGameServicesEngine::Tick()
{
	// Process nats transport
	if (NatsTransport)
	{
		NatsTransport->DispatchMessageCallbacks();
	}
}

void FGameServicesEngine::Stop()
{
	if (TickerHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	}

	ServiceLocator = nullptr;
	ProxyLocator = nullptr;
	RpcServerResponder = nullptr;

	if (ServiceBus)
	{
		ServiceBus.Reset();
	}

	TcpBridge = nullptr;
	TcpTransport = nullptr;

	NatsBridge = nullptr;
	NatsTransport = nullptr;
}

// ----------------------- config -------------------------

FString FGameServicesEngine::GetSettingFileName()
{
	// Command line argument is the first priority!
	FString ServiceConfigCmdParam;
	if (FParse::Value(FCommandLine::Get(), TEXT("GameServices="), ServiceConfigCmdParam))
	{
		const FString ServiceConfigFileByCmd = FString::Printf(TEXT("GameServices-%s.json"), *ServiceConfigCmdParam);
		const FString FilePath = FPaths::ProjectConfigDir() / ServiceConfigFileByCmd;
		if (FPaths::FileExists(FilePath))
		{
			return ServiceConfigFileByCmd;
		}
	}

	// Then Build
	{
		const FString ServiceConfigFileByBuild = FString::Printf(TEXT("GameServices-%s.json"), LexToString(FApp::GetBuildConfiguration()));
		const FString FilePath = FPaths::ProjectConfigDir() / ServiceConfigFileByBuild;
		if (FPaths::FileExists(FilePath))
		{
			return ServiceConfigFileByBuild;
		}
	}

	return TEXT("GameServices.json");
}

bool FGameServicesEngine::LoadSettingFromJsonFile(const FString& JsonFileName)
{
	const FString FilePath = FPaths::ProjectConfigDir() / JsonFileName;
	if (FPaths::FileExists(FilePath))
	{
		FString JsonString;
		if (FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &EngineSettings))
			{
				UE_LOG(LogGameServices, Log, TEXT("Load services settings sucess : \n%s"), *JsonString);
				return true;
			}
		}
	}
	return false;
}

bool FGameServicesEngine::SaveSettingToJsonFile(const FString& JsonFileName)
{
	const FString FilePath = FPaths::ProjectConfigDir() / JsonFileName;

	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(EngineSettings, JsonString))
	{
		if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
		{
			UE_LOG(LogGameServices, Log, TEXT("Save services settings to : %s"), *FilePath);
			return true;
		}
	}

	return false;
}


// ----------------------- init  -------------------------

bool FGameServicesEngine::InitServiceBus(const FGameServiceMessageBusSettings& BusSettings)
{
	// Creates a new message bus.
	ServiceBus = IMessagingModule::Get().CreateBus(BusSettings.MessageBusName);
	if (!ServiceBus)
	{
		UE_LOG(LogGameServices, Error, TEXT("InitServiceBus - failed to create service bus : %s"), *BusSettings.MessageBusName);
		return false;
	}

	// Create a message bridge with tcp transport layer
	if (BusSettings.bEnableTcpBridge)
	{
		// Listen 
		FIPv4Endpoint ListenEndpoint;
		if (!FIPv4Endpoint::Parse(BusSettings.TcpListenEndpoint, ListenEndpoint))
		{
			if (!BusSettings.TcpListenEndpoint.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("InitServiceBus - Invalid setting for ListenEndpoint '%s', listening disabled"), *BusSettings.TcpListenEndpoint);
			}

			ListenEndpoint = FIPv4Endpoint::Any;
		}

		// Connections
		TArray<FIPv4Endpoint> ConnectToEndpoints;
		for (const FString& ConnectToEndpointString : BusSettings.TcpConnectToEndpoints)
		{
			FIPv4Endpoint ConnectToEndpoint;
			if (FIPv4Endpoint::Parse(ConnectToEndpointString, ConnectToEndpoint) || FIPv4Endpoint::FromHostAndPort(ConnectToEndpointString, ConnectToEndpoint))
			{
				ConnectToEndpoints.Add(ConnectToEndpoint);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("InitServiceBus - Invalid entry for ConnectToEndpoint '%s', ignoring"), *ConnectToEndpointString);
			}
		}

		// Create Bridge with Transport
		TcpTransport = MakeShareable(new FGameTcpMessageTransport(ListenEndpoint, ConnectToEndpoints, 2.0));
		TcpBridge = FMessageBridgeBuilder(ServiceBus.ToSharedRef()).UsingTransport(TcpTransport.ToSharedRef());
		if (TcpBridge)
		{
			// TODO: ...
		}
	}

	// Create a message bridge with nats transport layer
	if (BusSettings.bEnableNatsBridge)
	{
		FMessageAddress BridgeAddress = FMessageAddress::NewAddress();

		// Create Bridge with Transport
		NatsTransport = MakeShareable(new FGameNatsMessageTransport(BusSettings.MessageBusName, BusSettings.NatsServerURL));
		NatsBridge = IMessagingModule::Get().CreateBridge(BridgeAddress, ServiceBus.ToSharedRef(), NatsTransport.ToSharedRef());
		if (NatsBridge)
		{
			NatsBridge->Enable();
		}
	}

	return true;
}

bool FGameServicesEngine::InitServices()
{
	if (!ServiceBus)
	{
		UE_LOG(LogGameServices, Error, TEXT("InitServices - invalid service bus"));
		return false;
	}

	IGameServicesModule* ServiceModule = IGameServicesModule::Get();
	if (ServiceModule)
	{
		ServiceDependencies = MakeShareable(new FTypeContainer);
		if (ServiceDependencies)
		{
			// TODO: add more dependencies
		}

		// Services
		ServiceLocator = ServiceModule->CreateLocator(ServiceDependencies.ToSharedRef());
		if (ServiceLocator)
		{
			for (const FGameServiceSettings& ServiceSettings : EngineSettings.GameServices)
			{
				ServiceLocator->Configure(ServiceSettings.ServiceName.ToString(), ServiceSettings.ServiceWildcard, ServiceSettings.ServiceModule);
			}
		}

		// Proxies
		ProxyLocator = ServiceModule->CreateLocator(ServiceDependencies.ToSharedRef());
		if (ProxyLocator)
		{
			for (const FGameServiceSettings& ServiceSettings : EngineSettings.GameProxies)
			{
				ProxyLocator->Configure(ServiceSettings.ServiceName.ToString(), ServiceSettings.ServiceWildcard, ServiceSettings.ServiceModule);
			}
		}
	}

	return true;
}

bool FGameServicesEngine::InitRpcServerResponder()
{
	// create rpc server responder
	IGameMessagingModule* GameMessagingModule = IGameMessagingModule::Get();
	if (!GameMessagingModule)
	{
		UE_LOG(LogGameServices, Error, TEXT("InitRpcServerResponder - invalid GameMessaging module"));
		return false;
	}

	static FName RpcServerResponderName = TEXT("RpcServerPresponder");
	RpcServerResponder = GameMessagingModule->CreateResponder(RpcServerResponderName, ServiceBus.ToSharedRef());
	if (!RpcServerResponder)
	{
		return false;
	}

	// ServerKey = Type:Instance
	// RpcServerResponder->OnLookup().BindLambda([this](const FString& ServerKey)
	// {
	// 	if (ServerKey == TEXT("UserService"))
	// 	{
	// 		auto UserService = ServiceLocator->GetService<IGameUserService>();
	// 		if (UserService)
	// 		{
	// 			return UserService->GetRpcServer();
	// 		}
	// 	}
	// 	return TSharedPtr<IGameServiceRpcServer>();
	// });

	return true;
}

void FGameServicesEngine::DumpServices()
{
	if (ServiceLocator)
	{
		ServiceLocator->DumpServices();
	}

	if (ProxyLocator)
	{
		ProxyLocator->DumpServices();
	}
}
