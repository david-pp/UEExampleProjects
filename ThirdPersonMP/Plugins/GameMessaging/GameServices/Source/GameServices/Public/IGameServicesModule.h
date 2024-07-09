// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
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

	static IGameServiceEngine* GetServiceEngine()
	{
		IGameServicesModule* Module = Get();
		return Module ? Module->GetEngine() : nullptr;
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
	virtual IGameServiceEngine* GetEngine() const = 0;

public:
	/** Virtual destructor. */
	virtual ~IGameServicesModule()
	{
	}
};
