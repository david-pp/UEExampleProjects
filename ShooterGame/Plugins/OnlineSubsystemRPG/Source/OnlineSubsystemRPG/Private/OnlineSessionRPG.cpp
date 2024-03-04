// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineSessionRPG.h"

#include "HttpModule.h"
#include "IHttpResponse.h"
#include "Misc/Guid.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemRPG.h"
#include "OnlineSubsystemRPGTypes.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineAsyncTaskManager.h"
#include "SocketSubsystem.h"
#include "NboSerializerRPG.h"

#include "Policies/CondensedJsonPrintPolicy.h"
#include "Serialization/JsonTypes.h"
#include "Serialization/JsonReader.h"
#include "Policies/PrettyJsonPrintPolicy.h"
#include "Serialization/JsonSerializer.h"

FOnlineSessionInfoRPG::FOnlineSessionInfoRPG() :
	HostAddr(NULL),
	SessionId(TEXT("INVALID"))
{
}

void FOnlineSessionInfoRPG::InitLAN(const FOnlineSubsystemRPG& Subsystem)
{
	// Read the IP from the system
	bool bCanBindAll;
	HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLocalHostAddr(*GLog, bCanBindAll);

	// The below is a workaround for systems that set hostname to a distinct address from 127.0.0.1 on a loopback interface.
	// See e.g. https://www.debian.org/doc/manuals/debian-reference/ch05.en.html#_the_hostname_resolution
	// and http://serverfault.com/questions/363095/why-does-my-hostname-appear-with-the-address-127-0-1-1-rather-than-127-0-0-1-in
	// Since we bind to 0.0.0.0, we won't answer on 127.0.1.1, so we need to advertise ourselves as 127.0.0.1 for any other loopback address we may have.
	uint32 HostIp = 0;
	HostAddr->GetIp(HostIp); // will return in host order
	// if this address is on loopback interface, advertise it as 127.0.0.1
	if ((HostIp & 0xff000000) == 0x7f000000)
	{
		HostAddr->SetIp(0x7f000001);	// 127.0.0.1
	}

	// Now set the port that was configured
	HostAddr->SetPort(GetPortFromNetDriver(Subsystem.GetInstanceName()));

	FGuid OwnerGuid;
	FPlatformMisc::CreateGuid(OwnerGuid);
	SessionId = FUniqueNetIdRPG(OwnerGuid.ToString());
}

/**
 *	Async task for ending a RPG online session
 */
class FOnlineAsyncTaskRPGEndSession : public FOnlineAsyncTaskBasic<FOnlineSubsystemRPG>
{
private:
	/** Name of session ending */
	FName SessionName;

public:
	FOnlineAsyncTaskRPGEndSession(class FOnlineSubsystemRPG* InSubsystem, FName InSessionName) :
		FOnlineAsyncTaskBasic(InSubsystem),
		SessionName(InSessionName)
	{
	}

	~FOnlineAsyncTaskRPGEndSession()
	{
	}

	/**
	 *	Get a human readable description of task
	 */
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FOnlineAsyncTaskRPGEndSession bWasSuccessful: %d SessionName: %s"), WasSuccessful(), *SessionName.ToString());
	}

	/**
	 * Give the async task time to do its work
	 * Can only be called on the async task manager thread
	 */
	virtual void Tick() override
	{
		bIsComplete = true;
		bWasSuccessful = true;
	}

	/**
	 * Give the async task a chance to marshal its data back to the game thread
	 * Can only be called on the game thread by the async task manager
	 */
	virtual void Finalize() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
		if (Session)
		{
			Session->SessionState = EOnlineSessionState::Ended;
		}
	}

	/**
	 *	Async task is given a chance to trigger it's delegates
	 */
	virtual void TriggerDelegates() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			SessionInt->TriggerOnEndSessionCompleteDelegates(SessionName, bWasSuccessful);
		}
	}
};

/**
 *	Async task for destroying a RPG online session
 */
class FOnlineAsyncTaskRPGDestroySession : public FOnlineAsyncTaskBasic<FOnlineSubsystemRPG>
{
private:
	/** Name of session ending */
	FName SessionName;

public:
	FOnlineAsyncTaskRPGDestroySession(class FOnlineSubsystemRPG* InSubsystem, FName InSessionName) :
		FOnlineAsyncTaskBasic(InSubsystem),
		SessionName(InSessionName)
	{
	}

	~FOnlineAsyncTaskRPGDestroySession()
	{
	}

	/**
	 *	Get a human readable description of task
	 */
	virtual FString ToString() const override
	{
		return FString::Printf(TEXT("FOnlineAsyncTaskRPGDestroySession bWasSuccessful: %d SessionName: %s"), WasSuccessful(), *SessionName.ToString());
	}

	/**
	 * Give the async task time to do its work
	 * Can only be called on the async task manager thread
	 */
	virtual void Tick() override
	{
		bIsComplete = true;
		bWasSuccessful = true;
	}

	/**
	 * Give the async task a chance to marshal its data back to the game thread
	 * Can only be called on the game thread by the async task manager
	 */
	virtual void Finalize() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			FNamedOnlineSession* Session = SessionInt->GetNamedSession(SessionName);
			if (Session)
			{
				SessionInt->RemoveNamedSession(SessionName);
			}
		}
	}

	/**
	 *	Async task is given a chance to trigger it's delegates
	 */
	virtual void TriggerDelegates() override
	{
		IOnlineSessionPtr SessionInt = Subsystem->GetSessionInterface();
		if (SessionInt.IsValid())
		{
			SessionInt->TriggerOnDestroySessionCompleteDelegates(SessionName, bWasSuccessful);
		}
	}
};

