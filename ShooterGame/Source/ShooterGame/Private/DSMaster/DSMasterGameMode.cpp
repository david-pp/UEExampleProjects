// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMaster/DSMasterGameMode.h"

#include "DSMasterBeaconClient.h"
#include "HttpModule.h"
#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "IHttpResponse.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"

ADSMasterGameMode::ADSMasterGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// PlayerControllerClass = AShooterPlayerController_Menu::StaticClass();
	DSMasterBeaconHostObjectClass = ADSMasterBeaconHost::StaticClass();
	DSBeaconHostObjectClass = ADSBeaconHost::StaticClass();

	DSMasterBeaconClientClass = ADSMasterBeaconClient::StaticClass();

	DSMaterServerAddress = TEXT("127.0.0.1:15000");


	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &ADSMasterGameMode::OnCreateSessionComplete);
	}
}

// Called when the game starts or when spawned
void ADSMasterGameMode::BeginPlay()
{
	Super::BeginPlay();
}

void ADSMasterGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	StopHttpServer();
}

void ADSMasterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// Setup DSMasterMode
	FString DSMasterModeStr;
	if (FParse::Value(FCommandLine::Get(), TEXT("DSMaster="), DSMasterModeStr))
	{
		if (DSMasterModeStr.Compare(TEXT("Manager"), ESearchCase::IgnoreCase) == 0)
		{
			DSMasterMode = EDSMasterMode::Manager;
		}
		else if (DSMasterModeStr.Compare(TEXT("Agent"), ESearchCase::IgnoreCase) == 0)
		{
			DSMasterMode = EDSMasterMode::Agent;
		}
		else
		{
			DSMasterMode = EDSMasterMode::None;
		}
	}
	else
	{
		if (FParse::Param(FCommandLine::Get(), TEXT("DSMaster")))
		{
			DSMasterMode = EDSMasterMode::AllInOne;
		}
		else
		{
			DSMasterMode = EDSMasterMode::None;
		}
	}

	UEnum* MasterModeEnum = StaticEnum<EDSMasterMode>();
	if (MasterModeEnum)
	{
		UE_LOG(LogDSMaster, Log, TEXT("DSMasterMode = %s"), *MasterModeEnum->GetNameStringByValue(static_cast<int64>(DSMasterMode)));
	}

	// Setup DSMasterServer Address
	FString DSMasterServerStr;
	if (FParse::Value(FCommandLine::Get(), TEXT("DSMasterServer="), DSMasterServerStr))
	{
		DSMaterServerAddress = DSMasterServerStr;
	}

	UE_LOG(LogDSMaster, Log, TEXT("DSMaterServerAddress = %s"), *DSMaterServerAddress);

	// Run as HTTP Server
	RegisterRoutes();
	StartHttpServer();
}

// Called every frame
void ADSMasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADSMasterGameMode::StartPlay()
{
	Super::StartPlay();

	CreateBeaconHost();

	if (IsAgent())
	{
		ConnectToMasterServer(DSMaterServerAddress);
	}
}

bool ADSMasterGameMode::IsManager() const
{
	return DSMasterMode == EDSMasterMode::AllInOne || DSMasterMode == EDSMasterMode::Manager;
}

bool ADSMasterGameMode::IsAgent() const
{
	return DSMasterMode == EDSMasterMode::AllInOne || DSMasterMode == EDSMasterMode::Agent;
}

