// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

/**
 * Online subsystem module class  (RPG Implementation)
 * Code related to the loading of the RPG module
 */
class FOnlineSubsystemRPGModule : public IModuleInterface
{
private:

	/** Class responsible for creating instance(s) of the subsystem */
	class FOnlineFactoryRPG* RPGFactory;

public:

	FOnlineSubsystemRPGModule() : 
		RPGFactory(NULL)
	{}

	virtual ~FOnlineSubsystemRPGModule() {}

	// IModuleInterface

	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

	virtual bool SupportsAutomaticShutdown() override
	{
		return false;
	}
};
