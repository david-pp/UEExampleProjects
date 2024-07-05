// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameMessages.h"
#include "GameFramework/Actor.h"
#include "MessageRpcServer.h"
#include "GameMessageRpcServerActor.generated.h"

class IMessageBus;
class FGameRpcServer;
class IGameRpcServerResponder;

UCLASS()
class GAMEMESSAGING_API AGameMessageRpcServerActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGameMessageRpcServerActor();

	// Find message bus by Name
	TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetMessageBus() const;

public:
	// Server Handle Rpc
	TAsyncResult<FGameRpcDebugResult> HandleDebugRpc(const FGameRpcDebugRequest& Request);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FName MessageBusName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FString RpcServerName;

	/** Rpc server */
	TSharedPtr<FGameRpcServer> RpcServer;
	/** Rpc server responder */
	TSharedPtr<IGameRpcServerResponder> RpcServerResponder;
};
