// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineRPGGameSession.h"
#include "DSMasterBeaconClient.h"
#include "OnlineRPGOnlineGameSettings.h"
#include "OnlineSubsystemSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Kismet/GameplayStatics.h"

namespace
{
	const FString CustomMatchKeyword("Custom");
}

AOnlineRPGGameSession::AOnlineRPGGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		OnCreateSessionCompleteDelegate = FOnCreateSessionCompleteDelegate::CreateUObject(this, &AOnlineRPGGameSession::OnCreateSessionComplete);
		OnDestroySessionCompleteDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &AOnlineRPGGameSession::OnDestroySessionComplete);

		OnFindSessionsCompleteDelegate = FOnFindSessionsCompleteDelegate::CreateUObject(this, &AOnlineRPGGameSession::OnFindSessionsComplete);
		OnJoinSessionCompleteDelegate = FOnJoinSessionCompleteDelegate::CreateUObject(this, &AOnlineRPGGameSession::OnJoinSessionComplete);

		OnStartSessionCompleteDelegate = FOnStartSessionCompleteDelegate::CreateUObject(this, &AOnlineRPGGameSession::OnStartOnlineGameComplete);
	}
}

/**
 * Delegate fired when a session start request has completed
 *
 * @param SessionName the name of the session this callback is for
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
void AOnlineRPGGameSession::OnStartOnlineGameComplete(FName InSessionName, bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegateHandle);
		}
	}

	if (bWasSuccessful)
	{
		// tell non-local players to start online game
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			// AOnlineRPGPlayerController* PC = Cast<AOnlineRPGPlayerController>(*It);
			// if (PC && !PC->IsLocalPlayerController())
			// {
			// 	PC->ClientStartOnlineGame();
			// }
		}
	}
}

/** Handle starting the match */
void AOnlineRPGGameSession::HandleMatchHasStarted()
{
	// start online game locally and wait for completion
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && (Sessions->GetNamedSession(NAME_GameSession) != nullptr))
		{
			UE_LOG(LogOnlineGame, Log, TEXT("Starting session %s on server"), *FName(NAME_GameSession).ToString());
			OnStartSessionCompleteDelegateHandle = Sessions->AddOnStartSessionCompleteDelegate_Handle(OnStartSessionCompleteDelegate);
			Sessions->StartSession(NAME_GameSession);
		}
	}
}

/** 
 * Ends a game session 
 *
 */
void AOnlineRPGGameSession::HandleMatchHasEnded()
{
	// end online game locally 
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && (Sessions->GetNamedSession(NAME_GameSession) != nullptr))
		{
			// tell the clients to end
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				// AOnlineRPGPlayerController* PC = Cast<AOnlineRPGPlayerController>(*It);
				// if (PC && !PC->IsLocalPlayerController())
				// {
				// 	PC->ClientEndOnlineGame();
				// }
			}

			// server is handled here
			UE_LOG(LogOnlineGame, Log, TEXT("Ending session %s on server"), *FName(NAME_GameSession).ToString() );
			Sessions->EndSession(NAME_GameSession);
		}
	}
}

bool AOnlineRPGGameSession::IsBusy() const
{ 
	if (HostSettings.IsValid() || SearchSettings.IsValid())
	{
		return true;
	}

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			EOnlineSessionState::Type GameSessionState = Sessions->GetSessionState(NAME_GameSession);
			EOnlineSessionState::Type PartySessionState = Sessions->GetSessionState(NAME_PartySession);
			if (GameSessionState != EOnlineSessionState::NoSession || PartySessionState != EOnlineSessionState::NoSession)
			{
				return true;
			}
		}
	}

	return false;
}

EOnlineAsyncTaskState::Type AOnlineRPGGameSession::GetSearchResultStatus(int32& SearchResultIdx, int32& NumSearchResults)
{
	SearchResultIdx = 0;
	NumSearchResults = 0;

	if (SearchSettings.IsValid())
	{
		if (SearchSettings->SearchState == EOnlineAsyncTaskState::Done)
		{
			SearchResultIdx = CurrentSessionParams.BestSessionIdx;
			NumSearchResults = SearchSettings->SearchResults.Num();
		}
		return SearchSettings->SearchState;
	}

	return EOnlineAsyncTaskState::NotStarted;
}

