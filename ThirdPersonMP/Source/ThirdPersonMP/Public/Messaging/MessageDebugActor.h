// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IMessageContext.h"
#include "IMessageBus.h"
#include "IMessageBridge.h"
#include "MessageEndpoint.h"
#include "DebugingMessages.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"
#include "MessageDebugActor.generated.h"

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

