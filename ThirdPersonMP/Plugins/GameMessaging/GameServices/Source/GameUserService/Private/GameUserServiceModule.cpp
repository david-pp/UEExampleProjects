// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "IGameServiceEngine.h"
#include "Modules/ModuleManager.h"
#include "Misc/TypeContainer.h"
#include "IGameServiceProvider.h"
#include "IGameServicesModule.h"
#include "User/GameUserService.h"

/**
 * Implements the GameUserService module.
 */
class FGameUserServiceModule : public IGameServiceProvider
{
public:
	// IModuleInterface interface

	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}

	virtual bool SupportsDynamicReloading() override
	{
		return false;
	}

public:
	// IGameServiceProvider interface
	virtual TSharedPtr<IGameService> GetService(const FString& ServiceName, const TSharedRef<FTypeContainer>& Dependencies) override
	{
		// UserService Only
		IGameServiceEngine* ServiceEngine = IGameServicesModule::GetServiceEngine();
		if (ServiceEngine)
		{
			return ServiceEngine->CreateRPCService<FGameUserService>(TEXT("UserService"));
		}
		return nullptr;
	}
};


IMPLEMENT_MODULE(FGameUserServiceModule, GameUserService);
