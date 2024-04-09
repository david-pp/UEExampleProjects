// Fill out your copyright notice in the Description page of Project Settings.


#include "Messaging/MessageDebugActor.h"

#include "IMessagingModule.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"
#include "Messaging/DebugingMessages.h"

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
	
	MessageEndpoint = FMessageEndpoint::Builder(ServiceName)
		.Handling<FDebugServicePing>(this, &AMessageDebugPingServiceActor::HandlePingMessage)
		.Handling<FDebugServiceHeartBeat>(this, &AMessageDebugPingServiceActor::HandleHeartBeatMessage);

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

AMessageDebugPingClientCharacter::AMessageDebugPingClientCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ClientName = TEXT("PingClient");
}

void AMessageDebugPingClientCharacter::BeginPlay()
{
	Super::BeginPlay();
	StartClient();
}

void AMessageDebugPingClientCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
	StopClient();
}

void AMessageDebugPingClientCharacter::StartClient()
{
	MessageBusPtr = IMessagingModule::Get().GetDefaultBus();

	auto MessageBus = MessageBusPtr.Pin();
	if (!MessageBus.IsValid())
	{
		return;
	}

	MessageEndpoint = FMessageEndpoint::Builder(ClientName)
		.Handling<FDebugServicePong>(this, &AMessageDebugPingClientCharacter::HandlePongMessage)
		.NotificationHandling(FOnBusNotification::CreateUObject(this, &AMessageDebugPingClientCharacter::OnBusNotification));
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

