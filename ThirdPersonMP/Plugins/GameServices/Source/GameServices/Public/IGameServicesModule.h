// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"

class FTypeContainer;
class IGameServiceLocator;

class IGameServicesModule : public IModuleInterface
{
public:
	/**
	 * Create a locator for Game services.
	 *
	 * @param ServiceDependencies Any dependencies that the services may need.
	 * @return A service locator.
	 */
	virtual TSharedRef<IGameServiceLocator> CreateLocator(const TSharedRef<FTypeContainer>& ServiceDependencies) = 0;

public:
	/** Virtual destructor. */
	virtual ~IGameServicesModule()
	{
	}
};
