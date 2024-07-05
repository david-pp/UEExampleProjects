// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameMessages.h"
#include "GameMessageEndpointActor.generated.h"

class IMessageBus;
class IMessageContext;
class FMessageEndpoint;

UCLASS()
class GAMEMESSAGING_API AGameMessageEndpointActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGameMessageEndpointActor();

	// Find message bus by Name
	TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetMessageBus() const;

protected:
	// ~ AActor Interfaces
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable)
	void BroadcastHeartBeat();

	UFUNCTION(BlueprintCallable)
	void SendPingToAddress(const FString& Address);
	UFUNCTION(BlueprintCallable)
	void SendPingToEndpointActor(AGameMessageEndpointActor* TargetEndpointActor);
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FName MessageBusName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FName MessageEndpointName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	float HeartBeatRate = 1.0f;;
	
	void HandleHeartBeatMessage(const FGameEndpointHeartBeat& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
	void HandlePingMessage(const FGameEndpointPing& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);
	void HandlePongMessage(const FGameEndpointPong& Message, const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context);

protected:
	TSharedPtr<FMessageEndpoint, ESPMode::ThreadSafe> MessageEndpoint;

	FTimerHandle HearBeatTimer;
};
