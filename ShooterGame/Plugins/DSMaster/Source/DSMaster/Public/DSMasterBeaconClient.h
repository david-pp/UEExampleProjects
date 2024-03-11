// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMasterTypes.h"
#include "OnlineBeaconClient.h"
#include "OnlineBeaconClient.h"
#include "DSMasterBeaconClient.generated.h"


// DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnReplyGameServerInstances, const TArray<FGameServerInstanceInfo>&, ServerInstances);

UCLASS()
class DSMASTER_API ADSMasterBeaconClient : public AOnlineBeaconClient
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ADSMasterBeaconClient();

	UFUNCTION(BlueprintPure)
	bool IsClient(EDSMasterClientType ClientType) const;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic)
	bool ConnectToMasterServer(FString HostAddress);

	UFUNCTION(BlueprintImplementableEvent)
	void OnConnectToMasterServer();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnConnected() override;
	virtual void OnFailure() override;

	UPROPERTY(Replicated, BlueprintReadWrite)
	FDSMasterClientSettings Settings;
	
public:
	UFUNCTION(Server, Reliable, BlueprintCallable)
	void SayHelloToMasterServer(const FString& Message);

	// UFUNCTION(Server, Reliable, BlueprintCallable)
	// void RequestGameServerInstances();
	// UFUNCTION(Client, Reliable, BlueprintCallable)
	// void ReplyGameServerInstances(const TArray<FGameServerInstanceInfo>& ServerInstances);
	// UPROPERTY(BlueprintAssignable)
	// FOnReplyGameServerInstances OnReplyGameServerInstances;
	//
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
