// Fill out your copyright notice in the Description page of Project Settings.

#include "GameServerManager.h"
#include "HAL/RunnableThread.h"
#include "Misc/Paths.h"

FGameServerProcess::FGameServerProcess(const FGameServerLaunchSettings& InLaunchSettings) : LaunchSettings(InLaunchSettings)
{
	Canceling = false;
	EndTime = 0;
	KillTree = false;
	ReadPipe = nullptr;
	Thread = nullptr;
	bIsRunning = false;

	if (LaunchSettings.WorkingDir.IsEmpty())
	{
		LaunchSettings.WorkingDir = FPaths::RootDir();
	}
}

TSharedPtr<FGameServerProcess> FGameServerProcess::CreateGameServer(const FString& InURL, const FString& InParams, bool InHidden, bool InCreatePipes, bool InCreateThread)
{
	FGameServerLaunchSettings Settings;
	Settings.URL = InURL;
	Settings.Params = InParams;
	Settings.WorkingDir = FPaths::RootDir();
	Settings.bHidden = InHidden;
	Settings.bCreatePipes = InCreatePipes;
	Settings.bCreateThread = InCreateThread;
	return MakeShared<FGameServerProcess>(Settings);
}

TSharedPtr<FGameServerProcess> FGameServerProcess::CreateGameServer(const FString& InURL, const FString& InParams, const FString& InWorkingDir, bool InHidden, bool InCreatePipes, bool InCreateThread)
{
	FGameServerLaunchSettings Settings;
	Settings.URL = InURL;
	Settings.Params = InParams;
	Settings.WorkingDir = InWorkingDir;
	Settings.bHidden = InHidden;
	Settings.bCreatePipes = InCreatePipes;
	Settings.bCreateThread = InCreateThread;
	return MakeShared<FGameServerProcess>(Settings);
}

FGameServerProcess::~FGameServerProcess()
{
	if (bIsRunning)
	{
		Cancel(true);
	}

	if (Thread != nullptr)
	{
		Thread->WaitForCompletion();
		delete Thread;
	}
}

FTimespan FGameServerProcess::GetDuration() const
{
	if (bIsRunning)
	{
		return (FDateTime::UtcNow() - StartTime);
	}

	return (EndTime - StartTime);
}

