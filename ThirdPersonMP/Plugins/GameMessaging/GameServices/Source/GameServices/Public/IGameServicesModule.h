// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameServiceSettings.h"
#include "IGameServiceEngine.h"
#include "Modules/ModuleInterface.h"

/** Declares a log category for this module. */
GAMESERVICES_API DECLARE_LOG_CATEGORY_EXTERN(LogGameServices, Log, All);

class FTypeContainer;
class IGameServiceLocator;

class IGameServicesModule : public IModuleInterface
{
public:
	static IGameServicesModule* Get()
	{
		return static_cast<IGameServicesModule*>(FModuleManager::Get().LoadModule("GameServices"));
	}

	// Get default Service Engine
	static IGameServiceEnginePtr GetServiceEngine()
	{
		IGameServicesModule* Module = Get();
		return Module ? Module->GetEngine() : nullptr;
	}

	// Create a new Service Engine
	static IGameServiceEnginePtr CreateServiceEngine(const FGameServiceEngineSettings& Settings)
	{
		IGameServicesModule* Module = Get();
		return Module ? Module->CreateEngine(Settings) : nullptr;
	}

	/**
	 * Create a locator for Game services.
	 *
	 * @param ServiceDependencies Any dependencies that the services may need.
	 * @return A service locator.
	 */
	virtual TSharedRef<IGameServiceLocator> CreateLocator(const TSharedRef<FTypeContainer>& ServiceDependencies) = 0;

	/*
	 * Get service engine
	 */
	virtual IGameServiceEnginePtr GetEngine() const = 0;

	/**
	 * Create a new service engine
	 */
	virtual IGameServiceEnginePtr CreateEngine(const FGameServiceEngineSettings& Settings) const = 0;

public:
	/** Virtual destructor. */
	virtual ~IGameServicesModule()
	{
	}
};
