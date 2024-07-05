// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMessageBusActor.h"

#include "GameNatsMessageTransport.h"
#include "IMessagingModule.h"
#include "MessageBridgeBuilder.h"
#include "GameTcpMessageTransport.h"
#include "Interfaces/IPv4/IPv4Endpoint.h"


// Sets default values
AGameMessageBusActor::AGameMessageBusActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bEnableMessageBus = true;
	bEnableTcpBridge = true;
	bEnableNatsBridge = false;

	TcpListenEndpoint = TEXT("127.0.0.1:5555");
	NatsServerURL = TEXT("nats://127.0.0.1:4222");
}

// Called when the game starts or when spawned
void AGameMessageBusActor::BeginPlay()
{
	Super::BeginPlay();

	if (!bEnableMessageBus) return;

	// Creates a new message bus.
	MessageBus = IMessagingModule::Get().CreateBus(MessageBusName);
	if (!MessageBus) return;

	// Create a message bridge with tcp transport layer
	if (bEnableTcpBridge)
	{
		// Listen 
		FIPv4Endpoint ListenEndpoint;
		if (!FIPv4Endpoint::Parse(TcpListenEndpoint, ListenEndpoint))
		{
			if (!TcpListenEndpoint.IsEmpty())
			{
				UE_LOG(LogTemp, Warning, TEXT("Invalid setting for ListenEndpoint '%s', listening disabled"), *TcpListenEndpoint);
			}

			ListenEndpoint = FIPv4Endpoint::Any;
		}

		// Connections
		TArray<FIPv4Endpoint> ConnectToEndpoints;
		for (FString& ConnectToEndpointString : TcpConnectToEndpoints)
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
		TcpTransport = MakeShareable(new FTcpMessageTransport(ListenEndpoint, ConnectToEndpoints, 2.0));
		TcpBridge = FMessageBridgeBuilder(MessageBus.ToSharedRef()).UsingTransport(TcpTransport.ToSharedRef());
		if (TcpBridge)
		{
			// TODO: ...
		}
	}

	// Create a message bridge with nats transport layer
	if (bEnableNatsBridge)
	{
		FMessageAddress BridgeAddress = FMessageAddress::NewAddress();

		// Create Bridge with Transport
		NatsTransport = MakeShareable(new FGameNatsMessageTransport(MessageBusName, NatsServerURL));
		NatsBridge = IMessagingModule::Get().CreateBridge(BridgeAddress, MessageBus.ToSharedRef(), NatsTransport.ToSharedRef());
		if (NatsBridge)
		{
			NatsBridge->Enable();
		}
	}
}

void AGameMessageBusActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	MessageBus = nullptr;

	TcpBridge = nullptr;
	TcpTransport = nullptr;
	
	NatsBridge = nullptr;
	NatsTransport = nullptr;
}

// Called every frame
void AGameMessageBusActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (NatsTransport)
	{
		NatsTransport->DispatchMessageCallbacks();
	}
}
