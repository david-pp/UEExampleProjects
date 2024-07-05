// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameMessages.h"
#include "GameMessageRpcClientActor.generated.h"

class IMessageBus;
class IMessageRpcClient;
class IGameRpcServerLocator;
struct FMessageAddress;

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnGameRpcDebugComplete, const FGameRpcDebugResult&, Result);

UCLASS()
class GAMEMESSAGING_API AGameMessageRpcClientActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AGameMessageRpcClientActor();

	// Find message bus by Name
	TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetMessageBus() const;

	// virtual bool IsConnected() const;
	// virtual TSharedPtr<IMessageRpcClient> GetRpcClient() const;
	virtual void OnServerLocated(const FMessageAddress& ServerAddress);
	virtual void OnServerLost(const FMessageAddress& ServerAddress);

public:
	// RPC 1 : Sync 
	UFUNCTION(BlueprintCallable)
	FGameRpcDebugResult DebugRpcDemo(FString Param1, int32 Param2);

	// RPC 2 : Async with delegate
	UFUNCTION(BlueprintCallable)
	void AsyncDebugRpcDemo(FString Param1, int32 Param2, const FOnGameRpcDebugComplete& OnComplete);

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FName MessageBusName;
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FName RpcClientName;

	/** Rpc server to find */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(ExposeOnSpawn=true), Category=GameMessaging)
	FString RpcServerKey;

	TSharedPtr<IMessageRpcClient> RpcClient;
	TSharedPtr<IGameRpcServerLocator> RpcServerLocator;
};
