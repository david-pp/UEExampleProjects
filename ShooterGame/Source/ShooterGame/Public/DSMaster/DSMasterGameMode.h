// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSBeaconHost.h"
#include "DSMaster.h"
#include "DSMasterBeaconHost.h"
#include "DSMasterBeaconClient.h"
#include "DSMasterTypes.h"
#include "OnlineBeaconHost.h"
#include "HttpPath.h"
#include "IHttpRouter.h"
#include "HttpRouteHandle.h"
#include "HttpServerRequest.h"
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
};

/////////////// HTTP Based ////////////

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDSMasterServerStarted, uint32 /*Port*/);

DECLARE_DELEGATE_RetVal_TwoParams(bool, FDSMasterRequestHandlerDelegate, const FHttpServerRequest&, const FHttpResultCallback&);

struct FDSMasterRequestRoute
{
	FDSMasterRequestRoute(FString InRouteDescription, FHttpPath InPath, EHttpServerRequestVerbs InVerb, FDSMasterRequestHandlerDelegate InHandler) : RouteDescription(MoveTemp(InRouteDescription)), Path(MoveTemp(InPath)), Verb(InVerb), Handler(MoveTemp(InHandler))
	{
	}

	/** A description of how the route should be used. */
	FString RouteDescription;
	/** Relative path (ie. /remote/object) */
	FHttpPath Path;
	/** The desired HTTP verb (ie. GET, PUT..) */
	EHttpServerRequestVerbs Verb = EHttpServerRequestVerbs::VERB_GET;
	/** The handler called when the route is accessed. */
	FDSMasterRequestHandlerDelegate Handler;

	friend uint32 GetTypeHash(const FDSMasterRequestRoute& Route) { return HashCombine(GetTypeHash(Route.Path), GetTypeHash(Route.Verb)); }
	friend bool operator==(const FDSMasterRequestRoute& LHS, const FDSMasterRequestRoute& RHS) { return LHS.Path == RHS.Path && LHS.Verb == RHS.Verb; }
};

//// 


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
public:
	/**
	 * Start the web control server
	 */
	void StartHttpServer();

	/**
	 * Stop the web control server.
	 */
	void StopHttpServer();

	/**
	 * Register a route to the API.
	 * @param Route The route to register.
	 */
	void RegisterRoute(const FDSMasterRequestRoute& Route);

	//~ Server started stopped delegates.
	FOnDSMasterServerStarted OnHttpServerStartedDelegate;
	FSimpleMulticastDelegate OnHttpServerStoppedDelegate;

protected:
	/** Bind the route in the http router and add it to the list of active routes. */
	void StartRoute(const FDSMasterRequestRoute& Route);

	/** Register HTTP and Websocket routes. */
	void RegisterRoutes();

	//~ Route handlers
	bool HandleSessionInfoRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleCreateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRestApi(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRestApi_GetSessionList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRestApi_GetSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRestApi_GetSessionAttributeList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRestApi_GetSessionAttribute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

	/** Http router handle */
	TSharedPtr<IHttpRouter> HttpRouter;

	/** Mapping of routes to delegate handles */
	TMap<uint32, FHttpRouteHandle> ActiveRouteHandles;

	/** Set of routes that will be activated on http server start. */
	TSet<FDSMasterRequestRoute> RegisteredHttpRoutes;

	/** List of preprocessor delegates that need to be registered when the server is started. */
	TMap<FDelegateHandle, FHttpRequestHandler> PreprocessorsToRegister;
	/** Mappings of preprocessors delegate handles generated from the WebRC module to the ones generated from the Http Module.*/
	TMap<FDelegateHandle, FDelegateHandle> PreprocessorsHandleMappings;

public:
	/** HTTP server's port. */
	UPROPERTY(config, EditAnywhere, Category = "HTTPServerPort")
	uint32 HttpServerPort = 30000;

	UFUNCTION(BlueprintCallable)
	void DebugRequestSessionInfo();

	UFUNCTION(BlueprintCallable)
	void DebugCreateSession();

	/** Current host settings */
	TSharedPtr<class FOnlineSessionSettings> HostSettings;
	/** Delegate for creating a new session */
	FOnCreateSessionCompleteDelegate OnCreateSessionCompleteDelegate;
	virtual void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	/** Handles to various registered delegates */
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;

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

	UPROPERTY(Config)
	FString ServerName;

	UPROPERTY(Config)
	int32 ServerPort = 7000;

	UPROPERTY(Config)
	TArray<FGameServerMapSettings> ServerMaps;

	int32 AllocateServerPort();

	TArray<FGameServerInstanceInfo> ServerInstances;
};
