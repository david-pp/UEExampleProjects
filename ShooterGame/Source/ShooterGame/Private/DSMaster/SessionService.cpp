// Fill out your copyright notice in the Description page of Project Settings.

#include "SessionService.h"
#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"

DEFINE_LOG_CATEGORY(LogSessionService)

void FHttpSessionService::Init(uint32 Port)
{
	HttpServerPort = Port;
	RegisterRoutes();
	StartHttpServer();
}

void FHttpSessionService::StartHttpServer()
{
	if (!HttpRouter)
	{
		HttpRouter = FHttpServerModule::Get().GetHttpRouter(HttpServerPort);
		if (!HttpRouter)
		{
			UE_LOG(LogSessionService, Error, TEXT("HttpSessionService couldn't be started on port %d"), HttpServerPort);
			return;
		}

		for (FHttpSessionRequestRoute& Route : RegisteredHttpRoutes)
		{
			StartRoute(Route);
		}

		// Go through externally registered request pre-processors and register them with the http router.
		for (const TPair<FDelegateHandle, FHttpRequestHandler>& Handler : PreprocessorsToRegister)
		{
			// Find the pre-processors HTTP-handle from the one we generated.
			FDelegateHandle& Handle = PreprocessorsHandleMappings.FindChecked(Handler.Key);
			if (Handle.IsValid())
			{
				HttpRouter->UnregisterRequestPreprocessor(Handle);
			}

			// Update the preprocessor handle mapping.
			Handle = HttpRouter->RegisterRequestPreprocessor(Handler.Value);
		}

		FHttpServerModule::Get().StartAllListeners();

		OnHttpServerStartedDelegate.Broadcast(HttpServerPort);
	}
}

void FHttpSessionService::StopHttpServer()
{
	if (FHttpServerModule::IsAvailable())
	{
		FHttpServerModule::Get().StopAllListeners();
	}

	if (HttpRouter)
	{
		for (const TPair<uint32, FHttpRouteHandle>& Tuple : ActiveRouteHandles)
		{
			if (Tuple.Key)
			{
				HttpRouter->UnbindRoute(Tuple.Value);
			}
		}

		ActiveRouteHandles.Reset();
	}

	HttpRouter.Reset();
	OnHttpServerStoppedDelegate.Broadcast();
}

void FHttpSessionService::RegisterRoute(const FHttpSessionRequestRoute& Route)
{
	RegisteredHttpRoutes.Add(Route);

	// If the route is registered after the server is already started.
	if (HttpRouter)
	{
		StartRoute(Route);
	}
}

void FHttpSessionService::StartRoute(const FHttpSessionRequestRoute& Route)
{
	// The handler is wrapped in a lambda since HttpRouter::BindRoute only accepts TFunctions
	ActiveRouteHandles.Add(GetTypeHash(Route),
		HttpRouter->BindRoute(Route.Path, Route.Verb,
			[this, Handler = Route.Handler](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
			{
				return Handler.Execute(Request, OnComplete);
			}));
}

void FHttpSessionService::RegisterRoutes()
{
	RegisterRoute({
		TEXT("Create Session "),
		FHttpPath(TEXT("/session")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpSessionRequestHandlerDelegate::CreateRaw(this, &FHttpSessionService::HandleCreateSession)
	});

	RegisterRoute({
		TEXT("Get Session"),
		FHttpPath(TEXT("/session/:session")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpSessionRequestHandlerDelegate::CreateRaw(this, &FHttpSessionService::HandleGetSession)
	});

	RegisterRoute({
		TEXT("Get Session"),
		FHttpPath(TEXT("/session/:session")),
		EHttpServerRequestVerbs::VERB_PUT,
		FHttpSessionRequestHandlerDelegate::CreateRaw(this, &FHttpSessionService::HandleUpdateSession)
	});


	RegisterRoute({
		TEXT("Destroy Session"),
		FHttpPath(TEXT("/session/:session")),
		EHttpServerRequestVerbs::VERB_DELETE,
		FHttpSessionRequestHandlerDelegate::CreateRaw(this, &FHttpSessionService::HandleDestroySession)
	});

	RegisterRoute({
		TEXT("Get Session List"),
		FHttpPath(TEXT("/sessions")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpSessionRequestHandlerDelegate::CreateRaw(this, &FHttpSessionService::HandleGetSessionList)
	});
}

bool FHttpSessionService::HandleCreateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FHttpServerHelper::DumpHttpServerRequest(Request, TEXT("HandleCreateSession"));

	FString ErrorMsg;
	FActiveRPGGameSession RPGGameSession;
	if (FHttpHelper::JsonHTTPBodyToUStruct(Request.Body, &RPGGameSession))
	{
		// debug
		FOnlineSessionSettings OnlineSessionSettings;
		FNamedOnlineSession NamedOnlineSession(FName(RPGGameSession.SessionName), OnlineSessionSettings);
		RPGGameSession.SetupToNamedOnlineSession(&NamedOnlineSession);
		DumpNamedSession(&NamedOnlineSession);
		
		FRPGGameSessionDetails* NewSession = SessionManager.CreateGameSession(RPGGameSession.SessionDetails);
		if (NewSession)
		{
			FString ReplyText;
			if (FJsonObjectConverter::UStructToJsonObjectString(*NewSession, ReplyText))
			{
				auto Response = FHttpServerResponse::Create(ReplyText, TEXT("application/json"));
				OnComplete(MoveTemp(Response));
				return true;
			}
		}
	}
	else
	{
		ErrorMsg = TEXT("Invalid input Json object");
	}

	auto Response = FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT("ServerError"), ErrorMsg);
	OnComplete(MoveTemp(Response));
	return true;
}

bool FHttpSessionService::HandleDestroySession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FString SessionId = Request.PathParams.FindRef(TEXT("session"));

	FString ErrorMsg;
	do
	{
		FRPGGameSessionDetails* Session = SessionManager.GetGameSession(SessionId);
		if (!Session)
		{
			ErrorMsg = FString::Printf(TEXT("Invalid SessionId : %s"), *SessionId);
			break;
		}

		SessionManager.DestroyGameSession(SessionId);
		TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
		Response->Code = EHttpServerResponseCodes::Ok;
		OnComplete(MoveTemp(Response));
		return true;
		
	} while (true);
	
	auto Response = FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT("ServerError"), ErrorMsg);
	OnComplete(MoveTemp(Response));
	return true;
}

