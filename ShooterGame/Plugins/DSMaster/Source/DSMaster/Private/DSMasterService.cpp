// Fill out your copyright notice in the Description page of Project Settings.

#include "DSMasterService.h"
#include "DSMaster.h"
#include "DSMasterBeaconHost.h"
#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"

bool FDSMasterService::InitServer(UWorld* World, FString& ErrorMessage)
{
	if (!World) return false;
	
	ServerName = TEXT("Default");

	// Run as Master Server
	if (FParse::Value(FCommandLine::Get(), TEXT("Master="), ServerName) ||
		FParse::Param(FCommandLine::Get(), TEXT("Master")))
	{
		// Run Master Beacon Server
		FDSMasterBeaconHostSettings HostSettings;
		HostSettings.ListenPort = 9000;
		HostSettings.RunningMode = EDSMasterMode::Master;
		HostSettings.DSMasterBeaconHostObjectClass = ADSMasterBeaconHost::StaticClass();

		FParse::Value(FCommandLine::Get(), TEXT("MasterPort="), HostSettings.ListenPort);

		CreateDSMasterBeaconHost(World, HostSettings);

		// Run HTTP Server
		FParse::Value(FCommandLine::Get(), TEXT("MasterHttpPort="), HttpServerPort);
		InitHttpServer(HttpServerPort);
		
		return true;
	}

	// Run as Agent Server
	if (FParse::Value(FCommandLine::Get(), TEXT("Agent="), ServerName) ||
		FParse::Param(FCommandLine::Get(), TEXT("Agent")))
	{
		// Run Agent Beacon Server
		FDSMasterBeaconHostSettings HostSettings;
		HostSettings.ListenPort = 9001;
		HostSettings.RunningMode = EDSMasterMode::Agent;
		HostSettings.DSMasterBeaconHostObjectClass = ADSMasterBeaconHost::StaticClass();

		FParse::Value(FCommandLine::Get(), TEXT("AgentPort="), HostSettings.ListenPort);

		CreateDSMasterBeaconHost(World, HostSettings);

		// Connect to Master Server
		FDSMasterClientSettings ClientSettings;
		ClientSettings.ClientType = EDSMasterClientType::Agent;
		ClientSettings.DSMasterBeaconClientClass = ADSMasterBeaconClient::StaticClass();
		FParse::Value(FCommandLine::Get(), TEXT("MasterServer="), ClientSettings.MasterServerAddress);

		ConnectToMasterServer(World, ClientSettings);
	}

	return true;
}

void FDSMasterService::InitHttpServer(uint32 Port)
{
	HttpServerPort = Port;
	RegisterRoutes();
	StartHttpServer();
}

void FDSMasterService::StartHttpServer()
{
	if (!HttpRouter)
	{
		HttpRouter = FHttpServerModule::Get().GetHttpRouter(HttpServerPort);
		if (!HttpRouter)
		{
			UE_LOG(LogDSMaster, Error, TEXT("DSMasterService couldn't be started on port %d"), HttpServerPort);
			return;
		}

		for (FDSMasterRequestRoute& Route : RegisteredHttpRoutes)
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

void FDSMasterService::StopHttpServer()
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

void FDSMasterService::RegisterRoute(const FDSMasterRequestRoute& Route)
{
	RegisteredHttpRoutes.Add(Route);

	// If the route is registered after the server is already started.
	if (HttpRouter)
	{
		StartRoute(Route);
	}
}

void FDSMasterService::StartRoute(const FDSMasterRequestRoute& Route)
{
	// The handler is wrapped in a lambda since HttpRouter::BindRoute only accepts TFunctions
	ActiveRouteHandles.Add(GetTypeHash(Route),
		HttpRouter->BindRoute(Route.Path, Route.Verb,
			[this, Handler = Route.Handler](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
			{
				return Handler.Execute(Request, OnComplete);
			}));
}

TUniquePtr<FHttpServerResponse> FDSMasterService::CreateHttpResponse(EHttpServerResponseCodes InResponseCode)
{
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), {TEXT("application/json")});
	Response->Code = InResponseCode;
	return Response;
}

void FDSMasterService::ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload)
{
	int32 StartIndex = OutTCHARPayload.Num();
	OutTCHARPayload.AddUninitialized(FUTF8ToTCHAR_Convert::ConvertedLength((ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR)) * sizeof(TCHAR));
	FUTF8ToTCHAR_Convert::Convert((TCHAR*)(OutTCHARPayload.GetData() + StartIndex), (OutTCHARPayload.Num() - StartIndex) / sizeof(TCHAR), (ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR));
}



