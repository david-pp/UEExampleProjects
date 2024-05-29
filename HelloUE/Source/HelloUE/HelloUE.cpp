// Copyright Epic Games, Inc. All Rights Reserved.

#include "HelloUE.h"
#include "Modules/ModuleManager.h"
#include "GameplayDebuggerCategory_Generic.h"
#include "GameplayDebuggerExtension_CharacterSelection.h"
#include "GameplayDebugger.h"

DEFINE_LOG_CATEGORY(LogHello);

class FHelloUEGameModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
		GameplayDebuggerModule.RegisterCategory("HelloUE", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_Generic::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 1);
		GameplayDebuggerModule.RegisterExtension("CharacterSelection", IGameplayDebugger::FOnGetExtension::CreateStatic(&GameplayDebuggerExtension_CharacterSelection::MakeInstance));
#endif
	}

	virtual void ShutdownModule() override
	{
#if WITH_GAMEPLAY_DEBUGGER
		if (IGameplayDebugger::IsAvailable())
		{
			IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
			GameplayDebuggerModule.UnregisterCategory("HelloUE");
			GameplayDebuggerModule.UnregisterExtension("CharacterSelection");
		}
#endif
	}
};

IMPLEMENT_PRIMARY_GAME_MODULE(FHelloUEGameModule, HelloUE, "HelloUE");
