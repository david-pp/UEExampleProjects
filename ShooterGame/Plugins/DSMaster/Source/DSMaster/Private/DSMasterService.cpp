// Fill out your copyright notice in the Description page of Project Settings.

#include "DSMasterService.h"
#include "DSMaster.h"
#include "DSMasterBeaconHost.h"
#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"

FString FDSMasterService::SettingFileName = TEXT("DSMasterService.json");

bool FDSMasterService::InitServer(bool bRunAsMaster)
{
	GameServerStoppedHandle = ServerManager.OnGameServerStopped.AddRaw(this, &FDSMasterService::OnGameServerStopped);
	TickerHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FDSMasterService::ServerTick), 0.1);

	// load config
	if (LoadServiceSettingFromJsonFile(SettingFileName))
	{
	}
	
	DSMasterName = TEXT("Default");

	// Run as Master Server
	if (FParse::Value(FCommandLine::Get(), TEXT("Master="), DSMasterName) ||
		FParse::Param(FCommandLine::Get(), TEXT("Master")) || bRunAsMaster)
	{
		// Run HTTP Server
		FParse::Value(FCommandLine::Get(), TEXT("MasterHttpPort="), HttpServerPort);
		InitHttpServer(HttpServerPort);

		// Run Master Beacon Server
		BeaconHostSettings.ListenPort = 9000;
		BeaconHostSettings.RunningMode = EDSMasterMode::Master;
		BeaconHostSettings.DSMasterBeaconHostObjectClass = ADSMasterBeaconHost::StaticClass();

		// Override listen port
		FParse::Value(FCommandLine::Get(), TEXT("MasterPort="), BeaconHostSettings.ListenPort);
	}
	// Run as Agent Server
	else if (FParse::Value(FCommandLine::Get(), TEXT("Agent="), DSMasterName) ||
		FParse::Param(FCommandLine::Get(), TEXT("Agent")))
	{
		// Run Agent Beacon Server
		BeaconHostSettings.ListenPort = 9001;
		BeaconHostSettings.RunningMode = EDSMasterMode::Agent;
		BeaconHostSettings.DSMasterBeaconHostObjectClass = ADSMasterBeaconHost::StaticClass();

		FParse::Value(FCommandLine::Get(), TEXT("AgentPort="), BeaconHostSettings.ListenPort);
	}

	return true;
}

bool FDSMasterService::StartDSMasterHttpService()
{
	if (GetRunningMode() == EDSMasterMode::Master)
	{
		// Run HTTP Server
		FParse::Value(FCommandLine::Get(), TEXT("MasterHttpPort="), HttpServerPort);
		InitHttpServer(HttpServerPort);
		return true;
	}

	return false;
}

bool FDSMasterService::StartDSMasterServer(UWorld* World)
{
	if (GetRunningMode() == EDSMasterMode::Master)
	{
		// Master Server
		CreateDSMasterBeaconHost(World, BeaconHostSettings);
	}
	else if (GetRunningMode() == EDSMasterMode::Agent)
	{
		// Agent Server
		CreateDSMasterBeaconHost(World, BeaconHostSettings);

		// Connect to Master Server
		FDSMasterClientSettings ClientSettings;
		ClientSettings.ClientType = EDSMasterClientType::Agent;
		ClientSettings.DSMasterBeaconClientClass = ADSMasterBeaconClient::StaticClass();
		FParse::Value(FCommandLine::Get(), TEXT("MasterServer="), ClientSettings.MasterServerAddress);

		ConnectToMasterServer(World, ClientSettings);
	}

	// Game Server Pools
	LaunchGameServersByConfig();

	return true;
}

void FDSMasterService::StopServer()
{
	StopHttpServer();

	ServerManager.StopAllGameServers();

	if (TickerHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(TickerHandle);
	}

	ServerManager.OnGameServerStopped.Remove(GameServerStoppedHandle);
}

bool FDSMasterService::LoadServiceSettingFromJsonFile(const FString& JsonFileName)
{
	const FString FilePath = FPaths::ProjectConfigDir() / JsonFileName;
	if (FPaths::FileExists(FilePath))
	{
		FString JsonString;
		if (FFileHelper::LoadFileToString(JsonString, *FilePath))
		{
			if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &Settings))
			{
				UE_LOG(LogDSMaster, Log, TEXT("Load services settings sucess : \n%s"), *JsonString);
				return true;
			}
		}
	}
	return false;
}