bool FGameServerProcess::Launch()
{
	if (bIsRunning)
	{
		return false;
	}

	if (LaunchSettings.bCreatePipes && !FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
	{
		return false;
	}

	ProcessInfo.UUID = FGuid::NewGuid();

	FString Params = LaunchSettings.Params;
	Params += FString::Printf(TEXT(" -ServerGuid=%s"), *ProcessInfo.UUID.ToString());
	
	ProcessInfo.Handle = FPlatformProcess::CreateProc(*LaunchSettings.URL, *Params, false, LaunchSettings.bHidden, LaunchSettings.bHidden, &ProcessInfo.PID, 0, *LaunchSettings.WorkingDir, WritePipe);
	if (!ProcessInfo.Handle.IsValid())
	{
		return false;
	}

	static int32 MonitoredProcessIndex = 0;
	const FString MonitoredProcessName = FString::Printf(TEXT("GameServer-%d"), MonitoredProcessIndex);

	ProcessInfo.Index = MonitoredProcessIndex++;
	ProcessInfo.Name = MonitoredProcessName;
	ProcessInfo.Name = FPlatformProcess::GetApplicationName(ProcessInfo.PID);

	bIsRunning = true;

	if (LaunchSettings.bCreateThread)
	{
		Thread = FRunnableThread::Create(this, *MonitoredProcessName, 128 * 1024, TPri_AboveNormal);
		if (!FPlatformProcess::SupportsMultithreading())
		{
			StartTime = FDateTime::UtcNow();
		}
	}
	else
	{
		StartTime = FDateTime::UtcNow();
	}

	return true;
}

/* FGameServerProcess implementation
 *****************************************************************************/

void FGameServerProcess::ProcessOutput(const FString& Output)
{
	TArray<FString> LogLines;

	Output.ParseIntoArray(LogLines, TEXT("\n"), false);

	for (int32 LogIndex = 0; LogIndex < LogLines.Num(); ++LogIndex)
	{
		// Don't accept if it is just an empty string
		if (LogLines[LogIndex].IsEmpty() == false)
		{
			OutputDelegate.ExecuteIfBound(LogLines[LogIndex]);
			// UE_LOG(LogTemp, Log, TEXT("Child Process  -> %s"), *LogLines[LogIndex]);
		}
	}
}

void FGameServerProcess::TickInternal()
{
	// monitor the process
	if (LaunchSettings.bCreatePipes)
	{
		ProcessOutput(FPlatformProcess::ReadPipe(ReadPipe));
	}

	if (Canceling)
	{
		FPlatformProcess::TerminateProc(ProcessInfo.Handle, KillTree);
		CanceledDelegate.ExecuteIfBound();
		bIsRunning = false;
	}
	else if (!FPlatformProcess::IsProcRunning(ProcessInfo.Handle))
	{
		EndTime = FDateTime::UtcNow();

		// close output pipes
		if (ReadPipe && WritePipe)
		{
			FPlatformProcess::ClosePipe(ReadPipe, WritePipe);
			ReadPipe = WritePipe = nullptr;
		}

		// get completion status
		if (!FPlatformProcess::GetProcReturnCode(ProcessInfo.Handle, &ReturnCode))
		{
			ReturnCode = -1;
		}

		CompletedDelegate.ExecuteIfBound(ReturnCode);
		bIsRunning = false;
	}
}


bool FGameServerProcess::Update()
{
	if (LaunchSettings.bCreateThread)
	{
		if (!FPlatformProcess::SupportsMultithreading())
		{
			FPlatformProcess::Sleep(SleepInterval);
			Tick();
		}
	}
	else
	{
		Tick();
	}

	return bIsRunning;
}


/* FRunnable interface
 *****************************************************************************/

FString FGameServerProcess::ToDebugString() const
{
	return FString::Printf(TEXT("GameServer:%s"), *GetProcessGuid().ToString());
}

void FGameServerProcess::DumpProcessInfo() const
{
	UE_LOG(LogTemp, Log, TEXT("----- %s -----"), *ToDebugString());
	UE_LOG(LogTemp, Log, TEXT("Info.Name = %s"), *ProcessInfo.Name);
	UE_LOG(LogTemp, Log, TEXT("Info.PID = %d"), ProcessInfo.PID);
	UE_LOG(LogTemp, Log, TEXT("Settings.URL = %s"), *LaunchSettings.URL);
	UE_LOG(LogTemp, Log, TEXT("Settings.Params = %s"), *LaunchSettings.Params);
}

uint32 FGameServerProcess::Run()
{
	StartTime = FDateTime::UtcNow();
	while (bIsRunning)
	{
		FPlatformProcess::Sleep(SleepInterval);
		TickInternal();
	}

	return 0;
}

/* FRunnableSingleThreaded interface
*****************************************************************************/
void FGameServerProcess::Tick()
{
	if (bIsRunning)
	{
		TickInternal();
	}
}


// -----------------------------

FGameServerManager::FGameServerManager()
{
}

FGameServerManager::~FGameServerManager()
{
}

bool FGameServerManager::LaunchGameServer(const FGameServerLaunchSettings& LaunchSettings)
{
	TSharedPtr<FGameServerProcess> GameServer = MakeShared<FGameServerProcess>(LaunchSettings);

	// Bind Callbacks
	GameServer->OnOutput().BindLambda([this, GameServer](const FString& Output)
	{
		OnGameServerOutput(GameServer, Output);
	});

	GameServer->OnCanceled().BindLambda([this, GameServer]()
	{
		OnGameServerCanceled(GameServer);
	});

	GameServer->OnCompleted().BindLambda([this, GameServer](int32 ReturnCode)
	{
		OnGameServerCompleted(GameServer, ReturnCode);
	});

	// Launch Game Server
	if (!GameServer->Launch())
	{
		return false;
	}

	GameServers.Add(GameServer->GetProcessGuid(), GameServer);
	OnGameServerLaunched(GameServer);
	return true;
}

bool FGameServerManager::StopGameServer(FGameServerProcessPtr ServerProcess)
{
	if (ServerProcess)
	{
		ServerProcess->Cancel(true);
		return true;
	}
	return false;
}

void FGameServerManager::StopAllGameServers()
{
	for (auto& GameServerPair : GameServers)
	{
		GameServerPair.Value->Cancel(true);
	}
}

FGameServerProcessPtr FGameServerManager::FindGameServer(const FGuid& ServerGuid)
{
	return GameServers.FindRef(ServerGuid);
}

void FGameServerManager::CheckAndUpdateGameServers()
{
	TSet<FGuid> GameServerOvered;
	for (auto& GameServerPair : GameServers)
	{
		TSharedPtr<FGameServerProcess> ServerProcess = GameServerPair.Value;
		if (ServerProcess)
		{
			if (ServerProcess->Update())
			{
				UE_LOG(LogTemp, Log, TEXT("%s - Is Running: %s"), *ServerProcess->ToDebugString(), *ServerProcess->GetProcessInfo().SessionId);
			}
			else
			{
				GameServerOvered.Add(ServerProcess->GetProcessGuid());
				UE_LOG(LogTemp, Warning, TEXT("%s - is Over: %s"),  *ServerProcess->ToDebugString(), *ServerProcess->GetProcessInfo().SessionId);
			}
		}
	}

	for (auto& ServerOvered : GameServerOvered)
	{
		GameServers.Remove(ServerOvered);
	}
}

int32 FGameServerManager::AllocateGameServerPort()
{
	static int32 LastServerPort = 7000;
	return LastServerPort++;
}

void FGameServerManager::OnGameServerLaunched(FGameServerProcessPtr ServerProcess)
{
	UE_LOG(LogTemp, Warning, TEXT("GameServer:%s - Launched"), *ServerProcess->GetProcessGuid().ToString());
	ServerProcess->DumpProcessInfo();
}

void FGameServerManager::OnGameServerOutput(FGameServerProcessPtr ServerProcess, const FString& Output)
{
	UE_LOG(LogTemp, Log, TEXT("GameServer:%s > %s"), *ServerProcess->GetProcessGuid().ToString(), *Output);
}

void FGameServerManager::OnGameServerCanceled(FGameServerProcessPtr ServerProcess)
{
	UE_LOG(LogTemp, Warning, TEXT("GameServer:%s - Canceled"), *ServerProcess->GetProcessGuid().ToString());
	OnGameServerStopped.Broadcast(ServerProcess, true);
}

void FGameServerManager::OnGameServerCompleted(FGameServerProcessPtr ServerProcess, int32 ReturnCode)
{
	UE_LOG(LogTemp, Warning, TEXT("GameServer:%s - Completed:%d"), *ServerProcess->GetProcessGuid().ToString(), ReturnCode);
	OnGameServerStopped.Broadcast(ServerProcess, false);
}