void ADSMasterGameMode::StartHttpServer()
{
	if (!HttpRouter)
	{
		HttpRouter = FHttpServerModule::Get().GetHttpRouter(HttpServerPort);
		if (!HttpRouter)
		{
			UE_LOG(LogDSMaster, Error, TEXT("DS Master server couldn't be started on port %d"), HttpServerPort);
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

void ADSMasterGameMode::StopHttpServer()
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

void ADSMasterGameMode::RegisterRoute(const FDSMasterRequestRoute& Route)
{
	RegisteredHttpRoutes.Add(Route);

	// If the route is registered after the server is already started.
	if (HttpRouter)
	{
		StartRoute(Route);
	}
}

void ADSMasterGameMode::StartRoute(const FDSMasterRequestRoute& Route)
{
	// The handler is wrapped in a lambda since HttpRouter::BindRoute only accepts TFunctions
	ActiveRouteHandles.Add(GetTypeHash(Route),
		HttpRouter->BindRoute(Route.Path, Route.Verb,
			[this, Handler = Route.Handler](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
			{
				return Handler.Execute(Request, OnComplete);
			}));
}

void ADSMasterGameMode::RegisterRoutes()
{
	RegisterRoute({
		TEXT("Get information of sessions"),
		FHttpPath(TEXT("/session/info")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateUObject(this, &ADSMasterGameMode::HandleSessionInfoRoute)
	});
	RegisterRoute({
		TEXT("Create Game Session"),
		FHttpPath(TEXT("/session/create")),
		EHttpServerRequestVerbs::VERB_POST,
		FDSMasterRequestHandlerDelegate::CreateUObject(this, &ADSMasterGameMode::HandleCreateSession)
	});
	
	RegisterRoute({
		TEXT("REST API Test"),
		FHttpPath(TEXT("/rest/devices/:id")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateUObject(this, &ADSMasterGameMode::HandleRestApi)
	});

	RegisterRoute({
		TEXT("REST API Test"),
		FHttpPath(TEXT("/sessions")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateUObject(this, &ADSMasterGameMode::HandleRestApi_GetSessionList)
	});

	RegisterRoute({
		TEXT("REST API Test"),
		FHttpPath(TEXT("/sessions/:session")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateUObject(this, &ADSMasterGameMode::HandleRestApi_GetSession)
	});

	RegisterRoute({
		TEXT("REST API Test"),
		FHttpPath(TEXT("/sessions/:session/attributes")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateUObject(this, &ADSMasterGameMode::HandleRestApi_GetSessionAttributeList)
	});

	RegisterRoute({
		TEXT("REST API Test"),
		FHttpPath(TEXT("/sessions/:session/attributes/:attribute")),
		EHttpServerRequestVerbs::VERB_GET,
		FDSMasterRequestHandlerDelegate::CreateUObject(this, &ADSMasterGameMode::HandleRestApi_GetSessionAttribute)
	});
}

bool ADSMasterGameMode::HandleSessionInfoRoute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FString Text = TEXT("aha....");
	
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), { "application/json" });
	Response->Code = EHttpServerResponseCodes::Ok;

	FTCHARToUTF8 ConvertToUtf8(*Text);
	const uint8* ConvertToUtf8Bytes = (reinterpret_cast<const uint8*>(ConvertToUtf8.Get()));
	Response->Body.Append(ConvertToUtf8Bytes, ConvertToUtf8.Length());
	
	OnComplete(MoveTemp(Response));
	return true;
}

void ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload)
{
	int32 StartIndex = OutTCHARPayload.Num();
	OutTCHARPayload.AddUninitialized(FUTF8ToTCHAR_Convert::ConvertedLength((ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR)) * sizeof(TCHAR));
	FUTF8ToTCHAR_Convert::Convert((TCHAR*)(OutTCHARPayload.GetData() + StartIndex), (OutTCHARPayload.Num() - StartIndex) / sizeof(TCHAR), (ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR));
}

typedef TJsonWriterFactory< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriterFactory;
typedef TJsonWriter< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriter;

typedef TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriterFactory;
typedef TJsonWriter< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriter;

bool ADSMasterGameMode::HandleCreateSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	/** Holds the request's TCHAR payload. */
	TArray<uint8> TCHARBody;
	ConvertToTCHAR(Request.Body, TCHARBody);

	FMemoryReaderView BodyReader(TCHARBody);

	// Parse Request
	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
	TSharedRef<TJsonReader<TCHAR>> JsonReader =  TJsonReaderFactory<TCHAR>::Create(&BodyReader);
	FJsonSerializer::Deserialize(JsonReader, JsonObject);

	FString SessionName = JsonObject->GetStringField(TEXT("SessionName"));

	// TODO :
	UE_LOG(LogDSMaster, Log, TEXT("HandleCreateSession - %s"), *SessionName);

	// Reply 
	FString ReplyText = TEXT("aha....");
	TSharedPtr<FJsonObject> ReplyObject = MakeShareable(new FJsonObject());
	ReplyObject->SetStringField(TEXT("SessionName"), SessionName);
	
	TSharedRef<FCondensedJsonStringWriter> Writer = FCondensedJsonStringWriterFactory::Create(&ReplyText);
	if (FJsonSerializer::Serialize(ReplyObject.ToSharedRef(), Writer))
	{
	}
	
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), { "application/json" });
	Response->Code = EHttpServerResponseCodes::Ok;

	FTCHARToUTF8 ConvertToUtf8(*ReplyText);
	const uint8* ConvertToUtf8Bytes = (reinterpret_cast<const uint8*>(ConvertToUtf8.Get()));
	Response->Body.Append(ConvertToUtf8Bytes, ConvertToUtf8.Length());
	
	OnComplete(MoveTemp(Response));
	return true;
}

void DumpServerRequest(const FHttpServerRequest& Request)
{
	UE_LOG(LogDSMaster, Warning, TEXT("--- ServerRequest : %s"), *Request.RelativePath.GetPath());

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

bool ADSMasterGameMode::HandleRestApi(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FString Id = Request.PathParams.FindChecked(TEXT("id"));

	DumpServerRequest(Request);

	UE_LOG(LogDSMaster, Warning, TEXT("HandleRestApi - %s"), *Id);
	
	FString Text = TEXT("done");
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), { "application/json" });
	Response->Code = EHttpServerResponseCodes::Ok;

	FTCHARToUTF8 ConvertToUtf8(*Text);
	const uint8* ConvertToUtf8Bytes = (reinterpret_cast<const uint8*>(ConvertToUtf8.Get()));
	Response->Body.Append(ConvertToUtf8Bytes, ConvertToUtf8.Length());
	
	OnComplete(MoveTemp(Response));
	return true;
}

bool ADSMasterGameMode::HandleRestApi_GetSessionList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	UE_LOG(LogDSMaster, Warning, TEXT("HandleRestApi_GetSessionList"));
	
	DumpServerRequest(Request);
	
	FString Text = TEXT("done");
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), { "application/json" });
	Response->Code = EHttpServerResponseCodes::Ok;

	FTCHARToUTF8 ConvertToUtf8(*Text);
	const uint8* ConvertToUtf8Bytes = (reinterpret_cast<const uint8*>(ConvertToUtf8.Get()));
	Response->Body.Append(ConvertToUtf8Bytes, ConvertToUtf8.Length());
	
	OnComplete(MoveTemp(Response));
	return true;
}