bool FOnlineSessionRPG::CreateSession(int32 HostingPlayerNum, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	uint32 Result = ONLINE_FAIL;

	// Check for an existing session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session == NULL)
	{
		// Create a new session and deep copy the game settings
		Session = AddNamedSession(SessionName, NewSessionSettings);
		check(Session);
		Session->SessionState = EOnlineSessionState::Creating;
		Session->NumOpenPrivateConnections = NewSessionSettings.NumPrivateConnections;
		Session->NumOpenPublicConnections = NewSessionSettings.NumPublicConnections;	// always start with full public connections, local player will register later

		Session->HostingPlayerNum = HostingPlayerNum;

		check(RPGSubsystem);
		IOnlineIdentityPtr Identity = RPGSubsystem->GetIdentityInterface();
		if (Identity.IsValid())
		{
			Session->OwningUserId = Identity->GetUniquePlayerId(HostingPlayerNum);
			Session->OwningUserName = Identity->GetPlayerNickname(HostingPlayerNum);
		}

		// if did not get a valid one, use just something
		if (!Session->OwningUserId.IsValid())
		{
			Session->OwningUserId = FUniqueNetIdRPG::Create(FString::Printf(TEXT("%d"), HostingPlayerNum));
			Session->OwningUserName = FString(TEXT("RPGUser"));
		}
		
		// Unique identifier of this build for compatibility
		Session->SessionSettings.BuildUniqueId = GetBuildUniqueId();

		// Create LAN match
		if (NewSessionSettings.bIsLANMatch)
		{
			// Setup the host session info
			FOnlineSessionInfoRPG* NewSessionInfo = new FOnlineSessionInfoRPG();
			NewSessionInfo->InitLAN(*RPGSubsystem);
			Session->SessionInfo = MakeShareable(NewSessionInfo);

			Result = UpdateLANStatus();
		}
		// Create RPG match
		else
		{
			Result = CreateRPGSession(HostingPlayerNum, Session);
		}

		if (Result != ONLINE_IO_PENDING)
		{
			// Set the game state as pending (not started)
			Session->SessionState = EOnlineSessionState::Pending;

			if (Result != ONLINE_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Cannot create session '%s': session already exists."), *SessionName.ToString());
	}

	if (Result != ONLINE_IO_PENDING)
	{
		TriggerOnCreateSessionCompleteDelegates(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
	}
	
	return Result == ONLINE_IO_PENDING || Result == ONLINE_SUCCESS;
}

bool FOnlineSessionRPG::CreateSession(const FUniqueNetId& HostingPlayerId, FName SessionName, const FOnlineSessionSettings& NewSessionSettings)
{
	// todo: use proper	HostingPlayerId
	return CreateSession(0, SessionName, NewSessionSettings);
}

bool FOnlineSessionRPG::NeedsToAdvertise()
{
	FScopeLock ScopeLock(&SessionLock);

	bool bResult = false;
	for (int32 SessionIdx=0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		FNamedOnlineSession& Session = Sessions[SessionIdx];
		if (NeedsToAdvertise(Session))
		{
			bResult = true;
			break;
		}
	}

	return bResult;
}

bool FOnlineSessionRPG::NeedsToAdvertise( FNamedOnlineSession& Session )
{
	// In RPG, we have to imitate missing online service functionality, so we advertise:
	// a) LAN match with open public connections (same as usually)
	// b) Not started public LAN session (same as usually)
	// d) Joinable presence-enabled session that would be advertised with in an online service
	// (all of that only if we're server)
	return Session.SessionSettings.bShouldAdvertise && IsHost(Session) &&
		(
			(
			  Session.SessionSettings.bIsLANMatch && 			  
			  (Session.SessionState != EOnlineSessionState::InProgress || (Session.SessionSettings.bAllowJoinInProgress && Session.NumOpenPublicConnections > 0))
			) 
			||
			(
				Session.SessionSettings.bAllowJoinViaPresence || Session.SessionSettings.bAllowJoinViaPresenceFriendsOnly
			)
		);		
}

bool FOnlineSessionRPG::IsSessionJoinable(const FNamedOnlineSession& Session) const
{
	const FOnlineSessionSettings& Settings = Session.SessionSettings;

	// LAN beacons are implicitly advertised.
	const bool bIsAdvertised = Settings.bShouldAdvertise || Settings.bIsLANMatch;
	const bool bIsMatchInProgress = Session.SessionState == EOnlineSessionState::InProgress;

	const bool bJoinableFromProgress = (!bIsMatchInProgress || Settings.bAllowJoinInProgress);

	const bool bAreSpacesAvailable = Session.NumOpenPublicConnections > 0;

	// LAN matches don't care about private connections / invites.
	// LAN matches don't care about presence information.
	return bIsAdvertised && bJoinableFromProgress && bAreSpacesAvailable;
}


uint32 FOnlineSessionRPG::UpdateLANStatus()
{
	uint32 Result = ONLINE_SUCCESS;

	if ( NeedsToAdvertise() )
	{
		// set up LAN session
		if (LANSessionManager.GetBeaconState() == ELanBeaconState::NotUsingLanBeacon)
		{
			FOnValidQueryPacketDelegate QueryPacketDelegate = FOnValidQueryPacketDelegate::CreateRaw(this, &FOnlineSessionRPG::OnValidQueryPacketReceived);
			if (!LANSessionManager.Host(QueryPacketDelegate))
			{
				Result = ONLINE_FAIL;

				LANSessionManager.StopLANSession();
			}
		}
	}
	else
	{
		if (LANSessionManager.GetBeaconState() != ELanBeaconState::Searching)
		{
			// Tear down the LAN beacon
			LANSessionManager.StopLANSession();
		}
	}

	return Result;
}

uint32 FOnlineSessionRPG::CreateLANSession(int32 HostingPlayerNum, FNamedOnlineSession* Session)
{
	check(Session);
	uint32 Result = ONLINE_SUCCESS;

	// Setup the host session info
	FOnlineSessionInfoRPG* NewSessionInfo = new FOnlineSessionInfoRPG();
	NewSessionInfo->InitLAN(*RPGSubsystem);
	Session->SessionInfo = MakeShareable(NewSessionInfo);

	Result = UpdateLANStatus();

	// Don't create a the beacon if advertising is off
	// if (Session->SessionSettings.bShouldAdvertise)
	// {
	// 	if (!LANSession.IsValid())
	// 	{
	// 		LANSession = MakeShareable(new FLANSession());
	// 	}
	//
	// 	FOnValidQueryPacketDelegate QueryPacketDelegate = FOnValidQueryPacketDelegate::CreateRaw(this, &FOnlineSessionEOS::OnValidQueryPacketReceived);
	// 	if (!LANSession->Host(QueryPacketDelegate))
	// 	{
	// 		Result = ONLINE_FAIL;
	// 	}
	// }

	return Result;
}

bool FOnlineSessionRPG::StartSession(FName SessionName)
{
	uint32 Result = ONLINE_FAIL;
	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't start a match multiple times
		if (Session->SessionState == EOnlineSessionState::Pending ||
			Session->SessionState == EOnlineSessionState::Ended)
		{
			// If this lan match has join in progress disabled, shut down the beacon
			Result = UpdateLANStatus();
			Session->SessionState = EOnlineSessionState::InProgress;
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning,	TEXT("Can't start an online session (%s) in state %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't start an online game for session (%s) that hasn't been created"), *SessionName.ToString());
	}

	if (Result != ONLINE_IO_PENDING)
	{
		// Just trigger the delegate
		TriggerOnStartSessionCompleteDelegates(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
	}

	return Result == ONLINE_SUCCESS || Result == ONLINE_IO_PENDING;
}

bool FOnlineSessionRPG::UpdateSession(FName SessionName, FOnlineSessionSettings& UpdatedSessionSettings, bool bShouldRefreshOnlineData)
{
	bool bWasSuccessful = true;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// @TODO ONLINE update LAN settings
		Session->SessionSettings = UpdatedSessionSettings;
		TriggerOnUpdateSessionCompleteDelegates(SessionName, bWasSuccessful);
	}

	return bWasSuccessful;
}

bool FOnlineSessionRPG::EndSession(FName SessionName)
{
	uint32 Result = ONLINE_FAIL;

	// Grab the session information by name
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// Can't end a match that isn't in progress
		if (Session->SessionState == EOnlineSessionState::InProgress)
		{
			Session->SessionState = EOnlineSessionState::Ended;

			// If the session should be advertised and the lan beacon was destroyed, recreate
			Result = UpdateLANStatus();
		}
		else
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't end session (%s) in state %s"),
				*SessionName.ToString(),
				EOnlineSessionState::ToString(Session->SessionState));
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't end an online game for session (%s) that hasn't been created"),
			*SessionName.ToString());
	}

	if (Result != ONLINE_IO_PENDING)
	{
		if (Session)
		{
			Session->SessionState = EOnlineSessionState::Ended;
		}

		TriggerOnEndSessionCompleteDelegates(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
	}

	return Result == ONLINE_SUCCESS || Result == ONLINE_IO_PENDING;
}

