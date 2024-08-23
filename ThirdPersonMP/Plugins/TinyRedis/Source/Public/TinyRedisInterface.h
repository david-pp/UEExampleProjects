#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TinyRedisTypes.h"
#include "TinyRedisCommand.h"
#include "TinyRedisPipeline.h"
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
	/**
	 * Override to implement Sync/Async/Pipeline APIs 
	 */
	virtual FRedisReply ExecCommand(ITinyRedisCommandPtr Command) = 0;
	virtual TFuture<FRedisReply> AsyncExecCommand(ITinyRedisCommandPtr Command) = 0;
	virtual ITinyRedisPipelinePtr CreatePipeline();

public:
	// Execute Redis Command
	template <typename CommandType, typename... InArgTypes>
	FRedisReply Command(InArgTypes&&... Args)
	{
		auto Cmd = MakeShared<CommandType>(Args...);
		return ExecCommand(Cmd);
	}

	// Async Execute Redis Command
	template <typename CommandType, typename... InArgTypes>
	TFuture<FRedisReply> AsyncCommand(InArgTypes&&... Args)
	{
		auto Cmd = MakeShared<CommandType>(Args...);
		return AsyncExecCommand(Cmd);
	}

	// Simple Command
	FRedisReply ExecCommand(const FString& InCommand, ERedisCommandType InCommandType = ERedisCommandType::UNKNOWN);
	// Simple Async Command
	TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand, ERedisCommandType InCommandType = ERedisCommandType::UNKNOWN);
	// Async API for Blueprint
	UFUNCTION(BlueprintCallable, Category=TinyRedis)
	virtual bool AsyncExecCommand(const FString& InCommand, const FOnRedisReplyDelegate& OnReply);

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

	FRedisReply GetStr(const FString& Key);
	FRedisReply SetStr(const FString& Key, const FString& Value);

	FRedisReply GetBin(const FString& Key);
	FRedisReply SetBin(const FString& Key, const TArray<uint8>& Array);

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
	ValueType Value = ValueType();
	FRedisReply Reply = Command<FTinyRedisCommand_Get>(Key, ERedisCommandType::GET);
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
	return Command<FTinyRedisCommand_Set>(Key, LexToString(Value), ERedisCommandType::SET);
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