/**
 * Get the search results.
 *
 * @return Search results
 */
const TArray<FOnlineSessionSearchResult> & AOnlineRPGGameSession::GetSearchResults() const
{ 
	return SearchSettings->SearchResults; 
};


/**
 * Delegate fired when a session create request has completed
 *
 * @param SessionName the name of the session this callback is for
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
void AOnlineRPGGameSession::OnCreateSessionComplete(FName InSessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("OnCreateSessionComplete %s bSuccess: %d"), *InSessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegateHandle);
	}

	OnCreatePresenceSessionComplete().Broadcast(InSessionName, bWasSuccessful);	
}

/**
 * Delegate fired when a destroying an online session has completed
 *
 * @param SessionName the name of the session this callback is for
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
void AOnlineRPGGameSession::OnDestroySessionComplete(FName InSessionName, bool bWasSuccessful)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("OnDestroySessionComplete %s bSuccess: %d"), *InSessionName.ToString(), bWasSuccessful);

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(OnDestroySessionCompleteDelegateHandle);
		HostSettings = NULL;
	}
}

bool AOnlineRPGGameSession::HostSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, const FString& GameType, const FString& MapName, bool bIsLAN, bool bIsPresence, int32 MaxNumPlayers)
{
	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		CurrentSessionParams.SessionName = InSessionName;
		CurrentSessionParams.bIsLAN = bIsLAN;
		CurrentSessionParams.bIsPresence = bIsPresence;
		CurrentSessionParams.UserId = UserId;
		MaxPlayers = MaxNumPlayers;

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
		{
			HostSettings = MakeShareable(new FOnlineRPGOnlineSessionSettings(bIsLAN, bIsPresence, MaxPlayers));
			HostSettings->bUseLobbiesIfAvailable = true;
			HostSettings->bUseLobbiesVoiceChatIfAvailable = true;
			HostSettings->Set(SETTING_GAMEMODE, GameType, EOnlineDataAdvertisementType::ViaOnlineService);
			HostSettings->Set(SETTING_MAPNAME, MapName, EOnlineDataAdvertisementType::ViaOnlineService);
			HostSettings->Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
			HostSettings->Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
			HostSettings->Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineDataAdvertisementType::DontAdvertise);
			if (UserId->IsValid())
			{
				FSessionSettings & UserSettings =  HostSettings->MemberSettings.Add(UserId.ToSharedRef(), FSessionSettings());			
				UserSettings.Add(SETTING_GAMEMODE, FOnlineSessionSetting(FString("GameSession"), EOnlineDataAdvertisementType::ViaOnlineService));
			}

			// On Switch, we don't have room for this in the LAN session data (and it's not used anyway when searching), so there's no need to add it.
			// Can be readded if the buffer size increases.
			if (!(PLATFORM_SWITCH && bIsLAN))
			{
				HostSettings->Set(SEARCH_KEYWORDS, CustomMatchKeyword, EOnlineDataAdvertisementType::ViaOnlineService);
			}

			OnCreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
			return Sessions->CreateSession(*CurrentSessionParams.UserId, CurrentSessionParams.SessionName, *HostSettings);
		}
		else
		{
			OnCreateSessionComplete(InSessionName, false);
		}
	}
#if !UE_BUILD_SHIPPING
	else 
	{
		// Hack workflow in development
		OnCreatePresenceSessionComplete().Broadcast(NAME_GameSession, true);
		return true;
	}
#endif

	return false;
}

bool AOnlineRPGGameSession::HostSession(const TSharedPtr<const FUniqueNetId> UserId, const FName InSessionName, const FOnlineSessionSettings& SessionSettings)
{
	bool bResult = false;

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		CurrentSessionParams.SessionName = InSessionName;
		CurrentSessionParams.bIsLAN = SessionSettings.bIsLANMatch;
		CurrentSessionParams.bIsPresence = SessionSettings.bUsesPresence;
		CurrentSessionParams.UserId = UserId;
		MaxPlayers = SessionSettings.NumPrivateConnections + SessionSettings.NumPublicConnections;

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
		{
			OnCreateSessionCompleteDelegateHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
			bResult = Sessions->CreateSession(*UserId, InSessionName, SessionSettings);
		}
		else
		{
			OnCreateSessionComplete(InSessionName, false);
		}
	}

	return bResult;
}

void AOnlineRPGGameSession::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("OnFindSessionsComplete bSuccess: %d"), bWasSuccessful);

	IOnlineSubsystem* const OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegateHandle);

			UE_LOG(LogOnlineGame, Verbose, TEXT("Num Search Results: %d"), SearchSettings->SearchResults.Num());
			for (int32 SearchIdx=0; SearchIdx < SearchSettings->SearchResults.Num(); SearchIdx++)
			{
				const FOnlineSessionSearchResult& SearchResult = SearchSettings->SearchResults[SearchIdx];
				DumpSession(&SearchResult.Session);
			}

			OnFindSessionsComplete().Broadcast(bWasSuccessful);
		}
	}
}

void AOnlineRPGGameSession::ResetBestSessionVars()
{
	CurrentSessionParams.BestSessionIdx = -1;
}

void AOnlineRPGGameSession::ChooseBestSession()
{
	// Start searching from where we left off
	for (int32 SessionIndex = CurrentSessionParams.BestSessionIdx+1; SessionIndex < SearchSettings->SearchResults.Num(); SessionIndex++)
	{
		// Found the match that we want
		CurrentSessionParams.BestSessionIdx = SessionIndex;
		return;
	}

	CurrentSessionParams.BestSessionIdx = -1;
}

void AOnlineRPGGameSession::StartMatchmaking()
{
	ResetBestSessionVars();
	ContinueMatchmaking();
}

void AOnlineRPGGameSession::ContinueMatchmaking()
{	
	ChooseBestSession();
	if (CurrentSessionParams.BestSessionIdx >= 0 && CurrentSessionParams.BestSessionIdx < SearchSettings->SearchResults.Num())
	{
		IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
		if (OnlineSub)
		{
			IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
			if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
			{
				OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
				Sessions->JoinSession(*CurrentSessionParams.UserId, CurrentSessionParams.SessionName, SearchSettings->SearchResults[CurrentSessionParams.BestSessionIdx]);
			}
		}
	}
	else
	{
		OnNoMatchesAvailable();
	}
}

void AOnlineRPGGameSession::OnNoMatchesAvailable()
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("Matchmaking complete, no sessions available."));
	SearchSettings = NULL;
}

void AOnlineRPGGameSession::FindSessions(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, bool bIsLAN, bool bIsPresence)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		CurrentSessionParams.SessionName = InSessionName;
		CurrentSessionParams.bIsLAN = bIsLAN;
		CurrentSessionParams.bIsPresence = bIsPresence;
		CurrentSessionParams.UserId = UserId;

		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && CurrentSessionParams.UserId.IsValid())
		{
			SearchSettings = MakeShareable(new FOnlineRPGOnlineSearchSettings(bIsLAN, bIsPresence));
			SearchSettings->QuerySettings.Set(SEARCH_KEYWORDS, CustomMatchKeyword, EOnlineComparisonOp::Equals);

			TSharedRef<FOnlineSessionSearch> SearchSettingsRef = SearchSettings.ToSharedRef();

			OnFindSessionsCompleteDelegateHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(OnFindSessionsCompleteDelegate);
			Sessions->FindSessions(*CurrentSessionParams.UserId, SearchSettingsRef);
		}
	}
	else
	{
		OnFindSessionsComplete(false);
	}
}

bool AOnlineRPGGameSession::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, int32 SessionIndexInSearchResults)
{
	bool bResult = false;

	if (SessionIndexInSearchResults >= 0 && SessionIndexInSearchResults < SearchSettings->SearchResults.Num())
	{
		bResult = JoinSession(UserId, InSessionName, SearchSettings->SearchResults[SessionIndexInSearchResults]);
	}

	return bResult;
}

bool AOnlineRPGGameSession::JoinSession(TSharedPtr<const FUniqueNetId> UserId, FName InSessionName, const FOnlineSessionSearchResult& SearchResult)
{
	bool bResult = false;

	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && UserId.IsValid())
		{
			OnJoinSessionCompleteDelegateHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegate);
			bResult = Sessions->JoinSession(*UserId, InSessionName, SearchResult);
		}
	}

	return bResult;
}

/**
 * Delegate fired when the joining process for an online session has completed
 *
 * @param SessionName the name of the session this callback is for
 * @param bWasSuccessful true if the async action completed without error, false if there was an error
 */