bool FOnlineSessionRPG::DestroySession(FName SessionName, const FOnDestroySessionCompleteDelegate& CompletionDelegate)
{
	uint32 Result = ONLINE_FAIL;
	// Find the session in question
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		// The session info is no longer needed
		RemoveNamedSession(Session->SessionName);

		Result = UpdateLANStatus();
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't destroy a null online session (%s)"), *SessionName.ToString());
	}

	if (Result != ONLINE_IO_PENDING)
	{
		CompletionDelegate.ExecuteIfBound(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
		TriggerOnDestroySessionCompleteDelegates(SessionName, (Result == ONLINE_SUCCESS) ? true : false);
	}

	return Result == ONLINE_SUCCESS || Result == ONLINE_IO_PENDING;
}

bool FOnlineSessionRPG::IsPlayerInSession(FName SessionName, const FUniqueNetId& UniqueId)
{
	return IsPlayerInSessionImpl(this, SessionName, UniqueId);
}

bool FOnlineSessionRPG::StartMatchmaking(const TArray< FUniqueNetIdRef >& LocalPlayers, FName SessionName, const FOnlineSessionSettings& NewSessionSettings, TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	UE_LOG_ONLINE_SESSION(Warning, TEXT("StartMatchmaking is not supported on this platform. Use FindSessions or FindSessionById."));
	TriggerOnMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionRPG::CancelMatchmaking(int32 SearchingPlayerNum, FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Warning, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionRPG::CancelMatchmaking(const FUniqueNetId& SearchingPlayerId, FName SessionName)
{
	UE_LOG_ONLINE_SESSION(Warning, TEXT("CancelMatchmaking is not supported on this platform. Use CancelFindSessions."));
	TriggerOnCancelMatchmakingCompleteDelegates(SessionName, false);
	return false;
}

bool FOnlineSessionRPG::FindSessions(int32 SearchingPlayerNum, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	uint32 Return = ONLINE_FAIL;

	// Don't start another search while one is in progress
	if (!CurrentSessionSearch.IsValid() && SearchSettings->SearchState != EOnlineAsyncTaskState::InProgress)
	{
		// Free up previous results
		SearchSettings->SearchResults.Empty();

		// Copy the search pointer so we can keep it around
		CurrentSessionSearch = SearchSettings;

		// remember the time at which we started search, as this will be used for a "good enough" ping estimation
		SessionSearchStartInSeconds = FPlatformTime::Seconds();

		// Check if its a LAN query
		Return = FindLANSession();

		if (Return == ONLINE_IO_PENDING)
		{
			SearchSettings->SearchState = EOnlineAsyncTaskState::InProgress;
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Ignoring game search request while one is pending"));
		Return = ONLINE_IO_PENDING;
	}

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}

bool FOnlineSessionRPG::FindSessions(const FUniqueNetId& SearchingPlayerId, const TSharedRef<FOnlineSessionSearch>& SearchSettings)
{
	// This function doesn't use the SearchingPlayerNum parameter, so passing in anything is fine.
	return FindSessions(0, SearchSettings);
}

bool FOnlineSessionRPG::FindSessionById(const FUniqueNetId& SearchingUserId, const FUniqueNetId& SessionId, const FUniqueNetId& FriendId, const FOnSingleSessionResultCompleteDelegate& CompletionDelegates)
{
	FOnlineSessionSearchResult EmptyResult;
	CompletionDelegates.ExecuteIfBound(0, false, EmptyResult);
	return true;
}

uint32 FOnlineSessionRPG::FindLANSession()
{
	uint32 Return = ONLINE_IO_PENDING;

	// Recreate the unique identifier for this client
	GenerateNonce((uint8*)&LANSessionManager.LanNonce, 8);

	FOnValidResponsePacketDelegate ResponseDelegate = FOnValidResponsePacketDelegate::CreateRaw(this, &FOnlineSessionRPG::OnValidResponsePacketReceived);
	FOnSearchingTimeoutDelegate TimeoutDelegate = FOnSearchingTimeoutDelegate::CreateRaw(this, &FOnlineSessionRPG::OnLANSearchTimeout);

	FNboSerializeToBufferRPG Packet(LAN_BEACON_MAX_PACKET_SIZE);
	LANSessionManager.CreateClientQueryPacket(Packet, LANSessionManager.LanNonce);
	if (LANSessionManager.Search(Packet, ResponseDelegate, TimeoutDelegate) == false)
	{
		Return = ONLINE_FAIL;

		FinalizeLANSearch();

		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
		
		// Just trigger the delegate as having failed
		TriggerOnFindSessionsCompleteDelegates(false);
	}
	return Return;
}

bool FOnlineSessionRPG::CancelFindSessions()
{
	uint32 Return = ONLINE_FAIL;
	if (CurrentSessionSearch.IsValid() && CurrentSessionSearch->SearchState == EOnlineAsyncTaskState::InProgress)
	{
		// Make sure it's the right type
		Return = ONLINE_SUCCESS;

		FinalizeLANSearch();

		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Failed;
		CurrentSessionSearch = NULL;
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Can't cancel a search that isn't in progress"));
	}

	if (Return != ONLINE_IO_PENDING)
	{
		TriggerOnCancelFindSessionsCompleteDelegates(true);
	}

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}

bool FOnlineSessionRPG::JoinSession(int32 PlayerNum, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	uint32 Return = ONLINE_FAIL;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	// Don't join a session if already in one or hosting one
	if (Session == NULL)
	{
		// Create a named session from the search result data
		Session = AddNamedSession(SessionName, DesiredSession.Session);
		Session->HostingPlayerNum = PlayerNum;

		// Create Internet or LAN match
		FOnlineSessionInfoRPG* NewSessionInfo = new FOnlineSessionInfoRPG();
		Session->SessionInfo = MakeShareable(NewSessionInfo);

		Return = JoinLANSession(PlayerNum, Session, &DesiredSession.Session);

		// turn off advertising on Join, to avoid clients advertising it over LAN
		Session->SessionSettings.bShouldAdvertise = false;

		if (Return != ONLINE_IO_PENDING)
		{
			if (Return != ONLINE_SUCCESS)
			{
				// Clean up the session info so we don't get into a confused state
				RemoveNamedSession(SessionName);
			}
			else
			{
				RegisterLocalPlayers(Session);
			}
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Session (%s) already exists, can't join twice"), *SessionName.ToString());
	}

	if (Return != ONLINE_IO_PENDING)
	{
		// Just trigger the delegate as having failed
		TriggerOnJoinSessionCompleteDelegates(SessionName, Return == ONLINE_SUCCESS ? EOnJoinSessionCompleteResult::Success : EOnJoinSessionCompleteResult::UnknownError);
	}

	return Return == ONLINE_SUCCESS || Return == ONLINE_IO_PENDING;
}

bool FOnlineSessionRPG::JoinSession(const FUniqueNetId& PlayerId, FName SessionName, const FOnlineSessionSearchResult& DesiredSession)
{
	// Assuming player 0 should be OK here
	return JoinSession(0, SessionName, DesiredSession);
}

bool FOnlineSessionRPG::FindFriendSession(int32 LocalUserNum, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in RPG subsystem
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(LocalUserNum, false, EmptySearchResult);
	return false;
};

bool FOnlineSessionRPG::FindFriendSession(const FUniqueNetId& LocalUserId, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in RPG subsystem
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	return false;
}

bool FOnlineSessionRPG::FindFriendSession(const FUniqueNetId& LocalUserId, const TArray<FUniqueNetIdRef>& FriendList)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in RPG subsystem
	TArray<FOnlineSessionSearchResult> EmptySearchResult;
	TriggerOnFindFriendSessionCompleteDelegates(0, false, EmptySearchResult);
	return false;
}

bool FOnlineSessionRPG::SendSessionInviteToFriend(int32 LocalUserNum, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in RPG subsystem
	return false;
};

bool FOnlineSessionRPG::SendSessionInviteToFriend(const FUniqueNetId& LocalUserId, FName SessionName, const FUniqueNetId& Friend)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in RPG subsystem
	return false;
}

bool FOnlineSessionRPG::SendSessionInviteToFriends(int32 LocalUserNum, FName SessionName, const TArray< FUniqueNetIdRef >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in RPG subsystem
	return false;
};

bool FOnlineSessionRPG::SendSessionInviteToFriends(const FUniqueNetId& LocalUserId, FName SessionName, const TArray< FUniqueNetIdRef >& Friends)
{
	// this function has to exist due to interface definition, but it does not have a meaningful implementation in RPG subsystem
	return false;
}

uint32 FOnlineSessionRPG::JoinLANSession(int32 PlayerNum, FNamedOnlineSession* Session, const FOnlineSession* SearchSession)
{
	check(Session != nullptr);

	uint32 Result = ONLINE_FAIL;
	Session->SessionState = EOnlineSessionState::Pending;

	if (Session->SessionInfo.IsValid() && SearchSession != nullptr && SearchSession->SessionInfo.IsValid())
	{
		// Copy the session info over
		const FOnlineSessionInfoRPG* SearchSessionInfo = (const FOnlineSessionInfoRPG*)SearchSession->SessionInfo.Get();
		FOnlineSessionInfoRPG* SessionInfo = (FOnlineSessionInfoRPG*)Session->SessionInfo.Get();
		SessionInfo->SessionId = SearchSessionInfo->SessionId;

		SessionInfo->HostAddr = SearchSessionInfo->HostAddr->Clone();
		Result = ONLINE_SUCCESS;
	}

	return Result;
}

bool FOnlineSessionRPG::PingSearchResults(const FOnlineSessionSearchResult& SearchResult)
{
	return false;
}

/** Get a resolved connection string from a session info */
static bool GetConnectStringFromSessionInfo(TSharedPtr<FOnlineSessionInfoRPG>& SessionInfo, FString& ConnectInfo, int32 PortOverride=0)
{
	bool bSuccess = false;
	if (SessionInfo.IsValid())
	{
		if (SessionInfo->HostAddr.IsValid() && SessionInfo->HostAddr->IsValid())
		{
			if (PortOverride != 0)
			{
				ConnectInfo = FString::Printf(TEXT("%s:%d"), *SessionInfo->HostAddr->ToString(false), PortOverride);
			}
			else
			{
				ConnectInfo = FString::Printf(TEXT("%s"), *SessionInfo->HostAddr->ToString(true));
			}

			bSuccess = true;
		}
	}

	return bSuccess;
}

bool FOnlineSessionRPG::GetResolvedConnectString(FName SessionName, FString& ConnectInfo, FName PortType)
{
	bool bSuccess = false;
	// Find the session
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session != NULL)
	{
		TSharedPtr<FOnlineSessionInfoRPG> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoRPG>(Session->SessionInfo);
		if (PortType == NAME_BeaconPort)
		{
			int32 BeaconListenPort = GetBeaconPortFromSessionSettings(Session->SessionSettings);
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);
		}
		else if (PortType == NAME_GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}

		if (!bSuccess)
		{
			UE_LOG_ONLINE_SESSION(Warning, TEXT("Invalid session info for session %s in GetResolvedConnectString()"), *SessionName.ToString());
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning,
			TEXT("Unknown session name (%s) specified to GetResolvedConnectString()"),
			*SessionName.ToString());
	}

	return bSuccess;
}

