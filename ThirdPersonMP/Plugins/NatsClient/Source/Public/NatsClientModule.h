#pragma once

#include "Modules/ModuleManager.h"
#include "CoreMinimal.h"
#include "NatsClient.h"

class NATSCLIENT_API INatsClientModule : public IModuleInterface
{
public:
	static INatsClientModule& Get()
	{
		return FModuleManager::LoadModuleChecked<INatsClientModule>("NatsClient");
	}

	static bool IsAvailable()
	{
		return FModuleManager::Get().IsModuleLoaded("NatsClient");
	}

	virtual FNatsClientPtr CreateNatsClient() = 0;
	
};

DECLARE_LOG_CATEGORY_EXTERN(LogNats, Log, All);