// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMaster.h"

DEFINE_LOG_CATEGORY(LogGameMaster);

#define LOCTEXT_NAMESPACE "FGameMasterModule"

void FGameMasterModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	RegisterConsoleCommands();
}

void FGameMasterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	StopHttpServer();
}

// @formatter:off
void FGameMasterModule::RegisterConsoleCommands()
{
	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommand>(
		TEXT("GameMaster.StartServer"),
		TEXT("Start the http web server"),
		FConsoleCommandDelegate::CreateRaw(this, &FGameMasterModule::StartHttpServer)));

	ConsoleCommands.Add(MakeUnique<FAutoConsoleCommand>(
		TEXT("GameMaster.StopServer"),
		TEXT("Stop the http web server"),
		FConsoleCommandDelegate::CreateRaw(this, &FGameMasterModule::StopHttpServer)));
}
// @formatter:on

void FGameMasterModule::UnregisterConsoleCommands()
{
	for (TUniquePtr<FAutoConsoleCommand>& Command : ConsoleCommands)
	{
		Command.Reset();
	}
	ConsoleCommands.Empty();
}

void FGameMasterModule::StartHttpServer()
{
	GameMasterService = MakeShared<FGameMasterService>(50000, TEXT("GameMasterService"));
	if (GameMasterService)
	{
		GameMasterService->RegisterRoutes();
		GameMasterService->Start();
	}

	BackendService = MakeShared<FTinyHttpService>(55555, TEXT("GameMasterBackend"));
	if (BackendService)
	{
		// @formatter:off
		BackendService->Start();
		BackendService->RegisterRoute({TEXT("heart beat info"),
			FHttpPath(TEXT("/hb")),
			EHttpServerRequestVerbs::VERB_GET,
			FHttpServiceHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
			{
				FTinyHttp::DumpServerRequest(Request);
					
				auto Response = FHttpServerResponse::Create(TEXT("Backend"), TEXT("application/text"));
				OnComplete(MoveTemp(Response));
				return true;
			})});

		// @formatter:on
	}
}

void FGameMasterModule::StopHttpServer()
{
	if (GameMasterService)
	{
		GameMasterService->Stop();
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameMasterModule, GameMaster)
