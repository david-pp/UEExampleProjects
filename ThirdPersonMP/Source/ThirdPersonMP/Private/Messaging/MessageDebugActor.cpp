// Fill out your copyright notice in the Description page of Project Settings.


#include "Messaging/MessageDebugActor.h"

#include "GameUserMessages.h"
#include "IGameServiceRpcClient.h"
#include "IGameServiceRpcLocator.h"
#include "IGameServiceRpcModule.h"
#include "IGameServicesModule.h"
#include "IMessagingModule.h"
#include "IMessagingRpcModule.h"
#include "MessageBridgeBuilder.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
#include "NatsClientModule.h"
#include "Messaging/DebugingMessages.h"
#include "Messaging/TcpMessageTransport.h"

AMessageDebugPingServiceActor::AMessageDebugPingServiceActor()
{
	ServiceName = TEXT("PingService");
}

void AMessageDebugPingServiceActor::BeginPlay()
{
	Super::BeginPlay();
	StartService();
}

void AMessageDebugPingServiceActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopService();
	Super::EndPlay(EndPlayReason);
}

void AMessageDebugPingServiceActor::StartService()
{
	MessageBusPtr = IMessagingModule::Get().GetDefaultBus();
	auto MessageBus = MessageBusPtr.Pin();
	if (!MessageBus.IsValid())
	{
		return;
	}

	MessageEndpoint = FMessageEndpoint::Builder(ServiceName).Handling<FDebugServicePing>(this, &AMessageDebugPingServiceActor::HandlePingMessage).Handling<FDebugServiceHeartBeat>(this, &AMessageDebugPingServiceActor::HandleHeartBeatMessage);

	if (MessageEndpoint)
	{
		MessageEndpoint->Subscribe<FDebugServicePing>();
		MessageEndpoint->Subscribe<FDebugServiceHeartBeat>();
	}
}

void AMessageDebugPingServiceActor::StopService()
{
	if (MessageEndpoint)
	{
		MessageEndpoint.Reset();
	}
}

void AMessageDebugPingServiceActor::HandlePingMessage(const FDebugServicePing& PingMessage, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (!MessageEndpoint) return;

	UE_LOG(LogTemp, Log, TEXT("HandlePingMessage@%s : From %s@%s"), *ServiceName.ToString(), *PingMessage.UserName, *Context->GetSender().ToString());

	FDebugServicePong* Message = new FDebugServicePong();
	{
		Message->SericeName = ServiceName.ToString();
		Message->UserName = PingMessage.UserName;
		Message->BuildDate = FApp::GetBuildDate();
		Message->DeviceName = FPlatformProcess::ComputerName();
		Message->InstanceId = FApp::GetInstanceId();
		Message->InstanceName = FApp::GetInstanceName();
		Message->PlatformName = FPlatformProperties::PlatformName();
		Message->SessionId = FApp::GetSessionId();
		Message->SessionName = FApp::GetSessionName();
		Message->SessionOwner = FApp::GetSessionOwner();
		Message->Standalone = FApp::IsStandalone();
	}

	MessageEndpoint->Send(Message, Context->GetSender());
}

void AMessageDebugPingServiceActor::HandleHeartBeatMessage(const FDebugServiceHeartBeat& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	UE_LOG(LogTemp, Log, TEXT("HandleHeartBeatMessage@%s - From : %s@%s"), *ServiceName.ToString(), *Message.ServiceName, *Context->GetSender().ToString());
}

// --------------------------------------

void AMessageDebugBusActor::BeginPlay()
{
	Super::BeginPlay();
}

AMessageDebugPingClientCharacter::AMessageDebugPingClientCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ClientName = TEXT("PingClient");
}

void AMessageDebugPingClientCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartClient();
}

void AMessageDebugPingClientCharacter::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	for (auto& KV : NatsClients)
	{
		KV.Value->Tick();
	}
}

void AMessageDebugPingClientCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	StopClient();

	for (auto& KV : NatsClients)
	{
		KV.Value->Close();
	}
}