void AOnlineRPGGameSession::OnJoinSessionComplete(FName InSessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bWillTravel = false;

	UE_LOG(LogOnlineGame, Verbose, TEXT("OnJoinSessionComplete %s bSuccess: %d"), *InSessionName.ToString(), static_cast<int32>(Result));
	
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid())
		{
			Sessions->ClearOnJoinSessionCompleteDelegate_Handle(OnJoinSessionCompleteDelegateHandle);
		}
	}

	OnJoinSessionComplete().Broadcast(Result);
}

bool AOnlineRPGGameSession::TravelToSession(int32 ControllerId, FName InSessionName)
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		FString URL;
		IOnlineSessionPtr Sessions = OnlineSub->GetSessionInterface();
		if (Sessions.IsValid() && Sessions->GetResolvedConnectString(InSessionName, URL))
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), ControllerId);
			if (PC)
			{
				PC->ClientTravel(URL, TRAVEL_Absolute);
				return true;
			}
		}
		else
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("Failed to join session %s"), *SessionName.ToString());
		}
	}
#if !UE_BUILD_SHIPPING
	else
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), ControllerId);
		if (PC)
		{
			FString LocalURL(TEXT("127.0.0.1"));
			PC->ClientTravel(LocalURL, TRAVEL_Absolute);
			return true;
		}
	}
