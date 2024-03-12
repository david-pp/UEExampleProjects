// Copyright Epic Games, Inc. All Rights Reserved.

#include "OnlineSubsystemRPG.h"
#include "HAL/RunnableThread.h"
#include "OnlineAsyncTaskManagerRPG.h"
#include "OnlineSessionRPG.h"
#include "OnlineIdentityRPG.h"
#include "Stats/Stats.h"

FThreadSafeCounter FOnlineSubsystemRPG::TaskCounter;

IOnlineSessionPtr FOnlineSubsystemRPG::GetSessionInterface() const
{
	return SessionInterface;
}

IOnlineFriendsPtr FOnlineSubsystemRPG::GetFriendsInterface() const
{
	return nullptr;
}

IOnlinePartyPtr FOnlineSubsystemRPG::GetPartyInterface() const
{
	return nullptr;
}

IOnlineGroupsPtr FOnlineSubsystemRPG::GetGroupsInterface() const
{
	return nullptr;
}

IOnlineSharedCloudPtr FOnlineSubsystemRPG::GetSharedCloudInterface() const
{
	return nullptr;
}

IOnlineUserCloudPtr FOnlineSubsystemRPG::GetUserCloudInterface() const
{
	return nullptr;
}

IOnlineEntitlementsPtr FOnlineSubsystemRPG::GetEntitlementsInterface() const
{
	return nullptr;
};

IOnlineLeaderboardsPtr FOnlineSubsystemRPG::GetLeaderboardsInterface() const
{
	return nullptr;
}

IOnlineVoicePtr FOnlineSubsystemRPG::GetVoiceInterface() const
{
	return nullptr;
}

IOnlineExternalUIPtr FOnlineSubsystemRPG::GetExternalUIInterface() const
{
	return nullptr;
}

IOnlineTimePtr FOnlineSubsystemRPG::GetTimeInterface() const
{
	return nullptr;
}

IOnlineIdentityPtr FOnlineSubsystemRPG::GetIdentityInterface() const
{
	return IdentityInterface;
}

IOnlineTitleFilePtr FOnlineSubsystemRPG::GetTitleFileInterface() const
{
	return nullptr;
}

IOnlineStoreV2Ptr FOnlineSubsystemRPG::GetStoreV2Interface() const
{
	return nullptr;
}

IOnlinePurchasePtr FOnlineSubsystemRPG::GetPurchaseInterface() const
{
	return nullptr;
}

IOnlineEventsPtr FOnlineSubsystemRPG::GetEventsInterface() const
{
	return nullptr;
}

IOnlineAchievementsPtr FOnlineSubsystemRPG::GetAchievementsInterface() const
{
	return nullptr;
}

IOnlineSharingPtr FOnlineSubsystemRPG::GetSharingInterface() const
{
	return nullptr;
}

IOnlineUserPtr FOnlineSubsystemRPG::GetUserInterface() const
{
	return nullptr;
}

IOnlineMessagePtr FOnlineSubsystemRPG::GetMessageInterface() const
{
	return nullptr;
}

IOnlinePresencePtr FOnlineSubsystemRPG::GetPresenceInterface() const
{
	return nullptr;
}

IOnlineChatPtr FOnlineSubsystemRPG::GetChatInterface() const
{
	return nullptr;
}

IOnlineStatsPtr FOnlineSubsystemRPG::GetStatsInterface() const
{
	return nullptr;
}

IOnlineTurnBasedPtr FOnlineSubsystemRPG::GetTurnBasedInterface() const
{
	return nullptr;
}

IOnlineTournamentPtr FOnlineSubsystemRPG::GetTournamentInterface() const
{
	return nullptr;
}

IMessageSanitizerPtr FOnlineSubsystemRPG::GetMessageSanitizer(int32 LocalUserNum, FString& OutAuthTypeToExclude) const
{
	return nullptr;
}

bool FOnlineSubsystemRPG::Tick(float DeltaTime)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FOnlineSubsystemRPG_Tick);

	if (!FOnlineSubsystemImpl::Tick(DeltaTime))
	{
		return false;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->GameTick();
	}

	if (SessionInterface.IsValid())
	{
		SessionInterface->Tick(DeltaTime);
	}

	return true;
}

bool FOnlineSubsystemRPG::Init()
{
	const bool bRPGInit = true;

	if (bRPGInit)
	{
		// Create the online async task thread
		OnlineAsyncTaskThreadRunnable = new FOnlineAsyncTaskManagerRPG(this);
		check(OnlineAsyncTaskThreadRunnable);
		OnlineAsyncTaskThread = FRunnableThread::Create(OnlineAsyncTaskThreadRunnable, *FString::Printf(TEXT("OnlineAsyncTaskThreadRPG %s(%d)"), *InstanceName.ToString(), TaskCounter.Increment()), 128 * 1024, TPri_Normal);
		check(OnlineAsyncTaskThread);
		UE_LOG_ONLINE(Verbose, TEXT("Created thread (ID:%d)."), OnlineAsyncTaskThread->GetThreadID());

		SessionInterface = MakeShareable(new FOnlineSessionRPG(this));
		IdentityInterface = MakeShareable(new FOnlineIdentityRPG(this));

		if (SessionInterface)
		{
			SessionInterface->HttpSession.Init(TEXT("http://127.0.0.1:30000"));
		}
	}
	else
	{
		Shutdown();
	}

	return bRPGInit;
}

bool FOnlineSubsystemRPG::Shutdown()
{
	UE_LOG_ONLINE(VeryVerbose, TEXT("FOnlineSubsystemRPG::Shutdown()"));

	FOnlineSubsystemImpl::Shutdown();

	if (OnlineAsyncTaskThread)
	{
		// Destroy the online async task thread
		delete OnlineAsyncTaskThread;
		OnlineAsyncTaskThread = nullptr;
	}

	if (OnlineAsyncTaskThreadRunnable)
	{
		delete OnlineAsyncTaskThreadRunnable;
		OnlineAsyncTaskThreadRunnable = nullptr;
	}

#define DESTRUCT_INTERFACE(Interface) \
	if (Interface.IsValid()) \
	{ \
		ensure(Interface.IsUnique()); \
		Interface = nullptr; \
	}

	// Destruct the interfaces
	// DESTRUCT_INTERFACE(IdentityInterface);
	DESTRUCT_INTERFACE(SessionInterface);

#undef DESTRUCT_INTERFACE

	return true;
}

FString FOnlineSubsystemRPG::GetAppId() const
{
	return TEXT("");
}

bool FOnlineSubsystemRPG::Exec(UWorld* InWorld, const TCHAR* Cmd, FOutputDevice& Ar)
{
	if (FOnlineSubsystemImpl::Exec(InWorld, Cmd, Ar))
	{
		return true;
	}
	return false;
}

FText FOnlineSubsystemRPG::GetOnlineServiceName() const
{
	return NSLOCTEXT("OnlineSubsystemRPG", "OnlineServiceName", "RPG");
}

void FOnlineSubsystemRPG::QueueAsyncTask(FOnlineAsyncTask* AsyncTask)
{
	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->AddToInQueue(AsyncTask);
	}
}

void FOnlineSubsystemRPG::QueueAsyncOutgoingItem(FOnlineAsyncItem* AsyncItem)
{
	if (OnlineAsyncTaskThreadRunnable)
	{
		OnlineAsyncTaskThreadRunnable->AddToOutQueue(AsyncItem);
	}
}