void AMessageDebugPingClientCharacter::CreateBus(FString BusName, FString ListenEndpointString, TArray<FString> ConnectToEndpointStrings)
{
	if (Buses.Find(BusName)) return;

	// Create Bus
	TSharedPtr<IMessageBus, ESPMode::ThreadSafe> Bus = IMessagingModule::Get().CreateBus(BusName);
	Buses.Add(BusName, Bus);

	// Listen 
	FIPv4Endpoint ListenEndpoint;
	if (!FIPv4Endpoint::Parse(ListenEndpointString, ListenEndpoint))
	{
		if (!ListenEndpointString.IsEmpty())
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid setting for ListenEndpoint '%s', listening disabled"), *ListenEndpointString);
		}

		ListenEndpoint = FIPv4Endpoint::Any;
	}

	// Connections
	TArray<FIPv4Endpoint> ConnectToEndpoints;
	for (FString& ConnectToEndpointString : ConnectToEndpointStrings)
	{
		FIPv4Endpoint ConnectToEndpoint;
		if (FIPv4Endpoint::Parse(ConnectToEndpointString, ConnectToEndpoint) || FIPv4Endpoint::FromHostAndPort(ConnectToEndpointString, ConnectToEndpoint))
		{
			ConnectToEndpoints.Add(ConnectToEndpoint);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Invalid entry for ConnectToEndpoint '%s', ignoring"), *ConnectToEndpointString);
		}
	}

	// Create Bridge with Transport
	TSharedRef<FTcpMessageTransport, ESPMode::ThreadSafe> Transport = MakeShareable(new FTcpMessageTransport(ListenEndpoint, ConnectToEndpoints, 2.0));

	TSharedPtr<IMessageBridge, ESPMode::ThreadSafe> MessageBridge = FMessageBridgeBuilder(Bus.ToSharedRef()).UsingTransport(Transport);
	if (MessageBridge)
	{
		Bridges.Add(BusName, MessageBridge);
	}
}

void AMessageDebugPingClientCharacter::CreateBusEndPoint(FString BusName, FString EndPointName, bool bSubscribeMsg)
{
	auto Bus = Buses.FindRef(BusName);
	if (!Bus) return;

	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> Endpoint = FMessageEndpoint::Builder(FName(EndPointName), Bus.ToSharedRef()).Handling<FDebugServiceHeartBeat>(this, &AMessageDebugPingClientCharacter::HandleHeartBeatMessage);

	if (Endpoint)
	{
		if (bSubscribeMsg)
		{
			Endpoint->Subscribe<FDebugServiceHeartBeat>();
		}

		EndPoints.Add(EndPointName, Endpoint);
	}
}

void AMessageDebugPingClientCharacter::EndPointSendHeartBeat(FString EndPointName)
{
	auto EndPoint = EndPoints.FindRef(EndPointName);
	if (EndPoint.IsValid())
	{
		auto Msg = FMessageEndpoint::MakeMessage<FDebugServiceHeartBeat>();
		Msg->ServiceName = EndPointName;
		EndPoint->Publish(Msg, EMessageScope::All);
	}
}

void AMessageDebugPingClientCharacter::HandleHeartBeatMessage(const FDebugServiceHeartBeat& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	UE_LOG(LogTemp, Log, TEXT("HandleHeartBeatMessage - From : %s@%s"), *Message.ServiceName, *Context->GetSender().ToString());
}

void AMessageDebugPingClientCharacter::CreateBusRpcClient(FString BusName, FString RpcClientName, FString RpcServerToConnect)
{
	auto Bus = Buses.FindRef(BusName);
	if (!Bus) return;

	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));
	if (MessagingRpcModule)
	{
		TSharedPtr<IMessageRpcClient> RpcClient = MessagingRpcModule->CreateRpcClient(RpcClientName, Bus.ToSharedRef());
		if (RpcClient)
		{
			// connect to server
			auto RpcServer = RpcServers.FindRef(RpcServerToConnect);
			if (RpcServer)
			{
				RpcClient->Connect(RpcServer->GetAddress());
			}

			RpcClients.Add(RpcClientName, RpcClient);

			// create a locator to find rpc server
			TSharedPtr<IMyRpcLocator> RpcLocator = IMyRpcLocator::Create(FName(RpcClientName), RpcServerToConnect, Bus.ToSharedRef());
			if (RpcLocator)
			{
				RpcLocators.Add(RpcClientName, RpcLocator);

				RpcLocator->OnServerLocated().BindLambda([RpcClient, RpcLocator]()
				{
					UE_LOG(LogTemp, Warning, TEXT("OnServerLocated ... %s"), *RpcLocator->GetServerAddress().ToString());
					RpcClient->Connect(RpcLocator->GetServerAddress());
				});

				RpcLocator->OnServerLost().BindLambda([RpcClient, RpcLocator]()
				{
					UE_LOG(LogTemp, Warning, TEXT("OnServerLost ... %s"), *RpcLocator->GetServerAddress().ToString());
					RpcClient->Disconnect();
				});
			}
		}
	}
}