bool ADSMasterGameMode::HandleRestApi_GetSession(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FString SessionName = Request.PathParams.FindChecked(TEXT("session"));
	
	UE_LOG(LogDSMaster, Warning, TEXT("HandleRestApi_GetSession - %s"), *SessionName);
	
	DumpServerRequest(Request);
	
	FString Text = TEXT("done");
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), { "application/json" });
	Response->Code = EHttpServerResponseCodes::Ok;

	FTCHARToUTF8 ConvertToUtf8(*Text);
	const uint8* ConvertToUtf8Bytes = (reinterpret_cast<const uint8*>(ConvertToUtf8.Get()));
	Response->Body.Append(ConvertToUtf8Bytes, ConvertToUtf8.Length());
	
	OnComplete(MoveTemp(Response));
	return true;
}

bool ADSMasterGameMode::HandleRestApi_GetSessionAttributeList(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FString SessionName = Request.PathParams.FindChecked(TEXT("session"));
	
	UE_LOG(LogDSMaster, Warning, TEXT("HandleRestApi_GetSessionAttributeList - %s"), *SessionName);
	
	DumpServerRequest(Request);
	
	FString Text = TEXT("done");
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), { "application/json" });
	Response->Code = EHttpServerResponseCodes::Ok;

	FTCHARToUTF8 ConvertToUtf8(*Text);
	const uint8* ConvertToUtf8Bytes = (reinterpret_cast<const uint8*>(ConvertToUtf8.Get()));
	Response->Body.Append(ConvertToUtf8Bytes, ConvertToUtf8.Length());
	
	OnComplete(MoveTemp(Response));
	return true;
}

