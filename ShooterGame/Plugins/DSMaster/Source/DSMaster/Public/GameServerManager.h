// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameServerManager.generated.h"

class FGameServerProcess;
typedef TSharedPtr<FGameServerProcess> FGameServerProcessPtr;

struct DSMASTER_API FGameServerProcessInfo
{
	/** Generate UUID */
	FGuid UUID;
	/** Monitored Process Index (Unique on current process) */
	uint32 Index;
	/** Process Name */
	FString Name;
	/** Process ID */
	uint32 PID;
	/** Process Handle */
	FProcHandle Handle;

	/** Running GameSession ID*/
	FString SessionId;
};

USTRUCT(BlueprintType)
struct DSMASTER_API FGameServerLaunchSettings
{
	GENERATED_BODY()

	/** MapID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString MapBucket;

	/** Holds the URL of the executable to launch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString URL;
	/** Holds the command line parameters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Params;
	/** Holds the URL of the working dir for the process. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString WorkingDir;

	/** Whether the window of the process should be hidden. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHidden = false;
	/** Holds if we should create pipes */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCreatePipes = true;
	/** Holds if we should create monitor thread */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCreateThread = false;
};

/**
 * Declares a delegate that is executed when a monitored process completed.
 *
 * The first parameter is the process return code.
 */
DECLARE_DELEGATE_OneParam(FOnGameServerProcessCompleted, int32)

/**
 * Declares a delegate that is executed when a monitored process produces output.
 *
 * The first parameter is the produced output.
 */
DECLARE_DELEGATE_OneParam(FOnGameServerProcessOutput, FString)


/**
 * Implements an external process that can be monitored.
 */
class DSMASTER_API FGameServerProcess : public FRunnable, FSingleThreadRunnable
{
public:
	/**
	 * Creates a new monitored process.
	 */
	FGameServerProcess(const FGameServerLaunchSettings& InLaunchSettings);

	/**
	 * Creates a new monitored process.
	 *
	 * @param InURL The URL of the executable to launch.
	 * @param InParams The command line parameters.
	 * @param InHidden Whether the window of the process should be hidden.
	 * @param InCreatePipes Whether the output should be redirected to the caller.
	 */
	static TSharedPtr<FGameServerProcess> CreateGameServer(const FString& InURL, const FString& InParams, bool InHidden, bool InCreatePipes = true, bool InCreateThread = true);
	
	/**
	* Creates a new monitored process.
	*
	* @param InURL The URL of the executable to launch.
	* @param InParams The command line parameters.
	* @param InHidden Whether the window of the process should be hidden.
	* @param InWorkingDir The URL of the working dir where the executable should launch.
	* @param InCreatePipes Whether the output should be redirected to the caller.
	*/
	static TSharedPtr<FGameServerProcess> CreateGameServer(const FString& InURL, const FString& InParams, const FString& InWorkingDir, bool InHidden, bool InCreatePipes = true, bool InCreateThread = true);

	/** Destructor. */
	~FGameServerProcess();

public:
	/**
	 * Cancels the process.
	 *
	 * @param InKillTree Whether to kill the entire process tree when canceling this process.
	 */
	void Cancel(bool InKillTree = false)
	{
		Canceling = true;
		KillTree = InKillTree;
	}

	/**
	 * Gets the duration of time that the task has been running.
	 *
	 * @return Time duration.
	 */
	FTimespan GetDuration() const;

	/**
	 * Gets the Process Handle. The instance can be invalid if the process was not created.
	 *
	 * @return The Process Handle
	 */
	FProcHandle GetProcessHandle() const
	{
		return ProcessInfo.Handle;
	}

	/**
	* Checks whether the process is still running. In single threaded mode, this will tick the thread processing
	*
	* @return true if the process is running, false otherwise.
	*/
	bool Update();

	/** Launches the process. */
	bool Launch();

	/** Monitor the process */
	bool Monitor(uint32 PID);

	/**
	 * Sets the sleep interval to be used in the main thread loop.
	 *
	 * @param InSleepInterval The Sleep interval to use.
	 */
	void SetSleepInterval(float InSleepInterval)
	{
		SleepInterval = InSleepInterval;
	}

public:
	/**
	 * Returns a delegate that is executed when the process has been canceled.
	 *
	 * @return The delegate.
	 */
	FSimpleDelegate& OnCanceled()
	{
		return CanceledDelegate;
	}

