// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "RedisClient.h"
#include "GameFramework/Character.h"
#include "ThirdPersonMP/ThirdPersonMPCharacter.h"
#include "DataClientDebugCharacter.generated.h"

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
	
protected:
	TSharedPtr<FRedisClient> RedisClient;
	FString RedisServerIP;
	int32 RedisServerPort;
	FString RedisServerPassword;

	FDateTime NextRedisAliveCheckTime{0};
};
