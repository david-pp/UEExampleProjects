#pragma once

#include "CoreMinimal.h"
#include "TinyRedisTypes.h"
#include "Misc/IQueuedWork.h"

class FAsyncRedis;

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

	bool IsValid() const;

protected:
	FAsyncRedis* AsyncRedis;
	FString Command;
	ERedisCommandType CommandType = ERedisCommandType::UNKNOWN;
	FNativeOnRedisReplyDelegate OnReply;
	TPromise<FRedisReply> Promise;
	bool bDebugReply = true;
};