bool ADSMasterGameMode::HandleRestApi_GetSessionAttribute(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FString SessionName = Request.PathParams.FindChecked(TEXT("session"));
	FString AttributeName = Request.PathParams.FindChecked(TEXT("attribute"));
	
	UE_LOG(LogDSMaster, Warning, TEXT("HandleRestApi_GetSessionAttribute - %s, %s"), *SessionName, *AttributeName);
	
	DumpServerRequest(Request);
	
	FString Text = TEXT("done");
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Headers.Add(TEXT("content-type"), { "application/json" });
	Response->Code = EHttpServerResponseCodes::Ok;

	FTCHARToUTF8 ConvertToUtf8(*Text);
	const uint8* ConvertToUtf8Bytes = (reinterpret_cast<const uint8*>(ConvertToUtf8.Get()));
	Response->Body.Append(ConvertToUtf8Bytes, ConvertToUtf8.Length());
	
	OnComplete(MoveTemp(Response));
	return true;
}

void ADSMasterGameMode::DebugRequestSessionInfo()
{
	static const FString DiscoveryURL = TEXT("http://127.0.0.1:30000/session/info");
	
	auto HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(DiscoveryURL);
	HttpRequest->SetVerb(TEXT("GET"));
	HttpRequest->OnProcessRequestComplete().BindLambda([](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		if (bSucceeded && HttpResponse->GetContentType() == "application/json")
		{
			TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
			TSharedRef<TJsonReader<TCHAR>> JsonReader =  TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
			FJsonSerializer::Deserialize(JsonReader, JsonObject);
			// SomeOtherVariable = JsonObject->GetStringField("some_response_field");

			UE_LOG(LogDSMaster, Warning, TEXT("Reply from Server:%s"), *HttpResponse->GetContentAsString());
		}
		else
		{
			// Handle error here
		}
	});

	HttpRequest->ProcessRequest();
}

void ADSMasterGameMode::DebugCreateSession()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = OnlineSub->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			TSharedPtr<class FOnlineSessionSettings> ShooterHostSettings = MakeShareable(new FOnlineSessionSettings);
			ShooterHostSettings->Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
			ShooterHostSettings->Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
			ShooterHostSettings->Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineDataAdvertisementType::DontAdvertise);
			ShooterHostSettings->Set(SETTING_GAMEMODE, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::ViaOnlineService);
			ShooterHostSettings->Set(SETTING_MAPNAME, GetWorld()->GetMapName(), EOnlineDataAdvertisementType::ViaOnlineService);
			ShooterHostSettings->bAllowInvites = true;
			ShooterHostSettings->bIsDedicated = true;
			if (FParse::Param(FCommandLine::Get(), TEXT("forcelan")))
			{
				UE_LOG(LogOnlineGame, Log, TEXT("Registering server as a LAN server"));
				ShooterHostSettings->bIsLANMatch = true;
			}
			HostSettings = ShooterHostSettings;
			OnCreateSessionCompleteDelegateHandle = SessionInt->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
			SessionInt->CreateSession(0, NAME_GameSession, *HostSettings);
		}
	}
}

void ADSMasterGameMode::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogDSMaster, Warning,TEXT("OnCreateSessionComplete --  %s"), *SessionName.ToString());
}