void FDSMasterService::DumpHttpServerRequest(const FHttpServerRequest& Request, const FString& Title)
{
	UE_LOG(LogDSMaster, Warning, TEXT("--- %s@ServerRequest : %s"), *Title, *Request.RelativePath.GetPath());

	for (auto Header : Request.Headers)
	{
		UE_LOG(LogDSMaster, Warning, TEXT("- Header : %s"), *Header.Key);

		for (auto Value : Header.Value)
		{
			UE_LOG(LogDSMaster, Warning, TEXT("-- Value : %s"), *Value);
		}
	}

	for (auto QueryParam : Request.QueryParams)
	{
		UE_LOG(LogDSMaster, Warning, TEXT("- QueryParam : %s -> %s"), *QueryParam.Key, *QueryParam.Value);
	}

	for (auto PathParam : Request.PathParams)
	{
		UE_LOG(LogDSMaster, Warning, TEXT("- PathParam : %s -> %s"), *PathParam.Key, *PathParam.Value);
	}
}

void FDSMasterService::RegisterRoutes()
{
	RegisterRoute({
		TEXT("Create Session "),
		FHttpPath(TEXT("/session")),
		EHttpServerRequestVerbs::VERB_POST,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleCreateSession)
	});

	RegisterRoute({
		TEXT("Get Session"),
		FHttpPath(TEXT("/session/:session")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleGetSession)
	});

	RegisterRoute({
		TEXT("Update Session"),
		FHttpPath(TEXT("/session/:session")),
		EHttpServerRequestVerbs::VERB_PUT,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleUpdateSession)
	});


	RegisterRoute({
		TEXT("Destroy Session"),
		FHttpPath(TEXT("/session/:session")),
		EHttpServerRequestVerbs::VERB_DELETE,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleDestroySession)
	});

	RegisterRoute({
		TEXT("Get Session List"),
		FHttpPath(TEXT("/sessions")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleGetSessionList)
	});


	RegisterRoute({
		TEXT("Request a Game Session"),
		FHttpPath(TEXT("/game/request-session")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleRequestGameSession)
	});
}

bool FDSMasterService::HandleCreateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleCreateSession"));

	FString ErrorMsg;
	FRPGGameSessionDetails SessionDetails;
	if (JsonHTTPBodyToUStruct(Request.Body, &SessionDetails))
	{
		// debug
		// FOnlineSessionSettings OnlineSessionSettings;
		// FNamedOnlineSession NamedOnlineSession(FName(RPGGameSession.SessionName), OnlineSessionSettings);
		// RPGGameSession.SetupToNamedOnlineSession(&NamedOnlineSession);
		// DumpNamedSession(&NamedOnlineSession);
		
		FRPGGameSessionDetails* NewSession = SessionManager.CreateGameSession(SessionDetails);
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

bool FDSMasterService::HandleDestroySession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
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

bool FDSMasterService::HandleGetSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleGetSession"));

	FString SessionId = Request.PathParams.FindRef(TEXT("session"));
	FString GameMode = Request.QueryParams.FindRef(TEXT("game"));
	if (!GameMode.IsEmpty())
	{
		UE_LOG(LogDSMaster, Log, TEXT("game=%s"), *GameMode);
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

bool FDSMasterService::HandleUpdateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleUpdateSession"));

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
		if (!JsonHTTPBodyToUStruct(Request.Body, &InputSession))
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

bool FDSMasterService::HandleGetSessionList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleGetSessionList"));
	
	FString GameMode = Request.QueryParams.FindRef(TEXT("game"));
	if (!GameMode.IsEmpty())
	{
		UE_LOG(LogDSMaster, Log, TEXT("game=%s"), *GameMode);
	}

	FString ReplyText;
	{
		FHttpSessionSearchResult SearchResult;
		for (auto& KV : SessionManager.GameSessions)
		{
			SearchResult.Sessions.Add(KV.Value);
		}

		FJsonObjectConverter::UStructToJsonObjectString(SearchResult, ReplyText);
	}

	auto Response = FHttpServerResponse::Create(ReplyText, TEXT("application/json"));
	OnComplete(MoveTemp(Response));
	return true;
}

// Maybe take a long time
bool FDSMasterService::HandleRequestGameSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleUpdateSession"));

	FString GameMode = Request.QueryParams.FindRef(TEXT("game"));
	FString ErrorMsg;
	do
	{
		if (GameMode.IsEmpty())
		{
			ErrorMsg = FString::Printf(TEXT("Invalid GameMode : %s"), *GameMode);
			break;
		}

		// TODO: refactor
		

		FString ServerName = TEXT("ShooterServer-Win64-Debug.exe");
		FString BinaryDir = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory());
		FString GameServerExePath = BinaryDir / ServerName;
		GameServerExePath = FPaths::ConvertRelativePathToFull(GameServerExePath);
		FPaths::NormalizeFilename(GameServerExePath);

		FString MapName = TEXT("Sanctuary");
		FString Options = TEXT("game=/Script/ShooterGame.AShooterGame_FreeForAll");
		FString Params = MapName;
		if (Options.Len() > 0)
		{
			Params += FString(TEXT("?")) + Options;
		}

		const int32 ServerPort = ServerManager.AllocateGameServerPort();

		Params += TEXT(" -Log");
		Params += FString::Printf(TEXT(" -Port=%d"), ServerPort);
		
		FGameServerLaunchSettings LaunchSettings;
		LaunchSettings.URL = GameServerExePath;
		LaunchSettings.Params = Params;
		ServerManager.LaunchGameServer(LaunchSettings);

		FString ReplyText;
		auto Response = FHttpServerResponse::Create(ReplyText, TEXT("application/json"));
		OnComplete(MoveTemp(Response));
		return true;
		
	} while (true);

	// Error
	auto Response = FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT("ServerError"), ErrorMsg);
	OnComplete(MoveTemp(Response));
	return true;
}

