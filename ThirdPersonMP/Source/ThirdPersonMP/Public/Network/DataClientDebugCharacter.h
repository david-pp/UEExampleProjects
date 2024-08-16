// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RedisClient.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"
#include "DataClientDebugCharacter.generated.h"

USTRUCT(BlueprintType)
struct FAsyncDemoResult
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 RetValue = 0;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString RetString;
};

DECLARE_MULTICAST_DELEGATE_OneParam(FNativeOnAsyncDemoComplete, const FAsyncDemoResult& Result);
typedef FNativeOnAsyncDemoComplete::FDelegate FNativeOnAsyncDemoCompleteDelegate;
// DECLARE_DELEGATE_OneParam(FNativeOnAsyncDemoCompleteDelegate, const FAsyncDemoResult& Result);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAsyncDemoComplete, const FAsyncDemoResult&, Result);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnAsyncDemoCompleteDelegate, const FAsyncDemoResult&, Result);

UCLASS()
class UAsyncDemoAction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject"))
	static UAsyncDemoAction* AsyncDemo(UObject* WorldContextObject, const FString& InParam1);

	/** Execute the actual operation */
	virtual void Activate() override;

	/** Delegate called when completes */
	UPROPERTY(BlueprintAssignable)
	FOnAsyncDemoComplete Completed;

protected:
	// Parameters
	FString Param1;
	// The world context object in which this call is taking place
	UObject* WorldContextObject;
};


UCLASS()
class THIRDPERSONMP_API ADataClientDebugCharacter : public AThirdPersonMPCharacter
{
	GENERATED_BODY()

public:
	ADataClientDebugCharacter(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION(BlueprintCallable, Category = "Redis")
	bool NewRedisClient(const FString& InIP = FString(TEXT("127.0.0.1")), int32 InPort = 6379, const FString& InPassword = FString(TEXT("")));

	UFUNCTION(BlueprintCallable, Category = "Redis")
	void Ping();

	UFUNCTION(BlueprintCallable, Category = "Redis")
	void Demo1();


	UFUNCTION(BlueprintCallable, Category = "Redis")
	void DebugAsync();

	UFUNCTION()
	void OnAsyncDemoCallback(const FAsyncDemoResult& Result);


	/** Async with Future */
	TFuture<FAsyncDemoResult> AsyncDemo(FString Param1);

	/** Async API : with callback or not */
	void AsyncDemoAPI(FString Param1, const FNativeOnAsyncDemoCompleteDelegate& OnComplete = FNativeOnAsyncDemoCompleteDelegate());

	/** Async API - used by Blueprint */
	UFUNCTION(BlueprintCallable)
	void AsyncDemoAPI(FString Param1, const FOnAsyncDemoCompleteDelegate& OnComplete);

protected:
	TSharedPtr<FRedisClient> RedisClient;
	FString RedisServerIP;
	int32 RedisServerPort;
	FString RedisServerPassword;

	FDateTime NextRedisAliveCheckTime{0};
};
