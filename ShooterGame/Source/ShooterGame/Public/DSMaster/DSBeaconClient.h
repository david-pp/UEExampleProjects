// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "DSBeaconClient.generated.h"

USTRUCT(BlueprintType)
struct FGameServerInstanceInfo
{
	GENERATED_BODY()

	/** IP:Port */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerAddress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MapName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Cmd;
};

UCLASS()
class SHOOTERGAME_API ADSBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADSBeaconClient();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(Replicated)
	FGameServerInstanceInfo GameServerInstanceInfo;

	//
	// Client Code
	// 
	UFUNCTION(BlueprintCallable, BlueprintCosmetic)
	bool ConnectToAgentServer(FString HostAddress);

	UFUNCTION(BlueprintImplementableEvent)
	void OnConnectToAgentServer();

	virtual void OnConnected() override;
	virtual void OnFailure() override;

	// Server Code
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void SayHelloToAgentServer(FGameServerInstanceInfo InstanceInfo);


	void UpdateGameServerInstanceInfo();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