void FDSMasterService::AddReferencedObjects(FReferenceCollector& Collector)
{
	if (BeaconHost)
	{
		Collector.AddReferencedObject(BeaconHost);
	}

	if (DSMasterHost)
	{
		Collector.AddReferencedObject(DSMasterHost);
	}

	if (DSMasterClient)
	{
		Collector.AddReferencedObject(DSMasterClient);
	}
}

AOnlineBeaconHost* FDSMasterService::CreateDSMasterBeaconHost(UWorld* World, const FDSMasterBeaconHostSettings& InBeaconHostSettings)
{
	BeaconHostSettings = InBeaconHostSettings;
	if (BeaconHostSettings.RunningMode == EDSMasterMode::None)
	{
		UE_LOG(LogDSMaster, Warning, TEXT("CreateBeaconHost - DSMasterMode=None"));
		return nullptr;
	}

	BeaconHost = World->SpawnActor<AOnlineBeaconHost>(AOnlineBeaconHost::StaticClass());
	if (!BeaconHost)
	{
		UE_LOG(LogDSMaster, Error, TEXT("CreateBeaconHost - Spawn AOnlineBeaconHost Failed"));
		return nullptr;
	}

	if (InBeaconHostSettings.ListenPort > 0)
	{
		BeaconHost->ListenPort = InBeaconHostSettings.ListenPort;
	}

	if (!BeaconHost->InitHost())
	{
		UE_LOG(LogDSMaster, Error, TEXT("CreateBeaconHost - InitHost Failed"));
		return nullptr;
	}

	// Set Beacon state : EBeaconState::AllowRequests
	BeaconHost->PauseBeaconRequests(false);

	UE_LOG(LogDSMaster, Log, TEXT("Init Master Beacon Host, ListenPort=%d"), BeaconHost->GetListenPort())

	if (BeaconHostSettings.RunningMode == EDSMasterMode::Agent || BeaconHostSettings.RunningMode == EDSMasterMode::Master)
	{
		TSubclassOf<ADSMasterBeaconHost> BeaconHostObjectClass = BeaconHostSettings.DSMasterBeaconHostObjectClass ? BeaconHostSettings.DSMasterBeaconHostObjectClass : ADSMasterBeaconHost::StaticClass();
		DSMasterHost = World->SpawnActor<ADSMasterBeaconHost>(BeaconHostObjectClass);
		if (DSMasterHost)
		{
			BeaconHost->RegisterHost(DSMasterHost);
			UE_LOG(LogDSMaster, Log, TEXT("RegisterHost - Master Host : %s"), *BeaconHostObjectClass->GetName());
		}
	}
	return BeaconHost;
}


bool FDSMasterService::ConnectToMasterServer(UWorld* World, const FDSMasterClientSettings& InClientSetting)
{
	DSMasterClient = World->SpawnActor<ADSMasterBeaconClient>(InClientSetting.DSMasterBeaconClientClass);
	if (DSMasterClient)
	{
		DSMasterClient->Settings = InClientSetting;
		return DSMasterClient->ConnectToMasterServer(InClientSetting.MasterServerAddress);
	}
	return false;
}