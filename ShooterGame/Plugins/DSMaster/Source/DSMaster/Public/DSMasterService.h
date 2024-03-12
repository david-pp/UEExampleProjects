// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMaster.h"
#include "DSMasterBeaconClient.h"
#include "DSMasterTypes.h"
#include "GameServerManager.h"
#include "GameSessionManager.h"
#include "HttpResultCallback.h"
#include "HttpRouteHandle.h"
#include "HttpPath.h"
#include "HttpServerConstants.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "JsonObjectConverter.h"
#include "OnlineBeaconHost.h"
// #include "DSMasterService.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnDSMasterServiceStarted, uint32 /*Port*/);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FDSMasterRequestHandlerDelegate, const FHttpServerRequest&, const FHttpResultCallback&);

/**
 * HTTP Request Route Rules
 */
struct DSMASTER_API FDSMasterRequestRoute
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


/**
 * DS MasterServices:
 *  - Session Service by HTTP
 *  - DS->Master/Agent->Master by Beacon
 */
class DSMASTER_API FDSMasterService : public FGCObject
{
public:
	bool InitServer(UWorld* World, FString& ErrorMessage);
	void StopServer();
	
	/**
	 * Init
	 */
	void InitHttpServer(uint32 Port = 30000);

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
	FOnDSMasterServiceStarted OnHttpServerStartedDelegate;
	FSimpleMulticastDelegate OnHttpServerStoppedDelegate;

	bool ServerTick(float DeltaTime);

public:
	// FGCObject Interface
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	AOnlineBeaconHost* CreateDSMasterBeaconHost(UWorld* World, const FDSMasterBeaconHostSettings& InBeaconHostSettings);

	FDSMasterBeaconHostSettings BeaconHostSettings;
	AOnlineBeaconHost* BeaconHost = nullptr;
	// Master/Agent Host
	ADSMasterBeaconHost* DSMasterHost = nullptr;
	FString ServerName;

	// Agent Server
	bool ConnectToMasterServer(UWorld* World, const FDSMasterClientSettings& InClientSetting);

	// Master Client
	ADSMasterBeaconClient* DSMasterClient = nullptr;
	
public:
	/** Helper functions */
	static void DumpHttpServerRequest(const FHttpServerRequest& Request, const FString& Title = TEXT(""));
	static TUniquePtr<FHttpServerResponse> CreateHttpResponse(EHttpServerResponseCodes InResponseCode = EHttpServerResponseCodes::BadRequest);

	static void ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload);
	
	template <typename OutStructType>
	static bool JsonHTTPBodyToUStruct(const TArray<uint8>& Body, OutStructType* OutStruct, int64 CheckFlags = 0, int64 SkipFlags = 0)
	{
		TArray<uint8> TCHARBody;
		ConvertToTCHAR(Body, TCHARBody);

		FMemoryReaderView BodyReader(TCHARBody);
		FString JsonString;
		TSharedPtr<FJsonObject> JsonObject;
		TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(&BodyReader);
		if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
		{
			UE_LOG(LogDSMaster, Warning, TEXT("JsonObjectStringToUStruct - Unable to parse json=[%s]"), *JsonString);
			return false;
		}
		if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), OutStruct, CheckFlags, SkipFlags))
		{
			UE_LOG(LogDSMaster, Warning, TEXT("JsonObjectStringToUStruct - Unable to deserialize. json=[%s]"), *JsonString);
			return false;
		}
		return true;
	}

protected:
	/** Bind the route in the http router and add it to the list of active routes. */
	void StartRoute(const FDSMasterRequestRoute& Route);

	/** Register HTTP and Websocket routes. */
	void RegisterRoutes();

	//~ Route handlers
	bool HandleCreateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleDestroySession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleGetSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleUpdateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleGetSessionList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	// bool HandleGetSessionAttributeList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	// bool HandleGetSessionAttribute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleRequestGameSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

	//~ Game server callbacks
	void OnGameServerLaunched(FGameServerProcessPtr GameServer);
	void OnGameServerStopped(FGameServerProcessPtr GameServer, bool bIsCanceled);
	FDelegateHandle GameServerStoppedHandle;

	/** HTTP server's port. */
	uint32 HttpServerPort = 30000;

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

	FDelegateHandle TickerHandle;
	
	/** Game session manager */
	FGameSessionManager SessionManager;

	/** Game server manager */
	FGameServerManager ServerManager;

	
};
