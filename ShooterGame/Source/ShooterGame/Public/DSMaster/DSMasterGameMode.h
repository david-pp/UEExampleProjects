// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSBeaconHost.h"
#include "DSMaster.h"
#include "DSMasterBeaconHost.h"
#include "DSMasterBeaconClient.h"
#include "DSMasterTypes.h"
#include "GameServerManager.h"
#include "OnlineBeaconHost.h"
#include "HttpPath.h"
#include "IHttpRouter.h"
#include "HttpRouteHandle.h"
#include "HttpServerRequest.h"
#include "InteractiveProcess.h"
#include "SessionService.h"
#include "ShooterGameSession.h"
#include "DSMasterGameMode.generated.h"

USTRUCT(BlueprintType)
struct FGameServerMapSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinInstances = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxInstances = 10;
};

USTRUCT(BlueprintType)
struct FGameServerSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString ServerName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ServerPort = 7777;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameServerMapSettings> ServerMaps;
};

USTRUCT(BlueprintType)
struct FDSMasterGameSessionSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MapName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Options;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DSAgentServer;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DebugExe;
};



UCLASS()
class SHOOTERGAME_API ADSMasterGameMode : public AGameModeBase
{
	GENERATED_UCLASS_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	virtual void StartPlay() override;

	UFUNCTION(BlueprintPure, Category=DSMaster)
	bool IsManager() const;
	UFUNCTION(BlueprintPure, Category=DSMaster)
	bool IsAgent() const;

	////////////////////////// HTTP Based //////////////////////////
	FHttpSessionService HttpSessionService;

	/** HTTP server's port. */
	UPROPERTY(config, EditAnywhere, Category = "HTTPServerPort")
	uint32 HttpServerPort = 30000;

	UFUNCTION(BlueprintCallable)
	void DebugRequestSessionInfo();

	UFUNCTION(BlueprintCallable)
	void DebugCreateSession();

	UFUNCTION(BlueprintCallable)
	void DebugFindSession();

	UFUNCTION(BlueprintCallable)
	void DebugSessionProtocol();

	/** Transient properties of a session during game creation/matchmaking */
	FShooterGameSessionParams CurrentSessionParams;
	/** Current host settings */
	TSharedPtr<class FOnlineSessionSettings> HostSettings;
	/** Current search settings */
	TSharedPtr<class FOnlineSessionSearch> SearchSettings;

	/** Delegate for creating a new session */
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	/** Delegate for searching for sessions */
	FOnFindSessionsCompleteDelegate OnFindSessionsCompleteDelegate;
	
	virtual void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	
	/** Handles to various registered delegates */
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FDelegateHandle OnFindSessionCompleteDelegateHandle;

	////////////////////////// Beacon Based //////////////////////////

	//
	// DS Master Server
	// 
	UFUNCTION(BlueprintCallable, Category=DSMaster)
	bool CreateBeaconHost();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=DSMaster)
	EDSMasterMode DSMasterMode = EDSMasterMode::AllInOne;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BeaconHost)
	int32 BeaconHostPort = 15000;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BeaconHost)
	TSubclassOf<ADSMasterBeaconHost> DSMasterBeaconHostObjectClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BeaconHost)
	TSubclassOf<ADSBeaconHost> DSBeaconHostObjectClass;

	//
	// Agent -> Manager
	// 
	UFUNCTION(BlueprintCallable, Category=DSMaster)
	bool ConnectToMasterServer(FString ServerAddress);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BeaconClient)
	TSubclassOf<ADSMasterBeaconClient> DSMasterBeaconClientClass;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=BeaconClient)
	FString DSMaterServerAddress;

protected:
	UPROPERTY(Transient)
	AOnlineBeaconHost* BeaconHost;

	UPROPERTY(Transient)
	ADSMasterBeaconHost* DSMasterHost;

	UPROPERTY(Transient)
	ADSBeaconHost* DSHost;

	UPROPERTY(Transient)
	class ADSMasterBeaconClient* DSMasterClient;

public:
	//
	// Create Game Server Instances
	UFUNCTION(BlueprintCallable, Category=DSMaster)
	bool CreateGameServerInstance(FDSMasterGameSessionSettings SessionSettings);

	// ~ Debug for FInteractiveProcess
	UFUNCTION(BlueprintCallable, Category=DSMaster)
	bool DebugCreateGameServerInstance(FDSMasterGameSessionSettings SessionSettings);
	UFUNCTION(BlueprintCallable, Category=DSMaster)
	bool DebugCancelGameServerProcess();
	// TSharedPtr<FInteractiveProcess> GameServerProcess;
	TSharedPtr<FGameServerProcess> GameServerProcess;
	
	// ~ Debug for NamedPipe
	UFUNCTION(BlueprintCallable, Category=DSMaster)
	bool DebugCreateGameServerInstanceWithNamedPipe(FDSMasterGameSessionSettings SessionSettings);

	// We need two pipes, as the named pipe API does not support simultaneous read/write on two threads.
	FPlatformNamedPipe InputNamedPipe, OutputNamedPipe;

	UPROPERTY(Config)
	FString ServerName;

	UPROPERTY(Config)
	int32 ServerPort = 7000;

	UPROPERTY(Config)
	TArray<FGameServerMapSettings> ServerMaps;

	int32 AllocateServerPort();

	TArray<FGameServerInstanceInfo> ServerInstances;
};
