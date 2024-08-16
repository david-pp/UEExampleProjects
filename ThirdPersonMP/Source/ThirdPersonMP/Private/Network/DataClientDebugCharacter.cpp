// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/DataClientDebugCharacter.h"

#include "HAL/ThreadManager.h"
#include "Kismet/GameplayStatics.h"

UAsyncDemoAction* UAsyncDemoAction::AsyncDemo(UObject* WorldContextObject, const FString& InParam1)
{
	UAsyncDemoAction* Action = NewObject<UAsyncDemoAction>();
	Action->WorldContextObject = WorldContextObject;
	Action->Param1 = InParam1;
	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UAsyncDemoAction::Activate()
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull);
	if (World)
	{
		ADataClientDebugCharacter* Character = Cast<ADataClientDebugCharacter>(UGameplayStatics::GetPlayerCharacter(WorldContextObject, 0));
		if (Character)
		{
			// with native callback api
			Character->AsyncDemoAPI(Param1, FNativeOnAsyncDemoCompleteDelegate::CreateLambda([this](const FAsyncDemoResult& Result)
			{
				Completed.Broadcast(Result);
				SetReadyToDestroy();
			}));
		}
	}
}

/// -------------

ADataClientDebugCharacter::ADataClientDebugCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
}

void ADataClientDebugCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void ADataClientDebugCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Reconnection with redis server
	const FDateTime Now = FDateTime::UtcNow();
	if (Now > NextRedisAliveCheckTime)
	{
		if (RedisClient)
		{
			if (!RedisClient->ExecCommand("PING"))
			{
				RedisClient->ConnectToRedis(RedisServerIP, RedisServerPort, RedisServerPassword);
			}
		}

		NextRedisAliveCheckTime = Now + FTimespan::FromSeconds(20);
	}
}

void ADataClientDebugCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

bool ADataClientDebugCharacter::NewRedisClient(const FString& InIP, int32 InPort, const FString& InPassword)
{
	if (RedisClient)
	{
		RedisClient->DisconnectRedis();
		RedisClient = nullptr;
	}

	RedisServerIP = InIP;
	RedisServerPort = InPort;
	RedisServerPassword = InPassword;

	RedisClient = MakeShared<FRedisClient>();
	if (!RedisClient->ConnectToRedis(InIP, InPort, InPassword))
	{
		RedisClient = nullptr;
		return false;
	}

	return true;
}

void ADataClientDebugCharacter::Ping()
{
	if (RedisClient)
	{
		RedisClient->ExecCommand("PING");
	}
}

void ADataClientDebugCharacter::Demo1()
{
	if (RedisClient)
	{
		FString Key = TEXT("user:1000");
		TMap<FString, FString> ResultMap;

		if (RedisClient->HGetAll(Key, ResultMap))
		{
			FString OutUserName;
			int32 OutUserAge = 0;
			auto UserName = ResultMap.Find(TEXT("name"));
			auto UserAge = ResultMap.Find(TEXT("age"));
			if (UserName && !UserName->IsEmpty()) LexFromString(OutUserName, GetData(*UserName));
			if (UserAge && !UserAge->IsEmpty()) LexFromString(OutUserAge, GetData(*UserAge));

			UE_LOG(LogTemp, Warning, TEXT("Demo1 : %s - %d"), *OutUserName, OutUserAge);
		}
	}
}

