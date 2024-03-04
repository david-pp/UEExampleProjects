// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMaster/DSBeaconClient.h"
#include "DSMaster.h"
#include "DSMasterGameMode.h"

// Sets default values
ADSBeaconClient::ADSBeaconClient()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

bool ADSBeaconClient::ConnectToAgentServer(FString HostAddress)
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

void ADSBeaconClient::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADSBeaconClient, GameServerInstanceInfo);
}

void ADSBeaconClient::OnConnected()
{
	UE_LOG(LogTemp, Warning, TEXT("DSBeaconClient - OnConnected"))

	Super::OnConnected();

	UpdateGameServerInstanceInfo();
	SayHelloToAgentServer(GameServerInstanceInfo);

	OnConnectToAgentServer();
}

void ADSBeaconClient::OnFailure()
{
	Super::OnFailure();

	UE_LOG(LogTemp, Warning, TEXT("DSBeaconClient - OnFailure"))
}

void ADSBeaconClient::SayHelloToAgentServer_Implementation(FGameServerInstanceInfo InstanceInfo)
{
	UE_LOG(LogDSMaster, Log, TEXT("Agent - GameServer Instance : %s,%s,%s"), *InstanceInfo.MapName, *InstanceInfo.ServerAddress, *InstanceInfo.Cmd);

	ADSMasterGameMode* DSMasterMode = Cast<ADSMasterGameMode>(GetWorld()->GetAuthGameMode());
	if (DSMasterMode)
	{
		DSMasterMode->ServerInstances.Add(InstanceInfo);
	}
}

void ADSBeaconClient::UpdateGameServerInstanceInfo()
{
	FWorldContext& WorldContext = GEngine->GetWorldContextFromWorldChecked(GetWorld());
	GameServerInstanceInfo.ServerAddress = WorldContext.LastURL.GetHostPortString();
	GameServerInstanceInfo.Cmd = FCommandLine::Get();
	GameServerInstanceInfo.MapName = GetWorld()->GetName();
}

// Called when the game starts or when spawned
void ADSBeaconClient::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ADSBeaconClient::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