bool FOnlineSessionRPG::GetResolvedConnectString(const FOnlineSessionSearchResult& SearchResult, FName PortType, FString& ConnectInfo)
{
	bool bSuccess = false;
	if (SearchResult.Session.SessionInfo.IsValid())
	{
		TSharedPtr<FOnlineSessionInfoRPG> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoRPG>(SearchResult.Session.SessionInfo);

		if (PortType == NAME_BeaconPort)
		{
			int32 BeaconListenPort = GetBeaconPortFromSessionSettings(SearchResult.Session.SessionSettings);
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo, BeaconListenPort);

		}
		else if (PortType == NAME_GamePort)
		{
			bSuccess = GetConnectStringFromSessionInfo(SessionInfo, ConnectInfo);
		}
	}
	
	if (!bSuccess || ConnectInfo.IsEmpty())
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Invalid session info in search result to GetResolvedConnectString()"));
	}

	return bSuccess;
}

FOnlineSessionSettings* FOnlineSessionRPG::GetSessionSettings(FName SessionName) 
{
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		return &Session->SessionSettings;
	}
	return NULL;
}

void FOnlineSessionRPG::RegisterLocalPlayers(FNamedOnlineSession* Session)
{
	if (!RPGSubsystem->IsDedicated())
	{
		IOnlineVoicePtr VoiceInt = RPGSubsystem->GetVoiceInterface();
		if (VoiceInt.IsValid())
		{
			for (int32 Index = 0; Index < MAX_LOCAL_PLAYERS; Index++)
			{
				// Register the local player as a local talker
				VoiceInt->RegisterLocalTalker(Index);
			}
		}
	}
}

