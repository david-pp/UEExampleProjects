// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "OnlineSubsystemRPGModule.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemNames.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemRPG.h"
#include "OnlineSubsystemRPGTypes.h"

IMPLEMENT_MODULE(FOnlineSubsystemRPGModule, OnlineSubsystemRPG);

/**
 * Class responsible for creating instance(s) of the subsystem
 */
class FOnlineFactoryRPG : public IOnlineFactory
{
public:
	FOnlineFactoryRPG()
	{
	}

	virtual ~FOnlineFactoryRPG()
	{
	}

	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		FOnlineSubsystemRPGPtr OnlineSub = MakeShared<FOnlineSubsystemRPG, ESPMode::ThreadSafe>(InstanceName);
		if (OnlineSub->IsEnabled())
		{
			if (!OnlineSub->Init())
			{
				UE_LOG_ONLINE(Warning, TEXT("RPG API failed to initialize!"));
				OnlineSub->Shutdown();
				OnlineSub = NULL;
			}
		}
		else
		{
			UE_LOG_ONLINE(Warning, TEXT("RPG API disabled!"));
			OnlineSub->Shutdown();
			OnlineSub = NULL;
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemRPGModule::StartupModule()
{
	RPGFactory = new FOnlineFactoryRPG();

	// Create and register our singleton factory with the main online subsystem for easy access
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(RPG_SUBSYSTEM, RPGFactory);
}

void FOnlineSubsystemRPGModule::ShutdownModule()
{
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(RPG_SUBSYSTEM);

	delete RPGFactory;
	RPGFactory = NULL;
}