void AMessageDebugPingClientCharacter::CreateBusRpcServer(FString BusName, FString RpcServerName, bool bRegisterHandlers)
{
	auto Bus = Buses.FindRef(BusName);
	if (!Bus) return;

	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));
	if (MessagingRpcModule)
	{
		// TSharedPtr<IMessageRpcServer> RpcServer = MessagingRpcModule->CreateRpcServer(RpcServerName, Bus.ToSharedRef());
		TSharedPtr<IMessageRpcServer> RpcServer = MakeShared<FMyRpcServerImpl>(RpcServerName, Bus.ToSharedRef());
		if (RpcServer)
		{
			if (bRegisterHandlers)
			{
				RpcServer->RegisterHandler<FMyRpc>(this, &AMessageDebugPingClientCharacter::HandleMyRpc);
			}
			RpcServers.Add(RpcServerName, RpcServer);
		}
	}

	TSharedPtr<IMyRpcResponder> RpcResponder = IMyRpcResponder::Create(FName(RpcServerName), Bus.ToSharedRef());
	if (RpcResponder)
	{
		RpcResponders.Add(RpcServerName, RpcResponder);

		RpcResponder->OnLookup().BindLambda([this](const FString& ProductionKey)
		{
			auto RpcServer = RpcServers.FindRef(ProductionKey);
			return RpcServer;
		});
	}
}

FMyResult AMessageDebugPingClientCharacter::MyRpcDemo(FString RpcClientName, FString Param1, int32 Param2)
{
	auto RpcClient = RpcClients.FindRef(RpcClientName);
	if (RpcClient)
	{
		TAsyncResult<FMyResult> AsyncResult = RpcClient->Call<FMyRpc>(Param1, Param2);
		// AsyncResult.GetFuture().Wait();
		AsyncResult.GetFuture().WaitFor(FTimespan::FromSeconds(5.0f));
		if (AsyncResult.GetFuture().IsReady())
		{
			return AsyncResult.GetFuture().Get();
		}
	}

	return FMyResult();
}

void AMessageDebugPingClientCharacter::AsyncMyRpcDemo(FString RpcClientName, FString Param1, int32 Param2)
{
	auto RpcClient = RpcClients.FindRef(RpcClientName);
	if (RpcClient)
	{
		TAsyncResult<FMyResult> AsyncResult = RpcClient->Call<FMyRpc>(Param1, Param2);
		TFuture<FMyResult>& Future = const_cast<TFuture<FMyResult>&>(AsyncResult.GetFuture());

		Future.Then([this](TFuture<FMyResult> Result)
		{
			OnMyRpcComplete.Broadcast(Result.Get());
		});
	}
}

TAsyncResult<FMyResult> AMessageDebugPingClientCharacter::HandleMyRpc(const FMyRpcRequest& Request)
{
	UE_LOG(LogTemp, Log, TEXT("HanldeMyRpc -- %s, %d"), *Request.Param1, Request.Param2);

	FMyResult Result;
	Result.RetValString = Request.Param1;
	Result.RetVal = Request.Param2 + 1;
	return TAsyncResult<FMyResult>(Result);
}

