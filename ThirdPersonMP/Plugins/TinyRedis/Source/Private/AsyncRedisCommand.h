#pragma once

#include "CoreMinimal.h"
#include "RedisConnection.h"
#include "RedisPipeline.h"
#include "TinyRedisInterface.h"
#include "TinyRedisTypes.h"
#include "Misc/IQueuedWork.h"

class FRedisConnection;
class FAsyncRedis;

/**
 * Redis General Command
 */
class FTinyRedisCommand : public ITinyRedisCommand
{
public:
	FTinyRedisCommand(const FString& InCommand = TEXT("")) : Command(InCommand)
	{
	}

	virtual FString ToDebugString() const override
	{
		return Command;
	}

	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override
	{
		return Connection->ExecCommandEx(Reply, Reply.Error, GetCommandType(), TCHAR_TO_ANSI(*Command));
	}

	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) override
	{
		return Connection->AppendPipelineCommand(GetCommandType(), TCHAR_TO_ANSI(*Command));
	}

protected:
	FString Command;
};

class FTinyRedisCommand_HGet : public FTinyRedisCommand
{
public:
	FTinyRedisCommand_HGet(const FString& InKey, const FString& InField) : Key(InKey), Field(InField)
	{
		Command = FString::Printf(TEXT("HGET %s %s"), *InKey, *InField);
	}

protected:
	FString Key;
	FString Field;
};


class FTinyRedisCommand_HSet : public ITinyRedisCommand
{
public:
	FTinyRedisCommand_HSet(const FString& InKey, const FString& InField, const FString& InValue) : Key(InKey), Field(InField), Value(InValue)
	{
	}

	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override
	{
		// Set Binary : SET %s %b 
		// Set UTF8 : SET %s %s
		// Set String : SET %s %s
		return Connection->ExecCommandEx(Reply, Reply.Error, GetCommandType(), "HSET %s %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*Field), TCHAR_TO_ANSI(*Value));
	}

	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) override
	{
		// Set Binary : SET %s %b 
		// Set UTF8 : SET %s %s
		// Set String : SET %s %s

		return Connection->AppendPipelineCommand(GetCommandType(), "HSET %s %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*Field), TCHAR_TO_ANSI(*Value));
	}

protected:
	FString Key;
	FString Field;
	FString Value;
};


/**
 * Redis Command as ThreadPool's Task 
 */
class FAsyncRedisCommand : public IQueuedWork
{
public:
	FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, ERedisCommandType InCommandType, const FNativeOnRedisReplyDelegate& InReplyDelegate = FNativeOnRedisReplyDelegate());

	FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, ERedisCommandType InCommandType, TPromise<FRedisReply>&& Promise);

	// ~Begin interface IQueuedWork
	virtual void DoThreadedWork() override;
	virtual void Abandon() override;
	// ~End Interface IQueuedWork

	virtual void ExecRedisCommand(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply);

	bool IsValid() const;

protected:
	FAsyncRedis* AsyncRedis;
	FString Command;
	ERedisCommandType CommandType = ERedisCommandType::UNKNOWN;
	FNativeOnRedisReplyDelegate OnReply;
	TPromise<FRedisReply> Promise;
	bool bDebugReply = true;
};

/**
 * Redis Command Task 
 */
class FRedisCommandAsyncTask : public IQueuedWork
{
public:
	FRedisCommandAsyncTask(FAsyncRedis* InAsyncRedis, ITinyRedisCommandPtr InCommand, TPromise<FRedisReply>&& InPromise);

	bool IsValid() const;

	// ~Begin interface IQueuedWork
	virtual void DoThreadedWork() override;
	virtual void Abandon() override;
	// ~End Interface IQueuedWork

protected:
	FAsyncRedis* AsyncRedis = nullptr;
	ITinyRedisCommandPtr Command;
	TPromise<FRedisReply> Promise;
};


/**
 * Async Set UTF8 String
 */
class FAsyncRedisCommand_SetStr : public FAsyncRedisCommand
{
public:
	FAsyncRedisCommand_SetStr(FAsyncRedis* InAsyncRedis, const FString& InKey, const FString& InValue, TPromise<FRedisReply>&& InPromise)
		: FAsyncRedisCommand(InAsyncRedis, TEXT("SET"), ERedisCommandType::SET_UTF8, MoveTemp(InPromise)), Key(InKey), Value(InValue)
	{
	}

	virtual void ExecRedisCommand(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override;

protected:
	FString Key;
	FString Value;
};

/**
 * Async Set Binary
 */
class FAsyncRedisCommand_SetBin : public FAsyncRedisCommand
{
public:
	FAsyncRedisCommand_SetBin(FAsyncRedis* InAsyncRedis, const FString& InKey, const TArray<uint8>& InArray, TPromise<FRedisReply>&& InPromise)
		: FAsyncRedisCommand(InAsyncRedis, TEXT("SET"), ERedisCommandType::SET_UTF8, MoveTemp(InPromise)), Key(InKey), Array(InArray)
	{
	}

	virtual void ExecRedisCommand(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override;

protected:
	FString Key;
	TArray<uint8> Array;
};
