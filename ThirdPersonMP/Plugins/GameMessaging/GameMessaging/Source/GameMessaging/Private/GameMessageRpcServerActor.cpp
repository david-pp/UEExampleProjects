// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMessageRpcServerActor.h"

#include "GameMessages.h"
#include "GameRpcServer.h"
#include "GameRpcServerResponder.h"
#include "IMessageBus.h"
#include "IMessagingModule.h"
#include "IMessagingRpcModule.h"


// Sets default values
AGameMessageRpcServerActor::AGameMessageRpcServerActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

TSharedPtr<IMessageBus, ESPMode::ThreadSafe> AGameMessageRpcServerActor::GetMessageBus() const
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


// Called when the game starts or when spawned
void AGameMessageRpcServerActor::BeginPlay()
{
	Super::BeginPlay();

	auto MessageBus = GetMessageBus();
	if (!MessageBus)
	{
		UE_LOG(LogTemp, Error, TEXT("AGameMessageRpcServerActor - Invalid message bus : %s"), *MessageBusName.ToString());
		return;
	}

	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));
	if (!MessagingRpcModule) return;

	// RpcServer = MessagingRpcModule->CreateRpcServer(RpcServerName, MessageBus.ToSharedRef());
	RpcServer = MakeShared<FGameRpcServer>(RpcServerName, MessageBus.ToSharedRef());
	if (RpcServer)
	{
		// Register Rpc Handler
		RpcServer->RegisterHandler<FGameRpcDebug>(this, &AGameMessageRpcServerActor::HandleDebugRpc);
	}

	// create server responder
	RpcServerResponder = IGameRpcServerResponder::Create(FName(RpcServerName), MessageBus.ToSharedRef());
	if (RpcServerResponder)
	{
		RpcServerResponder->AddRpcServerToLookup(RpcServerName, RpcServer);
	}
}

// Called every frame
void AGameMessageRpcServerActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameMessageRpcServerActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	RpcServer = nullptr;
	RpcServerResponder = nullptr;
}

TAsyncResult<FGameRpcDebugResult> AGameMessageRpcServerActor::HandleDebugRpc(const FGameRpcDebugRequest& Request)
{
	UE_LOG(LogTemp, Log, TEXT("HandleDebugRpc@%s : %s, %d"), *RpcServerName, *Request.Param1, Request.Param2);

	FGameRpcDebugResult Result;
	Result.RetValString = Request.Param1;
	Result.RetVal = Request.Param2;
	return TAsyncResult<FGameRpcDebugResult>(Result);
}