void FOnlineSessionRPG::RegisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineVoicePtr VoiceInt = RPGSubsystem->GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!RPGSubsystem->IsLocalPlayer(PlayerId))
		{
			VoiceInt->RegisterRemoteTalker(PlayerId);
		}
		else
		{
			// This is a local player. In case their PlayerState came last during replication, reprocess muting
			VoiceInt->ProcessMuteChangeNotification();
		}
	}
}

void FOnlineSessionRPG::UnregisterVoice(const FUniqueNetId& PlayerId)
{
	IOnlineVoicePtr VoiceInt = RPGSubsystem->GetVoiceInterface();
	if (VoiceInt.IsValid())
	{
		if (!RPGSubsystem->IsLocalPlayer(PlayerId))
		{
			if (VoiceInt.IsValid())
			{
				VoiceInt->UnregisterRemoteTalker(PlayerId);
			}
		}
	}
}

bool FOnlineSessionRPG::RegisterPlayer(FName SessionName, const FUniqueNetId& PlayerId, bool bWasInvited)
{
	TArray< FUniqueNetIdRef > Players;
	Players.Add(PlayerId.AsShared());
	return RegisterPlayers(SessionName, Players, bWasInvited);
}

bool FOnlineSessionRPG::RegisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players, bool bWasInvited)
{
	bool bSuccess = false;
	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		bSuccess = true;

		for (int32 PlayerIdx=0; PlayerIdx<Players.Num(); PlayerIdx++)
		{
			const FUniqueNetIdRef& PlayerId = Players[PlayerIdx];

			FUniqueNetIdMatcher PlayerMatch(*PlayerId);
			if (Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch) == INDEX_NONE)
			{
				Session->RegisteredPlayers.Add(PlayerId);
				RegisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections > 0)
				{
					Session->NumOpenPublicConnections--;
				}
				else if (Session->NumOpenPrivateConnections > 0)
				{
					Session->NumOpenPrivateConnections--;
				}
			}
			else
			{
				RegisterVoice(*PlayerId);
				UE_LOG_ONLINE_SESSION(Log, TEXT("Player %s already registered in session %s"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}			
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("No game present to join for session (%s)"), *SessionName.ToString());
	}

	TriggerOnRegisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

