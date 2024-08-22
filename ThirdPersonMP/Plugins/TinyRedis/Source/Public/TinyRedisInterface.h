#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TinyRedisTypes.h"
#include "Async/Future.h"
#include "TinyRedisInterface.generated.h"

class FRedisConnection;

/**
 * Redis Command
 */
class TINYREDIS_API ITinyRedisCommand
{
public:
	virtual ~ITinyRedisCommand()
	{
	}

	virtual ERedisCommandType GetCommandType() const
	{
		return ERedisCommandType::UNKNOWN;
	}

	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) = 0;
	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) = 0;

	FNativeOnRedisReplyDelegate OnReply;
};

typedef TSharedPtr<ITinyRedisCommand> ITinyRedisCommandPtr;

/**
 * Redis Pipeline :
 * - Redis pipelining is a technique for improving performance by issuing multiple commands at once
 *   without waiting for the response to each individual command. 
 * - Read More: https://redis.io/docs/latest/develop/use/pipelining/
 */
class TINYREDIS_API ITinyRedisPipeline
{
public:
	virtual ~ITinyRedisPipeline() = default;

	virtual void Start() = 0;
	virtual void AppendCommand(ITinyRedisCommandPtr Command, const FNativeOnRedisReplyDelegate& OnReply = FNativeOnRedisReplyDelegate()) = 0;
	virtual FRedisPipelineReply Commit() = 0;
	virtual TFuture<FRedisPipelineReply> AsyncCommit() = 0;

	template <typename CommandType, typename... InArgTypes>
	TSharedPtr<CommandType> Command(InArgTypes&&... Args)
	{
		auto Cmd = MakeShared<CommandType>(Args...);
		AppendCommand(Cmd);
		return Cmd;
	}
};

typedef TSharedPtr<ITinyRedisPipeline, ESPMode::ThreadSafe> ITinyRedisPipelinePtr;

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

	// Redis pipelining
	virtual ITinyRedisPipelinePtr CreatePipeline();

public:
	//
	// String Get/Set
	//  [Sync API]
	//     - Get/Set       - general get/set short string,integer,float,...
	//     - GetStr/SetStr - get/set UTF8 string (override to implement it)
	//     - GetBin/SetBin - get/set Binary Data (override to implement it)

	// ~ Sync APIs
	template <typename ValueType>
	ValueType Get(const FString& Key, FString* ErrorMsg = nullptr);
	template <typename ValueType>
	FRedisReply Set(const FString& Key, const ValueType& Value);

	virtual FRedisReply GetStr(const FString& Key);
	virtual FRedisReply SetStr(const FString& Key, const FString& Value);

	virtual FRedisReply GetBin(const FString& Key);
	virtual FRedisReply SetBin(const FString& Key, TArrayView<const uint8> Array);

	// ~ Async APIs
	TFuture<FRedisReply> AsyncGet(const FString& Key);
	template <typename ValueType>
	TFuture<FRedisReply> AsyncSet(const FString& Key, const ValueType& Value);

	virtual TFuture<FRedisReply> AsyncGetStr(const FString& Key);
	virtual TFuture<FRedisReply> AsyncSetStr(const FString& Key, const FString& Value);

	virtual TFuture<FRedisReply> AsyncGetBin(const FString& Key);
	virtual TFuture<FRedisReply> AsyncSetBin(const FString& Key, const TArray<uint8>& Array);

public:
	//
	// Hash APIs (TODO: add by requirements)
	// 

	// ~ Sync APIs
	template <typename ValueType>
	ValueType HashGet(const FString& Key, const FString& Field, FString* ErrorMsg = nullptr);
	template <typename ValueType>
	FRedisReply HashSet(const FString& Key, const FString& Field, const ValueType& Value);


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


template <typename ValueType>
ValueType ITinyRedisInterface::Get(const FString& Key, FString* ErrorMsg)
{
	ValueType Value;
	FString Command = FString::Printf(TEXT("GET %s"), *Key);
	FRedisReply Reply = ExecCommand(Command, ERedisCommandType::GET);
	if (!Reply.HasError())
	{
		LexFromString(Value, *Reply.String);
	}
	else
	{
		if (ErrorMsg) *ErrorMsg = Reply.Error;
	}
	return Value;
}

template <typename ValueType>
FRedisReply ITinyRedisInterface::Set(const FString& Key, const ValueType& Value)
{
	FString Command = FString::Printf(TEXT("SET %s %s"), *Key, *LexToString(Value));
	return ExecCommand(Command, ERedisCommandType::SET);
}

template <typename ValueType>
TFuture<FRedisReply> ITinyRedisInterface::AsyncSet(const FString& Key, const ValueType& Value)
{
	FString Command = FString::Printf(TEXT("SET %s %s"), *Key, *LexToString(Value));
	return AsyncExecCommand(Command, ERedisCommandType::SET);
}

template <typename ValueType>
ValueType ITinyRedisInterface::HashGet(const FString& Key, const FString& Field, FString* ErrorMsg)
{
	ValueType Value;
	FString Command = FString::Printf(TEXT("HGET %s %s"), *Key, *Field);
	FRedisReply Reply = ExecCommand(Command, ERedisCommandType::GET);
	if (!Reply.HasError())
	{
		LexFromString(Value, *Reply.String);
	}
	else
	{
		if (ErrorMsg) *ErrorMsg = Reply.Error;
	}
	return Value;
}

template <typename ValueType>
FRedisReply ITinyRedisInterface::HashSet(const FString& Key, const FString& Field, const ValueType& Value)
{
	FString Command = FString::Printf(TEXT("HSET %s %s %s"), *Key, *Field, *LexToString(Value));
	return ExecCommand(Command, ERedisCommandType::SET);
}
