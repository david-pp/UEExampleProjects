// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "TestBeaconClient.h"
#include "MyTestBeaconClient.generated.h"

UCLASS()
class THIRDPERSONMP_API AMyTestBeaconClient : public ATestBeaconClient
{
	GENERATED_BODY()

public:
	AMyTestBeaconClient();

	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnConnected() override;
	virtual void OnFailure() override;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic)
	void ConnectToHost(FString HostAddress);

	UFUNCTION(BlueprintImplementableEvent)
	void OnConnectToHost();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void ServerAddCounter(int32 Value);
	UFUNCTION(Client, Reliable, BlueprintCallable)
	void ClientAddCounterDone(int32 CounterValue);
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable)
	void MulticastAddCounterDone(int32 CounterValue);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated)
	int32 BeaconCounter = 0;
};
