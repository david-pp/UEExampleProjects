// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "OnlineAsyncTaskManager.h"

/**
 *	RPG version of the async task manager to register the various RPG callbacks with the engine
 */
class FOnlineAsyncTaskManagerRPG : public FOnlineAsyncTaskManager
{
protected:

	/** Cached reference to the main online subsystem */
	class FOnlineSubsystemRPG* RPGSubsystem;

public:

	FOnlineAsyncTaskManagerRPG(class FOnlineSubsystemRPG* InOnlineSubsystem)
		: RPGSubsystem(InOnlineSubsystem)
	{
	}

	~FOnlineAsyncTaskManagerRPG() 
	{
	}

	// FOnlineAsyncTaskManager
	virtual void OnlineTick() override;
};