void ADataClientDebugCharacter::DebugAsync()
{
#if 0
	auto Future = AsyncPool(*GThreadPool, []()
	                        {
		                        FAsyncDemoResult Result;
		                        uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		                        FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
		                        UE_LOG(LogTemp, Warning, TEXT("Async Task: %d, %s"), CurrentThreadId, *CurrentThreadName);
		                        Result.RetValue = CurrentThreadId;
		                        Result.RetString = CurrentThreadName;
		                        return Result;
	                        }, []()
	                        {
		                        uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		                        FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
		                        UE_LOG(LogTemp, Warning, TEXT("Async Task Complete : %d, %s"), CurrentThreadId, *CurrentThreadName);
	                        });

	Future.Then([](TFuture<FAsyncDemoResult> InFuture)
	{
		FAsyncDemoResult Result;
		if (InFuture.IsReady())
		{
			Result = InFuture.Get();
		}

		uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
		UE_LOG(LogTemp, Warning, TEXT("Async Task Future Complete : %d, %s, Result:%s"), CurrentThreadId, *CurrentThreadName, *Result.RetString);
	});

#endif

	// Async
	AsyncDemo(TEXT("Test1")).Then([](TFuture<FAsyncDemoResult> Future)
	{
		if (Future.IsReady())
		{
			FAsyncDemoResult Result = Future.Get();
			uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
			FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
			UE_LOG(LogTemp, Warning, TEXT("Async API Callback@(%d, %s), Result:%s"), CurrentThreadId, *CurrentThreadName, *Result.RetString);
		}
	});

	// Callback
	AsyncDemoAPI(TEXT("Test2"), FNativeOnAsyncDemoCompleteDelegate::CreateLambda([](const FAsyncDemoResult& Result)
	{
		uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
		UE_LOG(LogTemp, Warning, TEXT("Native API Callback@(%d, %s), Result:%s"), CurrentThreadId, *CurrentThreadName, *Result.RetString);
	}));

	// Dynamic Delegates
	FOnAsyncDemoCompleteDelegate Delegate;
	Delegate.BindDynamic(this, &ADataClientDebugCharacter::OnAsyncDemoCallback);
	AsyncDemoAPI(TEXT("Test3"), Delegate);

	// Async Blueprint Action
	UAsyncDemoAction* Action = UAsyncDemoAction::AsyncDemo(GetWorld(), TEXT("Test4"));
	if (Action) // will destroy when callback is called
	{
		Action->Completed.AddDynamic(this, &ADataClientDebugCharacter::OnAsyncDemoCallback);
		Action->Activate();
	}
}

void ADataClientDebugCharacter::OnAsyncDemoCallback(const FAsyncDemoResult& Result)
{
	uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
	FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
	UE_LOG(LogTemp, Warning, TEXT("Callback Function@(%d, %s), Result:%s"), CurrentThreadId, *CurrentThreadName, *Result.RetString);
}

TFuture<FAsyncDemoResult> ADataClientDebugCharacter::AsyncDemo(FString Param1)
{
	TPromise<FAsyncDemoResult> Promise;
	TFuture<FAsyncDemoResult> Future = Promise.GetFuture();

	AsyncPool(*GThreadPool, [Param1]()
	{
		// do sth in thread pool and return result, such as: rpc/database/...
		uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
		FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
		UE_LOG(LogTemp, Warning, TEXT("AsyncDemo@ThreadPool(%d, %s), Param:%s"), CurrentThreadId, *CurrentThreadName, *Param1);

		FAsyncDemoResult Result;
		Result.RetValue = CurrentThreadId;
		Result.RetString = CurrentThreadName + "+" + Param1;
		return Result;
	}).Then([Promise = MoveTemp(Promise)](TFuture<FAsyncDemoResult> InFuture) mutable
	{
		// result callback in thread pool
		if (InFuture.IsReady())
		{
			FAsyncDemoResult Result = InFuture.Get();

			// callback in game thread
			AsyncTask(ENamedThreads::GameThread, [Promise = MoveTemp(Promise), Result]() mutable
			{
				Promise.SetValue(Result);
			});
		}
	});

	return MoveTemp(Future);
}

void ADataClientDebugCharacter::AsyncDemoAPI(FString Param1, const FNativeOnAsyncDemoCompleteDelegate& OnComplete)
{
	AsyncDemo(Param1).Then([OnComplete](TFuture<FAsyncDemoResult> Future)
	{
		if (Future.IsReady())
		{
			FAsyncDemoResult Result = Future.Get();
			uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
			FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
			UE_LOG(LogTemp, Warning, TEXT("AsyncDemoAPI-Native@GameThread(%d, %s), Result:%s"), CurrentThreadId, *CurrentThreadName, *Result.RetString);
			OnComplete.ExecuteIfBound(Result);
		}
	});
}

void ADataClientDebugCharacter::AsyncDemoAPI(FString Param1, const FOnAsyncDemoCompleteDelegate& OnComplete)
{
	AsyncDemo(Param1).Then([OnComplete](TFuture<FAsyncDemoResult> Future)
	{
		if (Future.IsReady())
		{
			FAsyncDemoResult Result = Future.Get();
			uint32 CurrentThreadId = FPlatformTLS::GetCurrentThreadId();
			FString CurrentThreadName = FThreadManager::Get().GetThreadName(CurrentThreadId);
			UE_LOG(LogTemp, Warning, TEXT("AsyncDemoAPI-Blueprint@GameThread(%d, %s), Result:%s"), CurrentThreadId, *CurrentThreadName, *Result.RetString);
			OnComplete.ExecuteIfBound(Result);
		}
	});
}