#endif //!UE_BUILD_SHIPPING

	return false;
}


void AOnlineRPGGameSession::RegisterServer()
{
	IOnlineSubsystem* OnlineSub = Online::GetSubsystem(GetWorld());
	if (OnlineSub)
	{
		IOnlineSessionPtr SessionInt = OnlineSub->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			TSharedPtr<class FOnlineRPGOnlineSessionSettings> OnlineRPGHostSettings = MakeShareable(new FOnlineRPGOnlineSessionSettings(false, false, 16));
			OnlineRPGHostSettings->Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
			OnlineRPGHostSettings->Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
			OnlineRPGHostSettings->Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineDataAdvertisementType::DontAdvertise);
			OnlineRPGHostSettings->Set(SETTING_GAMEMODE, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::ViaOnlineService);
			OnlineRPGHostSettings->Set(SETTING_MAPNAME, GetWorld()->GetMapName(), EOnlineDataAdvertisementType::ViaOnlineService);
			OnlineRPGHostSettings->bAllowInvites = true;
			OnlineRPGHostSettings->bIsDedicated = true;
			if (FParse::Param(FCommandLine::Get(), TEXT("forcelan")))
			{
				UE_LOG(LogOnlineGame, Log, TEXT("Registering server as a LAN server"));
				OnlineRPGHostSettings->bIsLANMatch = true;
			}
			HostSettings = OnlineRPGHostSettings;
			OnCreateSessionCompleteDelegateHandle = SessionInt->AddOnCreateSessionCompleteDelegate_Handle(OnCreateSessionCompleteDelegate);
			SessionInt->CreateSession(0, NAME_GameSession, *HostSettings);
		}
	}

	// Connect to DS AgentServer
	FString DSAgentServerAddress;
	if(FParse::Value(FCommandLine::Get(), TEXT("MasterServer="), DSAgentServerAddress))
	{
		ConnectToDSMasterServer(DSAgentServerAddress);
	}
}

bool AOnlineRPGGameSession::ConnectToDSMasterServer(FString ServerAddress)
{
	DSMasterClient = GetWorld()->SpawnActor<ADSMasterBeaconClient>(ADSMasterBeaconClient::StaticClass());
	if (DSMasterClient)
	{
		DSMasterClient->Settings.ClientType = EDSMasterClientType::DedicatedServer;
		return DSMasterClient->ConnectToMasterServer(ServerAddress);
	}
	
	return false;
}
