﻿// Fill out your copyright notice in the Description page of Project Settings.

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
#include "IMessageTransport.h"
#include "NatsClient.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"
#include "MessageDebugActor.generated.h"

class IGameServiceRpcLocator;
class IGameServiceRpcClient;

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

class FBusEndpoint
{
public:
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;
	void HandleHeartBeatMessage(const FDebugServiceHeartBeat& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
	void HandlePingMessage(const FDebugServicePing& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
	void HandlePongMessage(const FDebugServicePong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
};

UCLASS()
class THIRDPERSONMP_API AMessageDebugPingClientCharacter : public AThirdPersonMPCharacter
{
	GENERATED_BODY()

public:
	AMessageDebugPingClientCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	//
	// User Service Demo
	//
	UFUNCTION(BlueprintCallable)
	void CreateUserRpcClient();
	UFUNCTION(BlueprintCallable)
	void AsyncGetUserDetails();
	UFUNCTION(BlueprintCallable)
	void AsyncGetUserDetails2();

	TSharedPtr<IGameServiceRpcClient> UserServiceRpcClient;
	TSharedPtr<IMessageRpcClient> UserRpcClient;
	TSharedPtr<IGameServiceRpcLocator> UserRpcLocator;


	//
	// NATs Demo
	//
	UFUNCTION(BlueprintCallable)
	void CreateNatsClient(FString InName, FString NatsURL = TEXT("nats://127.0.0.1:4222"), FString DefaultSubject=TEXT("Default"));
	void HandleNatsMessage(const char* DataPtr, int32 DataLength);

	UFUNCTION(BlueprintCallable)
	void PublishMessage(FString InName, FString Subject, const FString& Message);
	
	TMap<FString, TSharedPtr<INatsClient>> NatsClients;
	
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Messaging)
	FName ClientName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category=Messaging)
	FString UserName;

	/** Holds a weak pointer to the message bus. */
	TWeakPtr<IMessageBus, ESPMode::ThreadSafe> MessageBusPtr;
	/** Holds the message endpoint. */
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;
};