bool FOnlineSessionRPG::UnregisterPlayer(FName SessionName, const FUniqueNetId& PlayerId)
{
	TArray< FUniqueNetIdRef > Players;
	Players.Add(PlayerId.AsShared());
	return UnregisterPlayers(SessionName, Players);
}

bool FOnlineSessionRPG::UnregisterPlayers(FName SessionName, const TArray< FUniqueNetIdRef >& Players)
{
	bool bSuccess = true;

	FNamedOnlineSession* Session = GetNamedSession(SessionName);
	if (Session)
	{
		for (int32 PlayerIdx=0; PlayerIdx < Players.Num(); PlayerIdx++)
		{
			const FUniqueNetIdRef& PlayerId = Players[PlayerIdx];

			FUniqueNetIdMatcher PlayerMatch(*PlayerId);
			int32 RegistrantIndex = Session->RegisteredPlayers.IndexOfByPredicate(PlayerMatch);
			if (RegistrantIndex != INDEX_NONE)
			{
				Session->RegisteredPlayers.RemoveAtSwap(RegistrantIndex);
				UnregisterVoice(*PlayerId);

				// update number of open connections
				if (Session->NumOpenPublicConnections < Session->SessionSettings.NumPublicConnections)
				{
					Session->NumOpenPublicConnections++;
				}
				else if (Session->NumOpenPrivateConnections < Session->SessionSettings.NumPrivateConnections)
				{
					Session->NumOpenPrivateConnections++;
				}
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("Player %s is not part of session (%s)"), *PlayerId->ToDebugString(), *SessionName.ToString());
			}
		}
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("No game present to leave for session (%s)"), *SessionName.ToString());
		bSuccess = false;
	}

	TriggerOnUnregisterPlayersCompleteDelegates(SessionName, Players, bSuccess);
	return bSuccess;
}

void FOnlineSessionRPG::Tick(float DeltaTime)
{
	SCOPE_CYCLE_COUNTER(STAT_Session_Interface);
	TickLanTasks(DeltaTime);
}

void FOnlineSessionRPG::TickLanTasks(float DeltaTime)
{
	LANSessionManager.Tick(DeltaTime);
}

void FOnlineSessionRPG::AppendSessionToPacket(FNboSerializeToBufferRPG& Packet, FOnlineSession* Session)
{
	/** Owner of the session */
	Packet << *StaticCastSharedPtr<const FUniqueNetIdRPG>(Session->OwningUserId)
		<< Session->OwningUserName
		<< Session->NumOpenPrivateConnections
		<< Session->NumOpenPublicConnections;

	// Try to get the actual port the netdriver is using
	SetPortFromNetDriver(*RPGSubsystem, Session->SessionInfo);

	// Write host info (host addr, session id, and key)
	Packet << *StaticCastSharedPtr<FOnlineSessionInfoRPG>(Session->SessionInfo);

	// Now append per game settings
	AppendSessionSettingsToPacket(Packet, &Session->SessionSettings);
}

void FOnlineSessionRPG::AppendSessionSettingsToPacket(FNboSerializeToBufferRPG& Packet, FOnlineSessionSettings* SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("Sending session settings to client"));
#endif 

	// Members of the session settings class
	Packet << SessionSettings->NumPublicConnections
		<< SessionSettings->NumPrivateConnections
		<< (uint8)SessionSettings->bShouldAdvertise
		<< (uint8)SessionSettings->bIsLANMatch
		<< (uint8)SessionSettings->bIsDedicated
		<< (uint8)SessionSettings->bUsesStats
		<< (uint8)SessionSettings->bAllowJoinInProgress
		<< (uint8)SessionSettings->bAllowInvites
		<< (uint8)SessionSettings->bUsesPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresence
		<< (uint8)SessionSettings->bAllowJoinViaPresenceFriendsOnly
		<< (uint8)SessionSettings->bAntiCheatProtected
	    << SessionSettings->BuildUniqueId;

	// First count number of advertised keys
	int32 NumAdvertisedProperties = 0;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{	
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			NumAdvertisedProperties++;
		}
	}

	// Add count of advertised keys and the data
	Packet << (int32)NumAdvertisedProperties;
	for (FSessionSettings::TConstIterator It(SessionSettings->Settings); It; ++It)
	{
		const FOnlineSessionSetting& Setting = It.Value();
		if (Setting.AdvertisementType >= EOnlineDataAdvertisementType::ViaOnlineService)
		{
			Packet << It.Key();
			Packet << Setting;
#if DEBUG_LAN_BEACON
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("%s"), *Setting.ToString());
#endif
		}
	}
}

void FOnlineSessionRPG::OnValidQueryPacketReceived(uint8* PacketData, int32 PacketLength, uint64 ClientNonce)
{
	// Iterate through all registered sessions and respond for each one that can be joinable
	FScopeLock ScopeLock(&SessionLock);
	for (int32 SessionIndex = 0; SessionIndex < Sessions.Num(); SessionIndex++)
	{
		FNamedOnlineSession* Session = &Sessions[SessionIndex];

		// Don't respond to query if the session is not a joinable LAN match.
		if (Session && IsSessionJoinable(*Session))
		{
			FNboSerializeToBufferRPG Packet(LAN_BEACON_MAX_PACKET_SIZE);
			// Create the basic header before appending additional information
			LANSessionManager.CreateHostResponsePacket(Packet, ClientNonce);

			// Add all the session details
			AppendSessionToPacket(Packet, Session);

			// Broadcast this response so the client can see us
			if (!Packet.HasOverflow())
			{
				LANSessionManager.BroadcastPacket(Packet, Packet.GetByteCount());
			}
			else
			{
				UE_LOG_ONLINE_SESSION(Warning, TEXT("LAN broadcast packet overflow, cannot broadcast on LAN"));
			}
		}
	}
}

