// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMasterBeaconClient.h"
#include "DSMaster.h"
#include "UnrealNetwork.h"

// Sets default values
ADSMasterBeaconClient::ADSMasterBeaconClient()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

bool ADSMasterBeaconClient::IsClient(EDSMasterClientType ClientType) const
{
	return Settings.ClientType == ClientType;
}

bool ADSMasterBeaconClient::ConnectToMasterServer(FString HostAddress)
{
	FURL ConnectURL(NULL, *HostAddress, TRAVEL_Absolute);
	if (InitClient(ConnectURL))
	{
		UE_LOG(LogDSMaster, Log, TEXT("DSMasterBeaconClient - ConnectToHost: %s."), *ConnectURL.ToString());
		return true;
	}

	UE_LOG(LogDSMaster, Warning, TEXT("DSMasterBeaconClient - ConnectToHost: Failure to init client beacon with %s."), *ConnectURL.ToString());
	return false;
}

void ADSMasterBeaconClient::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADSMasterBeaconClient, Settings);
}

void ADSMasterBeaconClient::OnConnected()
{
	UE_LOG(LogTemp, Log, TEXT("DSMasterBeaconClient - OnConnected"))
	Super::OnConnected();
	OnConnectToMasterServer();

	if (IsClient(EDSMasterClientType::Agent))
	{
		SayHelloToMasterServer(TEXT("I'm a DSAgent"));
	}
	else if (IsClient(EDSMasterClientType::DedicatedServer))
	{
		SayHelloToMasterServer(TEXT("I'm a GameServer"));
	}
}

void ADSMasterBeaconClient::OnFailure()
{
	Super::OnFailure();
	UE_LOG(LogTemp, Warning, TEXT("DSMasterBeaconClient - OnFailure"))
}

void ADSMasterBeaconClient::SayHelloToMasterServer_Implementation(const FString& Message)
{
	UE_LOG(LogDSMaster, Log, TEXT("DSMaster - Hello from Client : %s"), *Message);
}

// void ADSMasterBeaconClient::RequestGameServerInstances_Implementation()
// {
// 	ADSMasterGameMode* DSMasterMode = Cast<ADSMasterGameMode>(GetWorld()->GetAuthGameMode());
// 	if (DSMasterMode)
// 	{
// 		ReplyGameServerInstances(DSMasterMode->ServerInstances);
// 	}
// }
//
// void ADSMasterBeaconClient::ReplyGameServerInstances_Implementation(const TArray<FGameServerInstanceInfo>& ServerInstances)
// {
// 	OnReplyGameServerInstances.Broadcast(ServerInstances);
// }

// Called when the game starts or when spawned
void ADSMasterBeaconClient::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ADSMasterBeaconClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