bool FDSMasterService::SaveServiceSettingToJsonFile(const FString& JsonFileName)
{
	const FString FilePath = FPaths::ProjectConfigDir() / JsonFileName;

	FString JsonString;
	if (FJsonObjectConverter::UStructToJsonObjectString(Settings, JsonString))
	{
		if (FFileHelper::SaveStringToFile(JsonString, *FilePath))
		{
			UE_LOG(LogDSMaster, Log, TEXT("Save services settings to : %s"), *FilePath);
			return true;
		}
	}

	return false;
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
			if (Tuple.Key && Tuple.Value.IsValid())
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
	UE_LOG(LogDSMaster, Verbose, TEXT("--- %s@ServerRequest : %s"), *Title, *Request.RelativePath.GetPath());

	for (auto Header : Request.Headers)
	{
		UE_LOG(LogDSMaster, Verbose, TEXT("- Header : %s"), *Header.Key);

		for (auto Value : Header.Value)
		{
			UE_LOG(LogDSMaster, Verbose, TEXT("-- Value : %s"), *Value);
		}
	}

	for (auto QueryParam : Request.QueryParams)
	{
		UE_LOG(LogDSMaster, Verbose, TEXT("- QueryParam : %s -> %s"), *QueryParam.Key, *QueryParam.Value);
	}

	for (auto PathParam : Request.PathParams)
	{
		UE_LOG(LogDSMaster, Verbose, TEXT("- PathParam : %s -> %s"), *PathParam.Key, *PathParam.Value);
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
		TEXT("Update Session State"),
		FHttpPath(TEXT("/session/:session/state")),
		EHttpServerRequestVerbs::VERB_PUT,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleUpdateSessionState)
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
		FHttpPath(TEXT("/gamesession/request")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleRequestGameSession)
	});
	
	RegisterRoute({
		TEXT("Request a Game Session"),
		FHttpPath(TEXT("/gamesession/find")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateRaw(this, &FDSMasterService::HandleRequestFindGameSession)
	});
}