void FOnlineSessionRPG::ReadSessionFromPacket(FNboSerializeFromBufferRPG& Packet, FOnlineSession* Session)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("Reading session information from server"));
#endif

	/** Owner of the session */
	FUniqueNetIdRPG* UniqueId = new FUniqueNetIdRPG;
	Packet >> *UniqueId
		>> Session->OwningUserName
		>> Session->NumOpenPrivateConnections
		>> Session->NumOpenPublicConnections;

	Session->OwningUserId = MakeShareable(UniqueId);

	// Allocate and read the connection data
	FOnlineSessionInfoRPG* RPGSessionInfo = new FOnlineSessionInfoRPG();
	RPGSessionInfo->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	Packet >> *RPGSessionInfo;
	Session->SessionInfo = MakeShareable(RPGSessionInfo); 

	// Read any per object data using the server object
	ReadSettingsFromPacket(Packet, Session->SessionSettings);
}

void FOnlineSessionRPG::ReadSettingsFromPacket(FNboSerializeFromBufferRPG& Packet, FOnlineSessionSettings& SessionSettings)
{
#if DEBUG_LAN_BEACON
	UE_LOG_ONLINE_SESSION(Verbose, TEXT("Reading game settings from server"));
#endif

	// Clear out any old settings
	SessionSettings.Settings.Empty();

	// Members of the session settings class
	Packet >> SessionSettings.NumPublicConnections
		>> SessionSettings.NumPrivateConnections;
	uint8 Read = 0;
	// Read all the bools as bytes
	Packet >> Read;
	SessionSettings.bShouldAdvertise = !!Read;
	Packet >> Read;
	SessionSettings.bIsLANMatch = !!Read;
	Packet >> Read;
	SessionSettings.bIsDedicated = !!Read;
	Packet >> Read;
	SessionSettings.bUsesStats = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinInProgress = !!Read;
	Packet >> Read;
	SessionSettings.bAllowInvites = !!Read;
	Packet >> Read;
	SessionSettings.bUsesPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresence = !!Read;
	Packet >> Read;
	SessionSettings.bAllowJoinViaPresenceFriendsOnly = !!Read;
	Packet >> Read;
	SessionSettings.bAntiCheatProtected = !!Read;

	// BuildId
	Packet >> SessionSettings.BuildUniqueId;

	// Now read the contexts and properties from the settings class
	int32 NumAdvertisedProperties = 0;
	// First, read the number of advertised properties involved, so we can presize the array
	Packet >> NumAdvertisedProperties;
	if (Packet.HasOverflow() == false)
	{
		FName Key;
		// Now read each context individually
		for (int32 Index = 0;
			Index < NumAdvertisedProperties && Packet.HasOverflow() == false;
			Index++)
		{
			FOnlineSessionSetting Setting;
			Packet >> Key;
			Packet >> Setting;
			SessionSettings.Set(Key, Setting);

#if DEBUG_LAN_BEACON
			UE_LOG_ONLINE_SESSION(Verbose, TEXT("%s"), *Setting->ToString());
#endif
		}
	}
	
	// If there was an overflow, treat the string settings/properties as broken
	if (Packet.HasOverflow())
	{
		SessionSettings.Settings.Empty();
		UE_LOG_ONLINE_SESSION(Verbose, TEXT("Packet overflow detected in ReadGameSettingsFromPacket()"));
	}
}

void FOnlineSessionRPG::OnValidResponsePacketReceived(uint8* PacketData, int32 PacketLength)
{
	// Create an object that we'll copy the data to
	FOnlineSessionSettings NewServer;
	if (CurrentSessionSearch.IsValid())
	{
		// Add space in the search results array
		FOnlineSessionSearchResult* NewResult = new (CurrentSessionSearch->SearchResults) FOnlineSessionSearchResult();
		// this is not a correct ping, but better than nothing
		NewResult->PingInMs = static_cast<int32>((FPlatformTime::Seconds() - SessionSearchStartInSeconds) * 1000);

		FOnlineSession* NewSession = &NewResult->Session;

		// Prepare to read data from the packet
		FNboSerializeFromBufferRPG Packet(PacketData, PacketLength);
		
		ReadSessionFromPacket(Packet, NewSession);

		// NOTE: we don't notify until the timeout happens
	}
	else
	{
		UE_LOG_ONLINE_SESSION(Warning, TEXT("Failed to create new online game settings object"));
	}
}

uint32 FOnlineSessionRPG::FinalizeLANSearch()
{
	if (LANSessionManager.GetBeaconState() == ELanBeaconState::Searching)
	{
		LANSessionManager.StopLANSession();
	}

	return UpdateLANStatus();
}

void FOnlineSessionRPG::OnLANSearchTimeout()
{
	FinalizeLANSearch();

	if (CurrentSessionSearch.IsValid())
	{
		if (CurrentSessionSearch->SearchResults.Num() > 0)
		{
			// Allow game code to sort the servers
			CurrentSessionSearch->SortSearchResults();
		}
		CurrentSessionSearch->SearchState = EOnlineAsyncTaskState::Done;

		CurrentSessionSearch = NULL;
	}

	// Trigger the delegate as complete
	TriggerOnFindSessionsCompleteDelegates(true);
}

int32 FOnlineSessionRPG::GetNumSessions()
{
	FScopeLock ScopeLock(&SessionLock);
	return Sessions.Num();
}

