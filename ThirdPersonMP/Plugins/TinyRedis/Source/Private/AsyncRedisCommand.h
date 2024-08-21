#pragma once

#include "CoreMinimal.h"
#include "TinyRedisInterface.h"
#include "TinyRedisTypes.h"
#include "Misc/IQueuedWork.h"

class FRedisConnection;
class FAsyncRedis;

class FTinyRedisCommand_HSet : public ITinyRedisCommand
{
public:
	virtual void ExecCommand(TSharedPtr<ITinyRedisConnection> Connection, FRedisReply& Reply) override
	{
		// Set Binary : SET %s %b 
		// Set UTF8 : SET %s %s
		// Set String : SET %s %s
	}
	virtual void ExecAppendCommand(TSharedPtr<ITinyRedisConnection> Connection) override
	{
		// Set Binary : SET %s %b 
		// Set UTF8 : SET %s %s
		// Set String : SET %s %s
	}
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