bool FDSMasterService::HandleCreateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleCreateSession"));

	FString ErrorMsg;
	FGameSessionDetails SessionDetails;
	if (JsonHTTPBodyToUStruct(Request.Body, &SessionDetails))
	{
		FGameSessionDetails* NewSession = SessionManager.CreateGameSession(SessionDetails);
		if (NewSession)
		{
			NewSession->SetSessionState(EOnlineSessionState::Pending);
			// Game server is launched by DSMaster
			if (!NewSession->ServerGuid.IsEmpty())
			{
				FGuid ServerGuid;
				FGuid::Parse(NewSession->ServerGuid, ServerGuid);
				FGameServerProcessPtr GameServer = ServerManager.FindGameServer(ServerGuid);
				if (GameServer)
				{
					GameServer->UpdateRunningSession(NewSession->SessionId);
				}
			}
			// Game server is launched manually (same vm with DSMaster)
			else
			{
				FGameServerProcessPtr GameServer = ServerManager.MonitorGameServer(SessionDetails.ServerPID);
				if (GameServer)
				{
					NewSession->ServerGuid = GameServer->GetProcessGuid().ToString();
					GameServer->UpdateRunningSession(NewSession->SessionId);
				}
			}
			
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
		FGameSessionDetails* Session = SessionManager.GetGameSession(SessionId);
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
		FGameSessionDetails* Session = SessionManager.GetGameSession(SessionId);
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
		FGameSessionDetails* ExistSession = SessionManager.GetGameSession(SessionId);
		if (!ExistSession)
		{
			ErrorMsg = FString::Printf(TEXT("Invalid SessionId : %s"), *SessionId);
			break;
		}

		FGameSessionDetails InputSession;
		if (!JsonHTTPBodyToUStruct(Request.Body, &InputSession))
		{
			ErrorMsg = FString::Printf(TEXT("Invalid session protocal"));
			break;
		}

		// update
		FGameSessionDetails* UpdatedSession = SessionManager.UpdateGameSession(SessionId, InputSession);

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

bool FDSMasterService::HandleUpdateSessionState(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleUpdateSessionState"));

	FString SessionId = Request.PathParams.FindRef(TEXT("session"));

	FGameSessionUpdateReply Reply;
	do
	{
		FGameSessionDetails* ExistSession = SessionManager.GetGameSession(SessionId);
		if (!ExistSession)
		{
			Reply.ErrorMessage = FString::Printf(TEXT("Invalid SessionId : %s"), *SessionId);
			break;
		}

		FGameSessionUpdateStateRequest UpdateRequest;
		if (!JsonHTTPBodyToUStruct(Request.Body, &UpdateRequest))
		{
			Reply.ErrorMessage = FString::Printf(TEXT("Invalid session protocal"));
			break;
		}

		// Before
		const EOnlineSessionState::Type StateFrom = ExistSession->GetSessionState();

		// Update
		ExistSession->SetSessionState(UpdateRequest.GetSessionState());

		// After
		const EOnlineSessionState::Type StateTo = ExistSession->GetSessionState();

		UE_LOG(LogDSMaster, Log, TEXT("Update Session : %s, %s->%s"),
			*ExistSession->SessionId, EOnlineSessionState::ToString(StateFrom), EOnlineSessionState::ToString(StateTo))

		if (EOnlineSessionState::Ended == StateTo)
		{
			// Pending it and can be used again
			ExistSession->SetSessionState(EOnlineSessionState::Pending);
		}
		else if (EOnlineSessionState::Destroying == StateTo)
		{
			SessionManager.DestroyGameSession(SessionId);
		}

	} while (false);

	auto Response = CreateHttpJsonResponse(Reply);
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
		FGameSessionSearchResult SearchResult;
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
	DumpHttpServerRequest(Request, TEXT("HandleRequestGameSession"));

	FString GameMode = Request.QueryParams.FindRef(TEXT("game"));
	FString GameMap = Request.QueryParams.FindRef(TEXT("map"));
	FString ErrorMsg;
	do
	{
		if (GameMap.IsEmpty())
		{
			ErrorMsg = FString::Printf(TEXT("Invalid Game Map : %s"), *GameMode);
			break;
		}

		FGameSessionSearchResult SearchResult;
		FGameSessionDetails* PendingSession = SessionManager.FindOnePendingSession(GameMap);
		if (PendingSession)
		{
			// Pending -> Starting
			SearchResult.Sessions.Add(*PendingSession);
			PendingSession->SetSessionState(EOnlineSessionState::Starting);
		}
		else
		{
			SearchResult.Sessions.Empty();
			LaunchGameServerByMapBucket(GameMap);
		}

		FString ReplyText;
		FJsonObjectConverter::UStructToJsonObjectString(SearchResult, ReplyText);
		auto Response = FHttpServerResponse::Create(ReplyText, TEXT("application/json"));
		OnComplete(MoveTemp(Response));
		return true;
		
	} while (true);

	// Error
	auto Response = FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT("ServerError"), ErrorMsg);
	OnComplete(MoveTemp(Response));
	return true;
}

bool FDSMasterService::HandleRequestFindGameSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	DumpHttpServerRequest(Request, TEXT("HandleRequestGameSession"));

	FString GameMode = Request.QueryParams.FindRef(TEXT("game"));
	FString GameMap = Request.QueryParams.FindRef(TEXT("map"));
	FString ErrorMsg;
	do
	{
		if (GameMap.IsEmpty())
		{
			ErrorMsg = FString::Printf(TEXT("Invalid Game Map : %s"), *GameMode);
			break;
		}

		FGameSessionSearchResult SearchResult;
		FGameSessionDetails* PendingSession = SessionManager.FindOnePendingSession(GameMap);
		if (PendingSession)
		{
			// Pending -> Starting
			SearchResult.Sessions.Add(*PendingSession);
			PendingSession->SetSessionState(EOnlineSessionState::Starting);
		}

		FString ReplyText;
		FJsonObjectConverter::UStructToJsonObjectString(SearchResult, ReplyText);
		auto Response = FHttpServerResponse::Create(ReplyText, TEXT("application/json"));
		OnComplete(MoveTemp(Response));
		return true;
		
	} while (true);

	// Error
	auto Response = FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT("ServerError"), ErrorMsg);
	OnComplete(MoveTemp(Response));
	return true;
}

