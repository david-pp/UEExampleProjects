// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "GameStorageEngine.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGameStorage, Log, All);

class FGameStorageModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static FGameStorageModule* Get()
	{
		return static_cast<FGameStorageModule*>(FModuleManager::Get().LoadModule("GameStorage"));
	}

public:
	/*
	 * Get default game storage interface
	 */
	virtual IGameStorageEnginePtr GetStorageEngine() const;

	/**
	 * Create a game storage interface
	 */
	virtual IGameStorageEnginePtr CreateStorageEngine(const FGameStorageEngineSettings& Settings);

protected:
	/**
	 * default storage setting & engine
	 */
	FGameStorageEngineSettings StorageEngineSettings;
	IGameStorageEnginePtr GameStorageEngine;
};