bool ADSMasterGameMode::CreateBeaconHost()
{
	if (DSMasterMode == EDSMasterMode::None)
	{
		UE_LOG(LogDSMaster, Warning, TEXT("CreateBeaconHost - DSMasterMode=None"));
		return false;
	}

	BeaconHost = GetWorld()->SpawnActor<AOnlineBeaconHost>(AOnlineBeaconHost::StaticClass());
	if (!BeaconHost)
	{
		UE_LOG(LogDSMaster, Error, TEXT("CreateBeaconHost - Spawn AOnlineBeaconHost Failed"));
		return false;
	}

	if (BeaconHostPort > 0)
	{
		BeaconHost->ListenPort = BeaconHostPort;
	}

	if (!BeaconHost->InitHost())
	{
		UE_LOG(LogDSMaster, Error, TEXT("CreateBeaconHost - InitHost Failed"));
		return false;
	}

	// Set Beacon state : EBeaconState::AllowRequests
	BeaconHost->PauseBeaconRequests(false);

	UE_LOG(LogDSMaster, Log, TEXT("Init Master Beacon Host, ListenPort=%d"), BeaconHost->GetListenPort())

	if (DSMasterMode == EDSMasterMode::AllInOne || DSMasterMode == EDSMasterMode::Manager)
	{
		if (DSMasterBeaconHostObjectClass)
		{
			DSMasterHost = GetWorld()->SpawnActor<ADSMasterBeaconHost>(DSMasterBeaconHostObjectClass);
			if (DSMasterHost)
			{
				BeaconHost->RegisterHost(DSMasterHost);
				UE_LOG(LogDSMaster, Log, TEXT("RegisterHost - Master Host : %s"), *DSMasterBeaconHostObjectClass->GetName());
			}
		}
		else
		{
			UE_LOG(LogDSMaster, Warning, TEXT("RegisterHost - Master Host : Invalid"));
		}
	}

	// if (DSMasterMode == EDSMasterMode::AllInOne || DSMasterMode == EDSMasterMode::Agent)
	{
		if (DSBeaconHostObjectClass)
		{
			DSHost = GetWorld()->SpawnActor<ADSBeaconHost>(DSBeaconHostObjectClass);
			if (DSHost)
			{
				BeaconHost->RegisterHost(DSHost);
				UE_LOG(LogDSMaster, Log, TEXT("RegisterHost - DS Host : %s"), *DSMasterBeaconHostObjectClass->GetName());
			}
		}
		else
		{
			UE_LOG(LogDSMaster, Warning, TEXT("RegisterHost - DS Host : Invalid"));
		}
	}

	return true;
}

bool ADSMasterGameMode::ConnectToMasterServer(FString ServerAddress)
{
	DSMasterClient = GetWorld()->SpawnActor<ADSMasterBeaconClient>(DSMasterBeaconClientClass);
	if (DSMasterClient)
	{
		DSMasterClient->BeaconClientType = EDSMasterBeaconClientType::DSAgent;
		return DSMasterClient->ConnectToMasterServer(ServerAddress);
	}
	return false;
}

bool ADSMasterGameMode::CreateGameServerInstance(FDSMasterGameSessionSettings SessionSettings)
{
	FString BinaryDir = FPaths::Combine(*FPaths::ProjectDir(), TEXT("Binaries"), FPlatformProcess::GetBinariesSubdirectory());
	FString GameServerExePath = BinaryDir / ServerName;
	GameServerExePath = FPaths::ConvertRelativePathToFull(GameServerExePath);
	FPaths::NormalizeFilename(GameServerExePath);

	FString Cmd = SessionSettings.MapName;
	if (SessionSettings.Options.Len() > 0)
	{
		Cmd += FString(TEXT("?")) + SessionSettings.Options;
	}

	const int32 ServerPort = AllocateServerPort();

	Cmd += TEXT(" -Log");
	Cmd += FString::Printf(TEXT(" -Port=%d"), ServerPort);

	if (SessionSettings.DSAgentServer.Len() > 0)
	{
		Cmd += FString::Printf(TEXT(" -DSAgentServer=%s"), *SessionSettings.DSAgentServer);
	}

	FProcHandle Handle = FPlatformProcess::CreateProc(*GameServerExePath, *Cmd, false, false, false, nullptr, 0, nullptr, nullptr, nullptr);

	// FPlatformProcess::WaitForProc(Handle);
	if (Handle.IsValid())
	{
		UE_LOG(LogDSMaster, Log, TEXT("CreateGameServerInstance - %s - Done"), *GameServerExePath);
		return true;
	}

	UE_LOG(LogDSMaster, Warning, TEXT("CreateGameServerInstance - %s - Failed"), *GameServerExePath);
	return false;
}

int32 ADSMasterGameMode::AllocateServerPort()
{
	static int32 LastServerPort = ServerPort;
	return LastServerPort++;
}