	/**
	 * Returns a delegate that is executed when a monitored process completed.
	 *
	 * @return The delegate.
	 */
	FOnGameServerProcessCompleted& OnCompleted()
	{
		return CompletedDelegate;
	}

	/**
	 * Returns a delegate that is executed when a monitored process produces output.
	 *
	 * @return The delegate.
	 */
	FOnGameServerProcessOutput& OnOutput()
	{
		return OutputDelegate;
	}

	/**
	 * Returns the return code from the exited process
	 *
	 * @return Process return code
	 */
	int GetReturnCode() const
	{
		return ReturnCode;
	}

	const FGameServerProcessInfo& GetProcessInfo() const
	{
		return ProcessInfo;
	}

	FGuid GetProcessGuid() const
	{
		return ProcessInfo.UUID;
	}

	FString ToDebugString() const;

	void DumpProcessInfo() const;

	void UpdateRunningSession(const FString& SessionId)
	{
		ProcessInfo.SessionId = SessionId;
	}

public:
	// FRunnable interface

	virtual bool Init() override
	{
		return true;
	}

	virtual uint32 Run() override;

	virtual void Stop() override
	{
		Cancel();
	}

	virtual void Exit() override
	{
	}

	virtual FSingleThreadRunnable* GetSingleThreadInterface() override
	{
		return this;
	}

protected:
	/**
	* FSingleThreadRunnable interface
	*/
	void Tick() override;

	/**
	 * Processes the given output string.
	 *
	 * @param Output The output string to process.
	 */
	void ProcessOutput(const FString& Output);

protected:
	void TickInternal();

	/** Process Launch Settings */
	FGameServerLaunchSettings LaunchSettings;

	/** Process Running Information */
	FGameServerProcessInfo ProcessInfo;

	// Whether the process is being canceled. */
	bool Canceling;

	// Holds the time at which the process ended. */
	FDateTime EndTime;

	// // Whether the window of the process should be hidden. */
	// bool Hidden;

	// Whether to kill the entire process tree when cancelling this process. */
	bool KillTree;

	// // Holds the command line parameters. */
	// FString Params;

	// Holds the read pipe. */
	void* ReadPipe;

	// Holds the return code. */
	int ReturnCode;

	// Holds the time at which the process started. */
	FDateTime StartTime;

	// Holds the monitoring thread object. */
	FRunnableThread* Thread;

	// Is the thread running? 
	TSAN_ATOMIC(bool) bIsRunning;

	// // Holds the URL of the executable to launch. */
	// FString URL;
	//
	// // Holds the URL of the working dir for the process. */
	// FString WorkingDir;

	// Holds the write pipe. */
	void* WritePipe;

	// // Holds if we should create pipes
	// bool bCreatePipes;
	//
	// // Holds if we should create monitor thread
	// bool bCreateThread;

	// Sleep interval to use
	float SleepInterval;

	// Buffered output text which does not contain a newline
	FString OutputBuffer;

protected:

	// Holds a delegate that is executed when the process has been canceled. */
	FSimpleDelegate CanceledDelegate;

	// Holds a delegate that is executed when a monitored process completed. */
	FOnGameServerProcessCompleted CompletedDelegate;

	// Holds a delegate that is executed when a monitored process produces output. */
	FOnGameServerProcessOutput OutputDelegate;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnGameServerStopped, FGameServerProcessPtr, bool);

class DSMASTER_API FGameServerManager
{
public:
	FGameServerManager();
	virtual ~FGameServerManager();
	
	bool LaunchGameServer(const FGameServerLaunchSettings& LaunchSettings);
	FGameServerProcessPtr MonitorGameServer(uint32 ServerPID);

	bool StopGameServer(FGameServerProcessPtr ServerProcess);
	void StopAllGameServers();
	

	void CheckAndUpdateGameServers();
	
	FGameServerProcessPtr FindGameServer(const FGuid&  ServerGuid);

	int32 AllocateGameServerPort();

	FOnGameServerStopped OnGameServerStopped;
	
protected:
	virtual void OnGameServerLaunched(FGameServerProcessPtr ServerProcess);
	virtual void OnGameServerOutput(FGameServerProcessPtr ServerProcess, const FString& Output);
	virtual void OnGameServerCanceled(FGameServerProcessPtr ServerProcess);
	virtual void OnGameServerCompleted(FGameServerProcessPtr ServerProcess, int32 ReturnCode);

protected:
	TMap<FGuid, FGameServerProcessPtr> GameServers;
};
