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
#include "GameSessionTypes.h"
#include "OnlineAsyncTaskManager.h"
#include "OnlineBeaconHost.h"
#include "OnlineSessionAsyncTasks.h"
#include "OnlineSessionRPG.h"
#include "OnlineSubsystemRPG.h"

ADSMasterGameMode::ADSMasterGameMode(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// PlayerControllerClass = AShooterPlayerController_Menu::StaticClass();
	// DSMasterBeaconHostObjectClass = ADSMasterBeaconHost::StaticClass();
	// DSBeaconHostObjectClass = ADSBeaconHost::StaticClass();
	//
	// DSMasterBeaconClientClass = ADSMasterBeaconClient::StaticClass();
	//
	// DSMaterServerAddress = TEXT("127.0.0.1:15000");


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

	DSMasterService.StopServer();
}

void ADSMasterGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);
}

// Called every frame
void ADSMasterGameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ADSMasterGameMode::StartPlay()
{
	Super::StartPlay();

	FString ErrorMessage;
	DSMasterService.InitServer(true);
	DSMasterService.StartDSMasterHttpService();
	DSMasterService.StartDSMasterServer(GetWorld());
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


#define SEARCH_FINDORCREATE FName(TEXT("SEARCH_FINDORCREATE"))


/**
 * General search setting for a Shooter game
 */
class FRPGOnlineSessionSearch : public FOnlineSessionSearch
{
public:
	FRPGOnlineSessionSearch(bool bSearchingLAN = false, bool bSearchingPresence = false)
	{
	
		bIsLanQuery = bSearchingLAN;
		MaxSearchResults = 10;
		PingBucketSize = 50;

		if (bSearchingPresence)
		{
			QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
			QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
		}
	}

	virtual ~FRPGOnlineSessionSearch() {}
};

void ADSMasterGameMode::DebugRequestOneGameSession()
{
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
			SearchSettings->MaxSearchResults = 1;
			SearchSettings->bIsLanQuery = false;
			
			SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("SomeThing")), EOnlineComparisonOp::Equals);
			SearchSettings->QuerySettings.Set(FName(TEXT("TESTSETTING1")), (int32)5, EOnlineComparisonOp::Equals);
			// SearchSettings->QuerySettings.Set(FName(TEXT("game")), FString(TEXT("ShooterGameMode")), EOnlineComparisonOp::Equals);

			SearchSettings->QuerySettings.Set(FName(TEXT("map")), FString(TEXT("Sanctuary")), EOnlineComparisonOp::Equals);
			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();

			OnFindSessionCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
			// Sessions->FindSessions(*CurrentSessionParams.UserId, SearchSettingsRef);
			FOnlineSubsystemRPG* OnlineSubsystemRPG = static_cast<FOnlineSubsystemRPG*>(OnlineSub);
			if (OnlineSubsystemRPG)
			{
				FOnlineAsyncTaskRequestRPGGameSession* NewTask = new FOnlineAsyncTaskRequestRPGGameSession(OnlineSubsystemRPG, SearchSettings);
				OnlineSubsystemRPG->QueueAsyncTask(NewTask);
			}
		}
	}
}

void ADSMasterGameMode::DebugCreateServiceConfigFile()
{
	DSMasterService.SaveServiceSettingToJsonFile(TEXT("DSMasterService-Saved.json"));
}

void ADSMasterGameMode::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogDSMaster, Warning,TEXT("OnCreateSessionComplete --  %s"), *SessionName.ToString());
}

void ADSMasterGameMode::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogDSMaster, Warning,TEXT("OnFindSessionsComplete - %d"), bWasSuccessful);

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

bool ADSMasterGameMode::DebugCreateGameServerInstance(FDSMasterGameSessionSettings SessionSettings)
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

	FGameServerLaunchSettings LaunchSettings;
	LaunchSettings.URL = GameServerExePath;
	LaunchSettings.Params = Cmd;
	GameServerManager.LaunchGameServer(LaunchSettings);

	FTimerHandle DummyHandle2;
	GetWorldTimerManager().SetTimer(DummyHandle2, FTimerDelegate::CreateLambda([this]()
	{
		GameServerManager.CheckAndUpdateGameServers();
	}), 1, true);

	return true;

	if (SessionSettings.DebugExe.Len() > 0)
	{
		// GameServerProcess = MakeShared<FInteractiveProcess>(*SessionSettings.DebugExe, TEXT(""), false, true);
		GameServerProcess = FGameServerProcess::CreateGameServer(*SessionSettings.DebugExe, TEXT(""), false, true);
	}
	else
	{
		// GameServerProcess = MakeShared<FInteractiveProcess>(*GameServerExePath, *Cmd, false, true);
		GameServerProcess = FGameServerProcess::CreateGameServer(*GameServerExePath, *Cmd, false, false, false);
	}

	GameServerProcess->OnOutput().BindLambda([](const FString& Output)
	{
		UE_LOG(LogDSMaster, Log, TEXT("> %s"), *Output);
	});

	GameServerProcess->OnCanceled().BindLambda([]()
	{
		UE_LOG(LogDSMaster, Warning, TEXT("OnCanceled"));
	});

	GameServerProcess->OnCompleted().BindLambda([](int32 ReturnCode)
	{
		UE_LOG(LogDSMaster, Warning, TEXT("OnCompleted : %d"), ReturnCode);
	});

	FTimerHandle DummyHandle;
	GetWorldTimerManager().SetTimer(DummyHandle, FTimerDelegate::CreateLambda([this]()
	{
		if (GameServerProcess)
		{
			if (GameServerProcess->Update())
			{
				UE_LOG(LogDSMaster, Log, TEXT("Server is Running : %s - %s"), *GameServerProcess->GetProcessInfo().Name, *GameServerProcess->GetProcessInfo().UUID.ToString());
			}
			else
			{
				UE_LOG(LogDSMaster, Warning, TEXT("Server is Over : %s - %s"), *GameServerProcess->GetProcessInfo().Name, *GameServerProcess->GetProcessInfo().UUID.ToString());
			}
		}
	}), 1, true);

	GameServerProcess->Launch();
	return true;
}

bool ADSMasterGameMode::DebugCancelGameServerProcess()
{
	GameServerManager.StopAllGameServers();
	
	// if (GameServerProcess)
	// {
	// 	GameServerProcess->Cancel(true);
	// 	GameServerProcess = nullptr;
	// }
	return true;
}

bool ADSMasterGameMode::DebugCreateGameServerInstanceWithNamedPipe(FDSMasterGameSessionSettings SessionSettings)
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

	// -------------------------
	
	FString PipeName = TEXT("DSMaster");

	// Create the output pipe as a server...
	if (!OutputNamedPipe.Create(FString::Printf(TEXT("\\\\.\\pipe\\%s-A"), *PipeName), true, false))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create the output named pipe."));
		return false;
	}

	// Wait for to connect to the output pipe
	if (!OutputNamedPipe.OpenConnection())
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to open a connection on the output named pipe."));
		return false;
	}

	
	return true;
}

int32 ADSMasterGameMode::AllocateServerPort()
{
	static int32 LastServerPort = 7000;
	return LastServerPort++;
}
