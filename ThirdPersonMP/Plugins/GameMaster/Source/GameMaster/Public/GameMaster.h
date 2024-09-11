// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameMasterService.h"
#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGameMaster, Log, All);

class FGameMasterModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;


	/** Register console commands. */
	void RegisterConsoleCommands();
	/** Unregister console commands. */
	void UnregisterConsoleCommands();

	void StartHttpServer();
	void StopHttpServer();

protected:
	/** Console commands handles. */
	TArray<TUniquePtr<FAutoConsoleCommand>> ConsoleCommands;

	TSharedPtr<FGameMasterService> GameMasterService;
	TSharedPtr<FTinyHttpService> BackendService;
};