void AMessageDebugPingClientCharacter::CreateUserRpcClient()
{
	IGameServiceEngine* ServiceEngine = IGameServicesModule::GetServiceEngine();
	IGameServiceRpcModule* ServiceRpcModule = IGameServiceRpcModule::Get();
	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));

	if (MessagingRpcModule && ServiceRpcModule && ServiceEngine && ServiceEngine->GetServiceBus())
	{
		UserServiceRpcClient = ServiceRpcModule->CreateClient(TEXT("UserRpcClient"), TEXT("UserService"), ServiceEngine->GetServiceBus().ToSharedRef());
		// UserRpcClient = MessagingRpcModule->CreateRpcClient(TEXT("UserRpcClient"), ServiceEngine->GetServiceBus().ToSharedRef());
		// if (UserRpcClient)
		// {
		// 	// create a locator to find rpc server
		// 	UserRpcLocator = ServiceRpcModule->CreateLocator(TEXT("UserServiceLocator"), TEXT("UserService"), ServiceEngine->GetServiceBus().ToSharedRef());
		// 	if (UserRpcLocator)
		// 	{
		// 		UserRpcLocator->OnServerLocated().BindLambda([=]()
		// 		{
		// 			UE_LOG(LogTemp, Warning, TEXT("OnServerLocated ... %s"), *UserRpcLocator->GetServerAddress().ToString());
		// 			UserRpcClient->Connect(UserRpcLocator->GetServerAddress());
		// 		});
		//
		// 		UserRpcLocator->OnServerLost().BindLambda([=]()
		// 		{
		// 			UE_LOG(LogTemp, Warning, TEXT("OnServerLost ... %s"), *UserRpcLocator->GetServerAddress().ToString());
		// 			UserRpcClient->Disconnect();
		// 		});
		// 	}
		// }
	}
}

void AMessageDebugPingClientCharacter::AsyncGetUserDetails()
{
	IGameServiceEngine* ServiceEngine = IGameServicesModule::GetServiceEngine();
	if (ServiceEngine)
	{
		auto UserService = ServiceEngine->GetServiceByName<IGameUserService>(TEXT("GameUserProxy"));
		if (UserService)
		{
			TAsyncResult<FGameUserDetails> AsyncResult = UserService->GetUserDetails();
			TFuture<FGameUserDetails>& Future = const_cast<TFuture<FGameUserDetails>&>(AsyncResult.GetFuture());

			Future.Then([this](TFuture<FGameUserDetails> InFuture)
			{
				if (InFuture.IsReady())
				{
					FGameUserDetails Result = InFuture.Get();
					UE_LOG(LogTemp, Log, TEXT("AsyncGetUserDetails Complete : %s"), *Result.DisplayName.ToString());
				}
			});
		}
	}

	// if (UserServiceRpcClient)
	// {
	// 	TAsyncResult<FGameUserDetails> AsyncResult = UserServiceRpcClient->Call<FGameUserGetUserDetails>();
	// 	TFuture<FGameUserDetails>& Future = const_cast<TFuture<FGameUserDetails>&>(AsyncResult.GetFuture());
	// 	
	// 	Future.Then([this](TFuture<FGameUserDetails> InFuture)
	// 	{
	// 		if (InFuture.IsReady())
	// 		{
	// 			FGameUserDetails Result = InFuture.Get();
	// 			UE_LOG(LogTemp, Log, TEXT("AsyncGetUserDetails Complete : %s"), *Result.DisplayName.ToString());
	// 		}
	// 	});
	// }


	// if (UserRpcClient)
	// {
	// 	TAsyncResult<FGameUserDetails> AsyncResult = UserRpcClient->Call<FGameUserGetUserDetails>();
	// 	TFuture<FGameUserDetails>& Future = const_cast<TFuture<FGameUserDetails>&>(AsyncResult.GetFuture());
	// 	
	// 	Future.Then([this](TFuture<FGameUserDetails> InFuture)
	// 	{
	// 		if (InFuture.IsReady())
	// 		{
	// 			FGameUserDetails Result = InFuture.Get();
	// 			UE_LOG(LogTemp, Log, TEXT("AsyncGetUserDetails Complete : %s"), *Result.DisplayName.ToString());
	// 		}
	// 	});
	// }
}

