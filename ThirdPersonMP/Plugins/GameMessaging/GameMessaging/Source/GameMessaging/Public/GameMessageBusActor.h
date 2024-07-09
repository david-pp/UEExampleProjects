// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameMessageBusActor.generated.h"

class IMessageBus;
class IMessageBridge;
class FGameTcpMessageTransport;
class FGameNatsMessageTransport;

UCLASS()
class GAMEMESSAGING_API AGameMessageBusActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGameMessageBusActor();

protected:
	// ~AActor Interface
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FString MessageBusName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	uint8 bEnableMessageBus : 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	uint8 bEnableTcpBridge : 1;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	uint8 bEnableNatsBridge : 1;

	/** Tcp Listen Server */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="bEnableTcpBridge", ExposeOnSpawn=true), Category=TcpTransport)
	FString TcpListenEndpoint;
	/** Tcp Servers to connect */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="bEnableTcpBridge", ExposeOnSpawn=true), Category=TcpTransport)
	TArray<FString> TcpConnectToEndpoints;

	/** Nats Server, such as : nats://127.0.0.1:4222 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(EditCondition="bEnableNatsBridge", ExposeOnSpawn=true), Category=TcpTransport)
	FString NatsServerURL;

protected:
	TSharedPtr<IMessageBus, ESPMode::ThreadSafe> MessageBus;

	TSharedPtr<IMessageBridge, ESPMode::ThreadSafe> TcpBridge;
	TSharedPtr<FGameTcpMessageTransport, ESPMode::ThreadSafe> TcpTransport;

	TSharedPtr<IMessageBridge, ESPMode::ThreadSafe> NatsBridge;
	TSharedPtr<FGameNatsMessageTransport, ESPMode::ThreadSafe> NatsTransport;
};