void FDSMasterService::OnGameServerLaunched(FGameServerProcessPtr GameServer)
{
}

void FDSMasterService::OnGameServerStopped(FGameServerProcessPtr GameServer, bool bIsCanceled)
{
	if (!GameServer) return;

	const FString SessionId = GameServer->GetProcessInfo().SessionId;

	FGameSessionDetails* GameSessionDetails = SessionManager.GetGameSession(SessionId);
	if (GameSessionDetails)
	{
		SessionManager.DestroyGameSession(SessionId);
	}
}

bool FDSMasterService::ServerTick(float DeltaTime)
{
	ServerManager.CheckAndUpdateGameServers();
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


FString FDSMasterService::GetGameServerExePath() const
{
	FString ServerDir = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory());
	if (Settings.GameServer.ServerDirectory.Len() > 0)
	{
		ServerDir = Settings.GameServer.ServerDirectory;
	}
	
	FString ServerName = TEXT("ShooterServer-Win64-Debug.exe");
	if (Settings.GameServer.ServerName.Len() > 0)
	{
		ServerName = Settings.GameServer.ServerName;
	}

	FString GameServerExePath = ServerDir / ServerName;
	GameServerExePath = FPaths::ConvertRelativePathToFull(GameServerExePath);
	FPaths::NormalizeFilename(GameServerExePath);

	if (!FPaths::FileExists(GameServerExePath))
	{
		UE_LOG(LogDSMaster, Error, TEXT("GetGameServerExePath - Invalid Server Path : %s"), *GameServerExePath);
	}

	return GameServerExePath;
}

FGameServerMapSettings* FDSMasterService::FindGameMapSettings(const FString& MapBucket)
{
	for (auto& ServerMap : Settings.GameServer.ServerMaps)
	{
		if (ServerMap.MapBucket == MapBucket)
			return &ServerMap;
	}
	return nullptr;
}

bool FDSMasterService::LaunchGameServerByMapBucket(const FString& MapBucket)
{
	FGameServerMapSettings* MapSettings = FindGameMapSettings(MapBucket);
	if (!MapSettings)
	{
		UE_LOG(LogDSMaster, Error, TEXT("LaunchOneGameServer - Invalid MapID: %s"), *MapBucket);
		return false;
	}

	return LaunchOneGameServer(*MapSettings);
}

bool FDSMasterService::LaunchOneGameServer(const FGameServerMapSettings& MapSettings)
{
	// FString MapName = TEXT("Sanctuary");
	// FString Options = TEXT("game=/Script/ShooterGame.AShooterGame_FreeForAll");
	
	FString Options;
	if (MapSettings.DefaultGameMode.Len() > 0)
	{
		// TODO: check game mode validation
		Options = FString::Printf(TEXT("game=%s"), *MapSettings.DefaultGameMode);
	}

	FString Params = MapSettings.MapName;
	if (Options.Len() > 0)
	{
		Params += FString(TEXT("?")) + Options;
	}

	const int32 ServerPort = ServerManager.AllocateGameServerPort();

	Params += TEXT(" -Log");
	Params += FString::Printf(TEXT(" -Port=%d"), ServerPort);
	Params += FString::Printf(TEXT(" -BucketId=%s"), *MapSettings.MapBucket);
		
	FGameServerLaunchSettings LaunchSettings;
	LaunchSettings.MapBucket = MapSettings.MapBucket;
	LaunchSettings.URL = GetGameServerExePath();
	LaunchSettings.Params = Params;
	LaunchSettings.bCreatePipes = false;
	LaunchSettings.bCreateThread = false;
	return ServerManager.LaunchGameServer(LaunchSettings);
}

void FDSMasterService::LaunchGameServersByConfig()
{
	for (auto& ServerMap : Settings.GameServer.ServerMaps)
	{
		for (int32 Count = 0; Count < ServerMap.MinInstances; Count++)
		{
			LaunchOneGameServer(ServerMap);
		}
	} 
}
