// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameSessionTypes.h"
#include "HttpResultCallback.h"
#include "HttpRouteHandle.h"
#include "HttpPath.h"
#include "HttpServerConstants.h"
#include "HttpServerResponse.h"
#include "Object.h"
#include "SessionService.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSessionService, Log, All);

/**
 * 
 */
UCLASS()
class SHOOTERGAME_API USessionService : public UObject
{
	GENERATED_BODY()
};

class FGameSessionManager : public TMap<FString, FRPGGameSessionDetails>
{
public:
	FRPGGameSessionDetails* CreateGameSession(const FRPGGameSessionDetails& SessionDetails)
	{
		FGuid Guid = FGuid::NewGuid();
		FString SessionId = Guid.ToString();

		FRPGGameSessionDetails& SessionAdded = Add(SessionId, SessionDetails);
		SessionAdded.SessionId = SessionId;
		return &SessionAdded;
	}

	FRPGGameSessionDetails* GetGameSession(const FString& SessionId)
	{
		if (SessionId.IsEmpty()) return nullptr;
		return Find(SessionId);
	}
	
	FRPGGameSessionDetails* UpdateGameSession(const FString& SessionId, const FRPGGameSessionDetails& NewSession)
	{
		Remove(SessionId);

		FRPGGameSessionDetails& SessionAdded = Add(SessionId, NewSession);
		SessionAdded.SessionId = SessionId;
		return &SessionAdded;
	}

	bool DestroyGameSession(const FString& SessionId)
	{
		return Remove(SessionId) > 0;
	}
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHttpSessionServiceStarted, uint32 /*Port*/);
DECLARE_DELEGATE_RetVal_TwoParams(bool, FHttpSessionRequestHandlerDelegate, const FHttpServerRequest&, const FHttpResultCallback&);

struct FHttpSessionRequestRoute
{
	FHttpSessionRequestRoute(FString InRouteDescription, FHttpPath InPath, EHttpServerRequestVerbs InVerb, FHttpSessionRequestHandlerDelegate InHandler) : RouteDescription(MoveTemp(InRouteDescription)), Path(MoveTemp(InPath)), Verb(InVerb), Handler(MoveTemp(InHandler))
	{
	}

	/** A description of how the route should be used. */
	FString RouteDescription;
	/** Relative path (ie. /remote/object) */
	FHttpPath Path;
	/** The desired HTTP verb (ie. GET, PUT..) */
	EHttpServerRequestVerbs Verb = EHttpServerRequestVerbs::VERB_GET;
	/** The handler called when the route is accessed. */
	FHttpSessionRequestHandlerDelegate Handler;

	friend uint32 GetTypeHash(const FHttpSessionRequestRoute& Route) { return HashCombine(GetTypeHash(Route.Path), GetTypeHash(Route.Verb)); }
	friend bool operator==(const FHttpSessionRequestRoute& LHS, const FHttpSessionRequestRoute& RHS) { return LHS.Path == RHS.Path && LHS.Verb == RHS.Verb; }
};

class FHttpSessionService
{
public:
	/**
	 * Init
	 */
	void Init(uint32 Port = 30000);

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
	void RegisterRoute(const FHttpSessionRequestRoute& Route);

	//~ Server started stopped delegates.
	FOnHttpSessionServiceStarted OnHttpServerStartedDelegate;
	FSimpleMulticastDelegate OnHttpServerStoppedDelegate;

protected:
	/** Bind the route in the http router and add it to the list of active routes. */
	void StartRoute(const FHttpSessionRequestRoute& Route);

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

	/** HTTP server's port. */
	uint32 HttpServerPort = 30000;

	/** Http router handle */
	TSharedPtr<IHttpRouter> HttpRouter;

	/** Mapping of routes to delegate handles */
	TMap<uint32, FHttpRouteHandle> ActiveRouteHandles;

	/** Set of routes that will be activated on http server start. */
	TSet<FHttpSessionRequestRoute> RegisteredHttpRoutes;

	/** List of preprocessor delegates that need to be registered when the server is started. */
	TMap<FDelegateHandle, FHttpRequestHandler> PreprocessorsToRegister;
	/** Mappings of preprocessors delegate handles generated from the WebRC module to the ones generated from the Http Module.*/
	TMap<FDelegateHandle, FDelegateHandle> PreprocessorsHandleMappings;

	/** Game session manager */
	FGameSessionManager SessionManager;
};

struct FHttpServerHelper
{
	static TUniquePtr<FHttpServerResponse> CreateHttpResponse(EHttpServerResponseCodes InResponseCode = EHttpServerResponseCodes::BadRequest)
	{
		TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
		Response->Headers.Add(TEXT("content-type"), {TEXT("application/json")});
		Response->Code = InResponseCode;
		return Response;
	}

	static void DumpHttpServerRequest(const FHttpServerRequest& Request, const FString& Title = TEXT(""))
	{
		UE_LOG(LogHttp, Warning, TEXT("--- ServerRequest@%s : %s"), *Title, *Request.RelativePath.GetPath());

		for (auto Header : Request.Headers)
		{
			UE_LOG(LogHttp, Warning, TEXT("- Header : %s"), *Header.Key);

			for (auto Value : Header.Value)
			{
				UE_LOG(LogHttp, Warning, TEXT("-- Value : %s"), *Value);
			}
		}

		for (auto QueryParam : Request.QueryParams)
		{
			UE_LOG(LogHttp, Warning, TEXT("- QueryParam : %s -> %s"), *QueryParam.Key, *QueryParam.Value);
		}

		for (auto PathParam : Request.PathParams)
		{
			UE_LOG(LogHttp, Warning, TEXT("- PathParam : %s -> %s"), *PathParam.Key, *PathParam.Value);
		}
	}
};

