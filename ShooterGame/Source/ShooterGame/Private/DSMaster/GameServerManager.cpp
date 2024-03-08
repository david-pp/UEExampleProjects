// Fill out your copyright notice in the Description page of Project Settings.

#include "GameServerManager.h"
#include "HAL/RunnableThread.h"
#include "Misc/Paths.h"

/* FGameServerProcess structors
 *****************************************************************************/

FGameServerProcess::FGameServerProcess( const FString& InURL, const FString& InParams, bool InHidden, bool InCreatePipes, bool InCreateThread)
	: FGameServerProcess(InURL, InParams, FPaths::RootDir(), InHidden, InCreatePipes, InCreateThread)
{ }

FGameServerProcess::FGameServerProcess( const FString& InURL, const FString& InParams, const FString& InWorkingDir, bool InHidden, bool InCreatePipes, bool InCreateThread)
	: Canceling(false)
	, EndTime(0)
	, Hidden(InHidden)
	, KillTree(false)
	, Params(InParams)
	, ReadPipe(nullptr)
	, ReturnCode(0)
	, StartTime(0)
	, Thread(nullptr)
	, bIsRunning(false)
	, URL(InURL)
	, WorkingDir(InWorkingDir)
	, WritePipe(nullptr)
	, bCreatePipes(InCreatePipes)
	, bCreateThread(InCreateThread)
	, SleepInterval(0.0f)
{ }

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


/* FGameServerProcess interface
 *****************************************************************************/

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

	if (bCreatePipes && !FPlatformProcess::CreatePipe(ReadPipe, WritePipe))
	{
		return false;
	}

	ProcessInfo.Handle = FPlatformProcess::CreateProc(*URL, *Params, false, Hidden, Hidden, &ProcessInfo.PID, 0, *WorkingDir, WritePipe);
	if (!ProcessInfo.Handle.IsValid())
	{
		return false;
	}

	static int32 MonitoredProcessIndex = 0;
	const FString MonitoredProcessName = FString::Printf( TEXT( "GameServer-%d" ), MonitoredProcessIndex );

	ProcessInfo.UUID = FGuid::NewGuid(); 
	ProcessInfo.Index = MonitoredProcessIndex++;
	ProcessInfo.Name = MonitoredProcessName;
	ProcessInfo.Name = FPlatformProcess::GetApplicationName(ProcessInfo.PID);

	bIsRunning = true;

	if (bCreateThread)
	{
		Thread = FRunnableThread::Create(this, *MonitoredProcessName, 128 * 1024, TPri_AboveNormal);
		if ( !FPlatformProcess::SupportsMultithreading() )
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

void FGameServerProcess::ProcessOutput( const FString& Output )
{
	TArray<FString> LogLines;

	Output.ParseIntoArray(LogLines, TEXT("\n"), false);

	for (int32 LogIndex = 0; LogIndex < LogLines.Num(); ++LogIndex)
	{
		// Don't accept if it is just an empty string
		if (LogLines[LogIndex].IsEmpty() == false)
		{
			OutputDelegate.ExecuteIfBound(LogLines[LogIndex]);
			UE_LOG(LogTemp, Log, TEXT("Child Process  -> %s"), *LogLines[LogIndex]);
		}
	}
	
	// // Append this output to the output buffer
	// OutputBuffer += Output;
	//
	// // Output all the complete lines
	// int32 LineStartIdx = 0;
	// for(int32 Idx = 0; Idx < OutputBuffer.Len(); Idx++)
	// {
	// 	if(OutputBuffer[Idx] == '\r' || OutputBuffer[Idx] == '\n')
	// 	{
	// 		OutputDelegate.ExecuteIfBound(OutputBuffer.Mid(LineStartIdx, Idx - LineStartIdx));
	// 		
	// 		if(OutputBuffer[Idx] == '\r' && Idx + 1 < OutputBuffer.Len() && OutputBuffer[Idx + 1] == '\n')
	// 		{
	// 			Idx++;
	// 		}
	//
	// 		LineStartIdx = Idx + 1;
	// 	}
	// }
	//
	// // Remove all the complete lines from the buffer
	// OutputBuffer.MidInline(LineStartIdx, MAX_int32, false);
}

void FGameServerProcess::TickInternal()
{
	// monitor the process
	if (bCreatePipes)
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
	if (bCreateThread)
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
