// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DSMasterServiceClient.h"
#include "OnlineSubsystemImpl.h"
#include "OnlineSubsystemRPGPackage.h"
#include "HAL/ThreadSafeCounter.h"

class FOnlineSessionRPG;
class FOnlineIdentityRPG;

/** Forward declarations of all interface classes */
typedef TSharedPtr<class FOnlineSessionRPG, ESPMode::ThreadSafe> FOnlineSessionRPGPtr;
typedef TSharedPtr<class FOnlineIdentityRPG, ESPMode::ThreadSafe> FOnlineIdentityRPGPtr;


/**
 *	OnlineSubsystemRPG - Implementation of the online subsystem for RPG services
 */
class ONLINESUBSYSTEMRPG_API FOnlineSubsystemRPG : public FOnlineSubsystemImpl
{
public:
	virtual ~FOnlineSubsystemRPG() = default;

	// IOnlineSubsystem

	virtual IOnlineSessionPtr GetSessionInterface() const override;
	virtual IOnlineFriendsPtr GetFriendsInterface() const override;
	virtual IOnlinePartyPtr GetPartyInterface() const override;
	virtual IOnlineGroupsPtr GetGroupsInterface() const override;
	virtual IOnlineSharedCloudPtr GetSharedCloudInterface() const override;
	virtual IOnlineUserCloudPtr GetUserCloudInterface() const override;
	virtual IOnlineEntitlementsPtr GetEntitlementsInterface() const override;
	virtual IOnlineLeaderboardsPtr GetLeaderboardsInterface() const override;
	virtual IOnlineVoicePtr GetVoiceInterface() const override;
	virtual IOnlineExternalUIPtr GetExternalUIInterface() const override;
	virtual IOnlineTimePtr GetTimeInterface() const override;
	virtual IOnlineIdentityPtr GetIdentityInterface() const override;
	virtual IOnlineTitleFilePtr GetTitleFileInterface() const override;
	virtual IOnlineStoreV2Ptr GetStoreV2Interface() const override;
	virtual IOnlinePurchasePtr GetPurchaseInterface() const override;
	virtual IOnlineEventsPtr GetEventsInterface() const override;
	virtual IOnlineAchievementsPtr GetAchievementsInterface() const override;
	virtual IOnlineSharingPtr GetSharingInterface() const override;
	virtual IOnlineUserPtr GetUserInterface() const override;
	virtual IOnlineMessagePtr GetMessageInterface() const override;
	virtual IOnlinePresencePtr GetPresenceInterface() const override;
	virtual IOnlineChatPtr GetChatInterface() const override;
	virtual IOnlineStatsPtr GetStatsInterface() const override;
	virtual IOnlineTurnBasedPtr GetTurnBasedInterface() const override;
	virtual IOnlineTournamentPtr GetTournamentInterface() const override;
	virtual IMessageSanitizerPtr GetMessageSanitizer(int32 LocalUserNum, FString& OutAuthTypeToExclude) const override;

	virtual bool Init() override;
	virtual bool Shutdown() override;
	virtual FString GetAppId() const override;
	virtual bool Exec(class UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar) override;
	virtual FText GetOnlineServiceName() const override;

	// FTickerObjectBase

	virtual bool Tick(float DeltaTime) override;

	// FOnlineSubsystemRPG


	/**
	 *	Add an async task onto the task queue for processing
	 * @param AsyncTask - new heap allocated task to process on the async task thread
	 */
	void QueueAsyncTask(class FOnlineAsyncTask* AsyncTask);

	/**
	 *	Add an async task onto the outgoing task queue for processing
	 * @param AsyncItem - new heap allocated task to process on the async task thread
	 */
	void QueueAsyncOutgoingItem(class FOnlineAsyncItem* AsyncItem);
	
PACKAGE_SCOPE:
	/** Only the factory makes instances */
	FOnlineSubsystemRPG() = delete;

	explicit FOnlineSubsystemRPG(FName InInstanceName) : FOnlineSubsystemImpl(NULL_SUBSYSTEM, InInstanceName), SessionInterface(nullptr),
	                                                     // VoiceInterface(nullptr),
	                                                     // bVoiceInterfaceInitialized(false),
	                                                     // LeaderboardsInterface(nullptr),
	                                                     // IdentityInterface(nullptr),
	                                                     // AchievementsInterface(nullptr),
	                                                     // StoreV2Interface(nullptr),
	                                                     // MessageSanitizerInterface(nullptr),
	                                                     OnlineAsyncTaskThreadRunnable(nullptr), OnlineAsyncTaskThread(nullptr)
	{
	}

public:
	// DSMaster Http Client
	FDSMasterHttpClient DSMasterClient;
	
private:
	/** Interface to the session services */
	FOnlineSessionRPGPtr SessionInterface;

	/** Interface to the identity registration/auth services */
	FOnlineIdentityRPGPtr IdentityInterface;

	/** Online async task runnable */
	class FOnlineAsyncTaskManagerRPG* OnlineAsyncTaskThreadRunnable;

	/** Online async task thread */
	class FRunnableThread* OnlineAsyncTaskThread;

	// task counter, used to generate unique thread names for each task
	static FThreadSafeCounter TaskCounter;
};

typedef TSharedPtr<FOnlineSubsystemRPG, ESPMode::ThreadSafe> FOnlineSubsystemRPGPtr;