void AMessageDebugPingClientCharacter::AsyncGetUserDetails2()
{
	IGameServiceEngine* ServiceEngine = IGameServicesModule::GetServiceEngine();
	if (ServiceEngine)
	{
		auto UserService = ServiceEngine->GetServiceByName<IGameUserService>(TEXT("IGameUserService"));
		if (UserService)
		{
			TAsyncResult<FGameUserDetails> AsyncResult = UserService->GetUserDetails();
			TFuture<FGameUserDetails>& Future = const_cast<TFuture<FGameUserDetails>&>(AsyncResult.GetFuture());

			Future.Then([this](TFuture<FGameUserDetails> InFuture)
			{
				if (InFuture.IsReady())
				{
					FGameUserDetails Result = InFuture.Get();
					UE_LOG(LogTemp, Log, TEXT("AsyncGetUserDetails Complete : %s"), *Result.DisplayName.ToString());
				}
			});
		}
	}
}

void AMessageDebugPingClientCharacter::CreateNatsClient(FString InName, FString NatsURL, FString DefaultSubject)
{
	auto NatsClient = INatsClientModule::Get().CreateNatsClient();
	if (NatsClient)
	{
		if (!NatsClient->ConnectTo(NatsURL))
		{
			return;
		}

		NatsClients.Add(InName, NatsClient);

		if (!DefaultSubject.IsEmpty())
		{
			bool bOk = NatsClient->Subscribe(DefaultSubject, [this](const char* DataPtr, int32 DataLength)
			{
				this->HandleNatsMessage(DataPtr, DataLength);
			});
		}
	}
}

void AMessageDebugPingClientCharacter::HandleNatsMessage(const char* DataPtr, int32 DataLength)
{
	UE_LOG(LogTemp, Log, TEXT("HandleNatsMessage : %s"), ANSI_TO_TCHAR(DataPtr));
}

void AMessageDebugPingClientCharacter::PublishMessage(FString InName, FString Subject, const FString& Message)
{
	TSharedPtr<INatsClient> NatsClient = NatsClients.FindRef(InName);
	if (NatsClient)
	{
		NatsClient->Publish(Subject, TCHAR_TO_ANSI(*Message), Message.Len());
	}
}

void AMessageDebugPingClientCharacter::StartClient()
{
	MessageBusPtr = IMessagingModule::Get().GetDefaultBus();

	auto MessageBus = MessageBusPtr.Pin();
	if (!MessageBus.IsValid())
	{
		return;
	}

	MessageEndpoint = FMessageEndpoint::Builder(ClientName).Handling<FDebugServicePong>(this, &AMessageDebugPingClientCharacter::HandlePongMessage).NotificationHandling(FOnBusNotification::CreateUObject(this, &AMessageDebugPingClientCharacter::OnBusNotification));
}

void AMessageDebugPingClientCharacter::StopClient()
{
	if (MessageEndpoint)
	{
		MessageEndpoint.Reset();
	}
}

void AMessageDebugPingClientCharacter::SendPing()
{
	if (MessageEndpoint.IsValid())
	{
		// MessageEndpoint->Send()
		MessageEndpoint->Publish(new FDebugServicePing(UserName), EMessageScope::Process);
	}
}

void AMessageDebugPingClientCharacter::SendHearBeat()
{
	if (MessageEndpoint.IsValid())
	{
		auto Msg = FMessageEndpoint::MakeMessage<FDebugServiceHeartBeat>();
		Msg->ServiceName = ClientName.ToString();
		MessageEndpoint->Publish(Msg, EMessageScope::All);
	}
}

void AMessageDebugPingClientCharacter::HandlePongMessage(const FDebugServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	UE_LOG(LogTemp, Log, TEXT("HandlePongMessage - %s@%s"), *Message.UserName, *Message.PlatformName);
}

void AMessageDebugPingClientCharacter::OnBusNotification(const FMessageBusNotification& Notification)
{
	if (Notification.NotificationType == EMessageBusNotification::Registered)
	{
		UE_LOG(LogTemp, Log, TEXT("OnBusNotification - Register : %s"), *Notification.RegistrationAddress.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("OnBusNotification - UnRegister : %s"), *Notification.RegistrationAddress.ToString());
	}
}
