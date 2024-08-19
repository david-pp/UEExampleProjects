#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TinyRedisTypes.h"
#include "Async/Future.h"
#include "TinyRedisInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UTinyRedisInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Redis APIs
 */
class TINYREDIS_API ITinyRedisInterface
{
	GENERATED_BODY()

public:
	// Sync API
	virtual FRedisReply ExecCommand(const FString& InCommand, ERedisCommandType InCommandType = ERedisCommandType::UNKNOWN) = 0;
	// Async API for Native
	virtual TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand, ERedisCommandType InCommandType = ERedisCommandType::UNKNOWN) = 0;
	// Async API for Blueprint
	UFUNCTION(BlueprintCallable, Category=TinyRedis)
	virtual bool AsyncExecCommand(const FString& InCommand, const FOnRedisReplyDelegate& OnReply);


	virtual FRedisReply SetStr(const FString& Key, const FString& Value)
	{
		FString Command = FString::Printf(TEXT("SET %s %s"), *Key, *LexToString(Value));
		return ExecCommand(Command, ERedisCommandType::SET);
	}

public:
	// ~ String APIs
	// template <typename ValueType>
	// FRedisReply Set(const FString& Key, const ValueType& Value)
	// {
	// 	
	// }

	TFuture<FRedisReply> AsyncGet(const FString& Key)
	{
		FString Command = FString::Printf(TEXT("GET %s"), *Key);
		return AsyncExecCommand(Command, ERedisCommandType::GET);
	}

	template <typename ValueType>
	TFuture<FRedisReply> AsyncSet(const FString& Key, const ValueType& Value)
	{
		FString Command = FString::Printf(TEXT("SET %s %s"), *Key, *LexToString(Value));
		return AsyncExecCommand(Command, ERedisCommandType::SET);
	}


	// ~ Hash APIs
	TFuture<FRedisReply> AsyncHashGetAll(const FString& InKey);
	TFuture<FRedisReply> AsyncHashMultiSet(const FString& InKey, const TMap<FString, FString>& FieldValuePairs);
	TFuture<FRedisReply> AsyncHashMultiGet(const FString& InKey, const TArray<FString>& Fields);

	UFUNCTION(BlueprintCallable, Category=TinyRedis)
	virtual void AsyncHashGetAll(const FString& InKey, const FOnRedisReplyDelegate& OnReply);
	UFUNCTION(BlueprintCallable, Category=TinyRedis)
	virtual void AsyncHashMultiSet(const FString& InKey, const TMap<FString, FString>& FieldValuePairs, const FOnRedisReplyDelegate& OnReply);
	UFUNCTION(BlueprintCallable, Category=TinyRedis)
	virtual void AsyncHashMultiGet(const FString& InKey, const TArray<FString>& Fields, const FOnRedisReplyDelegate& OnReply);
};

typedef TSharedPtr<ITinyRedisInterface, ESPMode::ThreadSafe> IRedisInterfacePtr;
