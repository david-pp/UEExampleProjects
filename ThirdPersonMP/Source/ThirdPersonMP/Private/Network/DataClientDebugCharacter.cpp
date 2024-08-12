// Fill out your copyright notice in the Description page of Project Settings.


#include "Network/DataClientDebugCharacter.h"

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
