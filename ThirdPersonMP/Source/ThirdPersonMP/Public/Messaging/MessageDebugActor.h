// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IMessageContext.h"
#include "IMessageBus.h"
#include "IMessageBridge.h"
#include "MessageEndpoint.h"
#include "IMessageRpcServer.h"
#include "IMessageRpcClient.h"
#include "DebugingMessages.h"
#include "IMyRpcLocator.h"
#include "IMyRpcResponder.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"
#include "MessageDebugActor.generated.h"

class IGameServiceRpcLocator;

UCLASS()
class THIRDPERSONMP_API AMessageDebugPingServiceActor : public AActor
{
	GENERATED_BODY()

public:
	AMessageDebugPingServiceActor();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void StartService();

	UFUNCTION(BlueprintCallable)
	void StopService();

	/** Handles  messages. */
	void HandlePingMessage(const FDebugServicePing& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
	void HandleHeartBeatMessage(const FDebugServiceHeartBeat& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Messaging)
	FName ServiceName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Messaging)
	FString UserName;

	/** Holds a weak pointer to the message bus. */
	TWeakPtr<IMessageBus, ESPMode::ThreadSafe> MessageBusPtr;
	/** Holds the message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;
};

UCLASS()
class AMessageDebugBusActor : public AActor
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	
	
};


UCLASS()
class THIRDPERSONMP_API AMessageDebugPingClientCharacter : public AThirdPersonMPCharacter
{
	GENERATED_BODY()

public:
	AMessageDebugPingClientCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//
	// Messaging Demos
	//
	
	UFUNCTION(BlueprintCallable)
	void CreateBus(FString BusName, FString ListenEndpoint, TArray<FString> ConnectToEndpoints);

	UFUNCTION(BlueprintCallable)
	void CreateBusEndPoint(FString BusName, FString EndPointName, bool bSubscribeMsg=false);

	UFUNCTION(BlueprintCallable)
	void EndPointSendHeartBeat(FString EndPointName);

	void HandleHeartBeatMessage(const FDebugServiceHeartBeat& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

	TMap<FString,TSharedPtr<IMessageBus, ESPMode::ThreadSafe>> Buses;
	TMap<FString,TSharedPtr<IMessageBridge, ESPMode::ThreadSafe>> Bridges;
	TMap<FString, TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe>> EndPoints;


	//
	// RPC Demos
	//
	UFUNCTION(BlueprintCallable)
	void CreateBusRpcClient(FString BusName, FString RpcClientName, FString RpcServerToConnect);

	UFUNCTION(BlueprintCallable)
	void CreateBusRpcServer(FString BusName, FString RpcServerName, bool bRegisterHandlers=true);

	UFUNCTION(BlueprintCallable)
	FMyResult MyRpcDemo(FString RpcClientName, FString Param1, int32 Param2);

	UFUNCTION(BlueprintCallable)
	void AsyncMyRpcDemo(FString RpcClientName, FString Param1, int32 Param2);
	UPROPERTY(BlueprintAssignable, EditAnywhere)
	FOnMyRpcComplete OnMyRpcComplete;


	
	// Server Handle Rpc
	TAsyncResult<FMyResult> HandleMyRpc(const FMyRpcRequest& Request);

	TMap<FString, TSharedPtr<IMessageRpcClient>> RpcClients;
	TMap<FString, TSharedPtr<IMessageRpcServer>> RpcServers;

	/** RPC server locator per client */
	TMap<FString, TSharedPtr<IMyRpcLocator>> RpcLocators;
	TMap<FString, TSharedPtr<IMyRpcResponder>> RpcResponders;


	//
	// User Service Demo
	//
	UFUNCTION(BlueprintCallable)
	void CreateUserRpcClient();
	UFUNCTION(BlueprintCallable)
	void AsyncGetUserDetails();

	TSharedPtr<IMessageRpcClient> UserRpcClient;
	TSharedPtr<IGameServiceRpcLocator> UserRpcLocator;
	
public:
	UFUNCTION(BlueprintCallable)
	void StartClient();

	UFUNCTION(BlueprintCallable)
	void StopClient();


	UFUNCTION(BlueprintCallable)
	void SendPing();

	UFUNCTION(BlueprintCallable)
	void SendHearBeat();

	/** Handles FDebugServicePong messages. */
	void HandlePongMessage(const FDebugServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
	void OnBusNotification(const FMessageBusNotification& Notification);

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Messaging)
	FName ClientName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Messaging)
	FString UserName;

	/** Holds a weak pointer to the message bus. */
	TWeakPtr<IMessageBus, ESPMode::ThreadSafe> MessageBusPtr;
	/** Holds the message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;
};