void FOnlineSessionRPG::DumpSessionState()
{
	FScopeLock ScopeLock(&SessionLock);

	for (int32 SessionIdx=0; SessionIdx < Sessions.Num(); SessionIdx++)
	{
		DumpNamedSession(&Sessions[SessionIdx]);
	}
}

typedef TJsonWriterFactory< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriterFactory;
typedef TJsonWriter< TCHAR, TCondensedJsonPrintPolicy<TCHAR> > FCondensedJsonStringWriter;

typedef TJsonWriterFactory< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriterFactory;
typedef TJsonWriter< TCHAR, TPrettyJsonPrintPolicy<TCHAR> > FPrettyJsonStringWriter;


uint32 FOnlineSessionRPG::CreateRPGSession(int32 HostingPlayerNum, FNamedOnlineSession* Session)
{
	static const FString DiscoveryURL = TEXT("http://127.0.0.1:30000/session/create");

	// Setup Options

	TSharedPtr<FJsonObject> Options = MakeShareable(new FJsonObject());;
	Options->SetStringField(TEXT("SessionName"), Session->SessionName.ToString());
	Options->SetNumberField(TEXT("MaxPlayers"), Session->SessionSettings.NumPrivateConnections + Session->SessionSettings.NumPublicConnections);

	// TODO: more options about a session
	
	FString CreateOptionsString;
	TSharedRef<FCondensedJsonStringWriter> Writer = FCondensedJsonStringWriterFactory::Create(&CreateOptionsString);
	if (FJsonSerializer::Serialize(Options.ToSharedRef(), Writer))
	{
	}

	// Update Local Session Info
	Session->SessionState = EOnlineSessionState::Creating;
	Session->bHosting = true;

	// Setup the host session info
	FOnlineSessionInfoRPG* NewSessionInfo = new FOnlineSessionInfoRPG();
	NewSessionInfo->InitLAN(*RPGSubsystem); // TODO:
	Session->SessionInfo = MakeShareable(NewSessionInfo);


	FName SessionName = Session->SessionName;
	
	auto HttpRequest = FHttpModule::Get().CreateRequest();
	HttpRequest->SetURL(DiscoveryURL);
	HttpRequest->SetVerb(TEXT("POST"));
	HttpRequest->SetHeader("Content-Type", TEXT("application/json"));
	HttpRequest->SetContentAsString(CreateOptionsString);
	HttpRequest->OnProcessRequestComplete().BindLambda([this, SessionName](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		FNamedOnlineSession* Session = GetNamedSession(SessionName);
		if (Session)
		{
			if (bSucceeded && HttpResponse->GetContentType() == "application/json")
			{
				TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
				TSharedRef<TJsonReader<TCHAR>> JsonReader =  TJsonReaderFactory<TCHAR>::Create(HttpResponse->GetContentAsString());
				FJsonSerializer::Deserialize(JsonReader, JsonObject);

				FString SessionId = JsonObject->GetStringField("SessionId");
				FString SessionNameReplied = JsonObject->GetStringField("SessionName");

				UE_LOG(LogOnlineSession, Warning, TEXT("Reply from Server:%s,%s"), *HttpResponse->GetContentAsString(), *SessionNameReplied);

				TSharedPtr<FOnlineSessionInfoRPG> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoRPG>(Session->SessionInfo);
				if (SessionInfo.IsValid())
				{
					SessionInfo->SessionId = FUniqueNetIdRPG(SessionId);
				}

				Session->SessionState = EOnlineSessionState::Pending;

				RegisterLocalPlayers(Session);
			}
			else
			{
				// Handle error here
				UE_LOG_ONLINE_SESSION(Error, TEXT("Create Session Failed"));

				Session->SessionState = EOnlineSessionState::NoSession;
				RemoveNamedSession(SessionName);
			}

			TriggerOnCreateSessionCompleteDelegates(SessionName, bSucceeded);
		}
	});

	HttpRequest->ProcessRequest();
	return ONLINE_IO_PENDING;
}

void FOnlineSessionRPG::RegisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnRegisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, EOnJoinSessionCompleteResult::Success);
}

void FOnlineSessionRPG::UnregisterLocalPlayer(const FUniqueNetId& PlayerId, FName SessionName, const FOnUnregisterLocalPlayerCompleteDelegate& Delegate)
{
	Delegate.ExecuteIfBound(PlayerId, true);
}

void FOnlineSessionRPG::SetPortFromNetDriver(const FOnlineSubsystemRPG& Subsystem, const TSharedPtr<FOnlineSessionInfo>& SessionInfo)
{
	auto NetDriverPort = GetPortFromNetDriver(Subsystem.GetInstanceName());
	auto SessionInfoRPG = StaticCastSharedPtr<FOnlineSessionInfoRPG>(SessionInfo);
	if (SessionInfoRPG.IsValid() && SessionInfoRPG->HostAddr.IsValid())
	{
		SessionInfoRPG->HostAddr->SetPort(NetDriverPort);
	}
}

bool FOnlineSessionRPG::IsHost(const FNamedOnlineSession& Session) const
{
	if (RPGSubsystem->IsDedicated())
	{
		return true;
	}

	IOnlineIdentityPtr IdentityInt = RPGSubsystem->GetIdentityInterface(); 
	if (!IdentityInt.IsValid())
	{
		return false;
	}

	FUniqueNetIdPtr UserId = IdentityInt->GetUniquePlayerId(Session.HostingPlayerNum);
	return (UserId.IsValid() && (*UserId == *Session.OwningUserId));
}

FUniqueNetIdPtr FOnlineSessionRPG::CreateSessionIdFromString(const FString& SessionIdStr)
{
	FUniqueNetIdPtr SessionId;
	if (!SessionIdStr.IsEmpty())
	{
		SessionId = FUniqueNetIdRPG::Create(SessionIdStr);
	}
	return SessionId;
}
