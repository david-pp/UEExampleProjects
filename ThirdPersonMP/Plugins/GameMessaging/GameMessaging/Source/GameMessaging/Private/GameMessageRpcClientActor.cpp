// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMessageRpcClientActor.h"

#include "GameRpcServerLocator.h"
#include "IMessageBus.h"
#include "IMessageRpcClient.h"
#include "IMessagingModule.h"
#include "IMessagingRpcModule.h"


// Sets default values
AGameMessageRpcClientActor::AGameMessageRpcClientActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

TSharedPtr<IMessageBus, ESPMode::ThreadSafe> AGameMessageRpcClientActor::GetMessageBus() const
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
void AGameMessageRpcClientActor::BeginPlay()
{
	Super::BeginPlay();

	auto MessageBus = GetMessageBus();
	if (!MessageBus)
	{
		UE_LOG(LogTemp, Error, TEXT("AGameMessageRpcClientActor - Invalid message bus : %s"), *MessageBusName.ToString());
		return;
	}

	IMessagingRpcModule* MessagingRpcModule = static_cast<IMessagingRpcModule*>(FModuleManager::Get().LoadModule("MessagingRpc"));
	if (!MessagingRpcModule) return;

	RpcClient = MessagingRpcModule->CreateRpcClient(RpcClientName.ToString(), MessageBus.ToSharedRef());
	if (RpcClient)
	{
		// create a locator to find rpc server
		FString LocatorName = RpcClientName.ToString() + TEXT("Locator");
		RpcServerLocator = IGameRpcServerLocator::Create(FName(LocatorName), RpcServerKey, MessageBus.ToSharedRef());
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

// Called every frame
void AGameMessageRpcClientActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void AGameMessageRpcClientActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}


void AGameMessageRpcClientActor::OnServerLocated(const FMessageAddress& ServerAddress)
{
	if (RpcClient)
	{
		RpcClient->Connect(ServerAddress);
	}

	UE_LOG(LogTemp, Warning, TEXT("RpcClient:%s - OnServerLocated ... %s"), *RpcClientName.ToString(), *ServerAddress.ToString());
}

void AGameMessageRpcClientActor::OnServerLost(const FMessageAddress& ServerAddress)
{
	if (RpcClient)
	{
		RpcClient->Disconnect();
	}

	UE_LOG(LogTemp, Warning, TEXT("RpcClient:%s - OnServerLost ... %s"), *RpcClientName.ToString(), *ServerAddress.ToString());
}

FGameRpcDebugResult AGameMessageRpcClientActor::DebugRpcDemo(FString Param1, int32 Param2)
{
	if (RpcClient)
	{
		TAsyncResult<FGameRpcDebugResult> AsyncResult = RpcClient->Call<FGameRpcDebug>(Param1, Param2);
		// AsyncResult.GetFuture().Wait();
		AsyncResult.GetFuture().WaitFor(FTimespan::FromSeconds(5.0f));
		if (AsyncResult.GetFuture().IsReady())
		{
			return AsyncResult.GetFuture().Get();
		}
	}

	return FGameRpcDebugResult();
}

void AGameMessageRpcClientActor::AsyncDebugRpcDemo(FString Param1, int32 Param2, const FOnGameRpcDebugComplete& OnComplete)
{
	if (RpcClient)
	{
		TAsyncResult<FGameRpcDebugResult> AsyncResult = RpcClient->Call<FGameRpcDebug>(Param1, Param2);
		TFuture<FGameRpcDebugResult>& Future = const_cast<TFuture<FGameRpcDebugResult>&>(AsyncResult.GetFuture());

		Future.Then([OnComplete](TFuture<FGameRpcDebugResult> Result)
		{
			OnComplete.ExecuteIfBound(Result.Get());
		});
	}
}
