// Fill out your copyright notice in the Description page of Project Settings.


#include "Messaging/MessageDebugActor.h"

#include "GameNatsMessageTransport.h"
#include "GameNatsMessageTransport.h"
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
#include "GameTcpMessageTransport.h"
#include "IGameServiceLocator.h"

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
	// StartClient();
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
	// StopClient();

	for (auto& KV : NatsClients)
	{
		KV.Value->Close();
	}
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
	if (!ServiceEngine) return;

	auto UserProxy = ServiceEngine->GetProxyByName<IGameUserService>(TEXT("UserProxy"));
	if (UserProxy)
	{
		TAsyncResult<FGameUserDetails> AsyncResult = UserProxy->GetUserDetails();
		TFuture<FGameUserDetails>& Future = const_cast<TFuture<FGameUserDetails>&>(AsyncResult.GetFuture());

		Future.Then([this](TFuture<FGameUserDetails> InFuture)
		{
			if (InFuture.IsReady())
			{
				FGameUserDetails Result = InFuture.Get();
				UE_LOG(LogGameServices, Log, TEXT("AsyncGetUserDetails - Complete : %s"), *Result.DisplayName.ToString());
			}
		});
	}
	else
	{
		UE_LOG(LogGameServices, Warning, TEXT("AsyncGetUserDetails - Invalid User Proxy"));
	}
}

void AMessageDebugPingClientCharacter::AsyncGetUserDetails2()
{
	IGameServiceEngine* ServiceEngine = IGameServicesModule::GetServiceEngine();
	if (!ServiceEngine) return;

	auto UserService = ServiceEngine->GetServiceByName<IGameUserService>(TEXT("UserService"));
	if (UserService)
	{
		TAsyncResult<FGameUserDetails> AsyncResult = UserService->GetUserDetails();
		TFuture<FGameUserDetails>& Future = const_cast<TFuture<FGameUserDetails>&>(AsyncResult.GetFuture());

		Future.Then([this](TFuture<FGameUserDetails> InFuture)
		{
			if (InFuture.IsReady())
			{
				FGameUserDetails Result = InFuture.Get();
				UE_LOG(LogGameServices, Log, TEXT("AsyncGetUserDetails2 Complete : %s"), *Result.DisplayName.ToString());
			}
		});
	}
	else
	{
		UE_LOG(LogGameServices, Warning, TEXT("AsyncGetUserDetails2 - Invalid User Service"));
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
