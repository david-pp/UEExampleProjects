// Fill out your copyright notice in the Description page of Project Settings.


#include "DSMaster/DSMasterGameMode.h"

#include "DSMasterBeaconClient.h"
#include "HttpModule.h"
#include "HttpServerModule.h"
#include "HttpServerResponse.h"
#include "IHttpResponse.h"
#include "JsonObjectConverter.h"
#include "JsonObjectConverter.h"
#include "OnlineSubsystemRPGTypes.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Session/GameSessionTypes.h"

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
		OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &ADSMasterGameMode::OnFindSessionsComplete);
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

	HttpSessionService.StopHttpServer();
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
	HttpSessionService.Init(HttpServerPort);
	// RegisterRoutes();
	// StartHttpServer();
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

void ADSMasterGameMode::DebugFindSession()
{
	
	// PlayerOwner->GetPreferredUniqueNetId().GetUniqueNetId()
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		CurrentSessionParams.SessionName = NAME_GameSession;
		CurrentSessionParams.bIsLAN = true;
		// CurrentSessionParams.bIsPresence = false;
		CurrentSessionParams.UserId = MakeShared<FUniqueNetIdString>();

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			SearchSettings = MakeShareable(new FOnlineSessionSearch());
			SearchSettings->bIsLanQuery = false;
			SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("SomeThing")), EOnlineComparisonOp::Equals);
			SearchSettings->QuerySettings.Set(FName(TEXT("TESTSETTING1")), (int32)5, EOnlineComparisonOp::Equals);

			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();

			OnFindSessionCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
			Sessions->FindSessions(*CurrentSessionParams.UserId, SearchSettingsRef);
		}
	}
}

void ADSMasterGameMode::DebugSessionProtocol()
{
	// FActiveRPGGameSession RPGGameSession;
	// RPGGameSession.SetupByOnlineSettings(*ShooterHostSettings);
	//
	// FString JsonObjectString;
	// if (FJsonObjectConverter::UStructToJsonObjectString(RPGGameSession, JsonObjectString))
	// {
	// 	UE_LOG(LogDSMaster, Warning, TEXT("\n%s"), *JsonObjectString);
	// }
	//
	// FActiveRPGGameSession RPGGameSession2;
	// if (FJsonObjectConverter::JsonObjectStringToUStruct(JsonObjectString, &RPGGameSession2))
	// {
	// 	TSharedPtr<FOnlineSessionSettings> Settings = RPGGameSession2.CreateOnlineSessionSettings();
	// 	if (Settings)
	// 	{
	// 		DumpSessionSettings(Settings.Get());
	// 	}
	// }
	//
}

void ADSMasterGameMode::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogDSMaster, Warning,TEXT("OnCreateSessionComplete --  %s"), *SessionName.ToString());
}

void ADSMasterGameMode::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogDSMaster, Warning,TEXT("OnFindSessionsComplete"))

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionCompleteDelegateHandle);

			UE_LOG(LogDSMaster, Warning, TEXT("Num Search Results: %d"), SearchSettings->SearchResults.Num());
			for (int32 SearchIdx=0; SearchIdx < SearchSettings->SearchResults.Num(); SearchIdx++)
			{
				const FOnlineSessionSearchResult& SearchResult = SearchSettings->SearchResults[SearchIdx];
				DumpSession(&SearchResult.Session);

				if (SearchResult.IsSessionInfoValid())
				{
					auto SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoRPG>(SearchResult.Session.SessionInfo);
					if (SessionInfo)
					{
						UE_LOG(LogDSMaster, Warning, TEXT("HostAddres: %s"), *SessionInfo->HostAddr->ToString(true));
					}
				}
			}
		}
	}
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
