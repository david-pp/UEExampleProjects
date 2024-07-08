// Copyright Epic Games, Inc. All Rights Reserved.

#include "CoreMinimal.h"
#include "GameServiceEngine.h"
#include "Modules/ModuleManager.h"
#include "IGameServicesModule.h"
#include "GameServiceLocator.h"
#include "Misc/CoreDelegates.h"

DEFINE_LOG_CATEGORY(LogGameServices);

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

	virtual IGameServiceEngine* GetEngine() const override
	{
		return GameServiceEngine.Get();
	}

	void InitServiceEngine()
	{
		GameServiceEngine = MakeShared<FGameServicesEngine>();
		if (GameServiceEngine)
		{
			GameServiceEngine->Init();
		}
	}

	void ShutdownServiceEngine()
	{
		if (GameServiceEngine)
		{
			GameServiceEngine->Stop();
			GameServiceEngine.Reset();
		}
	}

public:
	// IModuleInterface interface

	virtual void StartupModule() override
	{
		InitServiceEngine();
		FCoreDelegates::OnEnginePreExit.AddRaw(this, &FGameServicesModule::HandleEnginePreExit);
	}

	/** Callback for Engine shutdown. */
	void HandleEnginePreExit()
	{
		ShutdownServiceEngine();
	}

	virtual void ShutdownModule() override
	{
		ShutdownServiceEngine();
	}

	virtual bool SupportsDynamicReloading() override { return true; }

protected:
	TSharedPtr<FGameServicesEngine> GameServiceEngine;
};

IMPLEMENT_MODULE(FGameServicesModule, GameServices);
