// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMessageEndpointActor.h"

#include "IMessageBus.h"
#include "IMessagingModule.h"
#include "MessageEndpoint.h"
#include "MessageEndpointBuilder.h"


// Sets default values
AGameMessageEndpointActor::AGameMessageEndpointActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AGameMessageEndpointActor::BeginPlay()
{
	Super::BeginPlay();

	auto MessageBus = GetMessageBus();
	if (!MessageBus)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid message bus : %s"), *MessageBusName.ToString());
		return;
	}

	MessageEndpoint = FMessageEndpoint::Builder(MessageEndpointName, MessageBus.ToSharedRef())
		.Handling<FGameEndpointHeartBeat>(this, &AGameMessageEndpointActor::HandleHeartBeatMessage)
		.Handling<FGameEndpointPing>(this, &AGameMessageEndpointActor::HandlePingMessage)
		.Handling<FGameEndpointPong>(this, &AGameMessageEndpointActor::HandlePongMessage);
	if (MessageEndpoint)
	{
		MessageEndpoint->Subscribe<FGameEndpointHeartBeat>();
	}

	// heartbeat timer
	GetWorldTimerManager().SetTimer(HearBeatTimer, [this]()
	{
		BroadcastHeartBeat();
	}, HeartBeatRate, true, 2.0);
}

// Called every frame
void AGameMessageEndpointActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameMessageEndpointActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void AGameMessageEndpointActor::BroadcastHeartBeat()
{
	if (MessageEndpoint)
	{
		// publish heart beat
		FGameEndpointHeartBeat* Message = FMessageEndpoint::MakeMessage<FGameEndpointHeartBeat>();
		Message->EndpointName = MessageEndpointName;
		MessageEndpoint->Publish(Message, EMessageScope::All);
	}
}

void AGameMessageEndpointActor::SendPingToAddress(const FString& Address)
{
	FMessageAddress Recipient;
	FMessageAddress::Parse(Address, Recipient);
	if (MessageEndpoint)
	{
		// send ping
		MessageEndpoint->Send(new FGameEndpointPing(MessageEndpointName), Recipient);
	}
}

void AGameMessageEndpointActor::SendPingToEndpointActor(AGameMessageEndpointActor* TargetEndpointActor)
{
	if (TargetEndpointActor && TargetEndpointActor->MessageEndpoint)
	{
		const FMessageAddress& Recipient = TargetEndpointActor->MessageEndpoint->GetAddress();
		if (MessageEndpoint)
		{
			// send ping
			MessageEndpoint->Send(new FGameEndpointPing(MessageEndpointName), Recipient);
		}
	}
}

TSharedPtr<IMessageBus, ESPMode::ThreadSafe> AGameMessageEndpointActor::GetMessageBus() const
{
	for (auto Bus : IMessagingModule::Get().GetAllBuses())
	{
		if (Bus->GetName() == MessageBusName.ToString())
		{
			return Bus;
		}
	}

	return nullptr;
}

void AGameMessageEndpointActor::HandleHeartBeatMessage(const FGameEndpointHeartBeat& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (!MessageEndpoint) return;

	// FString RecipientStr = FString::JoinBy(Context->GetRecipients(), TEXT("+"), &FMessageAddress::ToString);
	UE_LOG(LogTemp, Log, TEXT("HandleHeartBeatMessage@%s - From : %s,%s"),
		*MessageEndpoint->GetDebugName().ToString(),
		*Message.EndpointName.ToString(), *Context->GetSender().ToString());
}

void AGameMessageEndpointActor::HandlePingMessage(const FGameEndpointPing& Ping, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (!MessageEndpoint) return;

	UE_LOG(LogTemp, Log, TEXT("HandlePingMessage@%s - From %s@%s"), *MessageEndpoint->GetDebugName().ToString(), *Ping.EndpointName.ToString(), *Context->GetSender().ToString());

	// send pong back to sender
	FGameEndpointPong* Message = FMessageEndpoint::MakeMessage<FGameEndpointPong>();
	Message->EndpointName = MessageEndpoint->GetDebugName();
	Message->BuildDate = FApp::GetBuildDate();
	Message->DeviceName = FPlatformProcess::ComputerName();
	Message->InstanceId = FApp::GetInstanceId();
	Message->InstanceName = FApp::GetInstanceName();
	Message->PlatformName = FPlatformProperties::PlatformName();
	Message->SessionId = FApp::GetSessionId();
	Message->SessionName = FApp::GetSessionName();
	Message->SessionOwner = FApp::GetSessionOwner();
	Message->Standalone = FApp::IsStandalone();

	MessageEndpoint->Send(Message, Context->GetSender());
}

void AGameMessageEndpointActor::HandlePongMessage(const FGameEndpointPong& Pong, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context)
{
	if (!MessageEndpoint) return;
	UE_LOG(LogTemp, Log, TEXT("HandlePongMessage@%s - From %s@%s"), *MessageEndpoint->GetDebugName().ToString(), *Pong.EndpointName.ToString(), *Context->GetSender().ToString());
}