bool FHttpSessionService::HandleGetSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FHttpServerHelper::DumpHttpServerRequest(Request, TEXT("HandleGetSession"));

	FString SessionId = Request.PathParams.FindRef(TEXT("session"));
	FString GameMode = Request.QueryParams.FindRef(TEXT("game"));
	if (!GameMode.IsEmpty())
	{
		UE_LOG(LogSessionService, Log, TEXT("game=%s"), *GameMode);
	}

	FString ErrorMsg;
	do
	{
		FRPGGameSessionDetails* Session = SessionManager.GetGameSession(SessionId);
		if (!Session)
		{
			ErrorMsg = FString::Printf(TEXT("Invalid SessionId : %s"), *SessionId);
			break;
		}

		FString ReplyText;
		{
			FJsonObjectConverter::UStructToJsonObjectString(*Session, ReplyText);
		}

		auto Response = FHttpServerResponse::Create(ReplyText, TEXT("application/json"));
		OnComplete(MoveTemp(Response));
		return true;
		
	} while (true);

	// Error
	auto Response = FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT("ServerError"), ErrorMsg);
	OnComplete(MoveTemp(Response));
	return true;
}

bool FHttpSessionService::HandleUpdateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FHttpServerHelper::DumpHttpServerRequest(Request, TEXT("HandleUpdateSession"));

	FString SessionId = Request.PathParams.FindRef(TEXT("session"));

	FString ErrorMsg;
	do
	{
		FRPGGameSessionDetails* ExistSession = SessionManager.GetGameSession(SessionId);
		if (!ExistSession)
		{
			ErrorMsg = FString::Printf(TEXT("Invalid SessionId : %s"), *SessionId);
			break;
		}

		FRPGGameSessionDetails InputSession;
		if (!FHttpHelper::JsonHTTPBodyToUStruct(Request.Body, &InputSession))
		{
			ErrorMsg = FString::Printf(TEXT("Invalid session protocal"));
			break;
		}

		// update
		FRPGGameSessionDetails* UpdatedSession = SessionManager.UpdateGameSession(SessionId, InputSession);

		TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
		Response->Code = EHttpServerResponseCodes::Ok;
		OnComplete(MoveTemp(Response));
		return true;
		
	} while (true);

	// Error
	auto Response = FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT("ServerError"), ErrorMsg);
	OnComplete(MoveTemp(Response));
	return true;
}

bool FHttpSessionService::HandleGetSessionList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FHttpServerHelper::DumpHttpServerRequest(Request, TEXT("HandleGetSessionList"));
	
	FString GameMode = Request.QueryParams.FindRef(TEXT("game"));
	if (!GameMode.IsEmpty())
	{
		UE_LOG(LogSessionService, Log, TEXT("game=%s"), *GameMode);
	}

	FString ReplyText;
	{
		FHttpSessionSearchResult SearchResult;
		for (auto& KV : SessionManager)
		{
			SearchResult.Sessions.Add(KV.Value);
		}

		FJsonObjectConverter::UStructToJsonObjectString(SearchResult, ReplyText);
	}

	auto Response = FHttpServerResponse::Create(ReplyText, TEXT("application/json"));
	OnComplete(MoveTemp(Response));
	return true;
}
