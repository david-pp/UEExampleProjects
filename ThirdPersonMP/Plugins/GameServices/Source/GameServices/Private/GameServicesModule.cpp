// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "IGameServicesModule.h"
#include "GameServiceLocator.h"


/**
 * Implements the GameServices module.
 */
class FGameServicesModule : public IGameServicesModule
{
public:
	/** Virtual destructor. */
	virtual ~FGameServicesModule()
	{
	}

public:
	// IGameServicesModule interface
	virtual TSharedRef<IGameServiceLocator> CreateLocator(const TSharedRef<FTypeContainer>& ServiceDependencies) override
	{
		return FGameServiceLocatorFactory::Create(ServiceDependencies);
	}

public:
	// IModuleInterface interface

	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}

	virtual bool SupportsDynamicReloading() override { return true; }
};

IMPLEMENT_MODULE(FGameServicesModule, GameServices);
