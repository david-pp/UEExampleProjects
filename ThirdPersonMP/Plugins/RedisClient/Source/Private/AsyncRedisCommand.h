#pragma once

#include "CoreMinimal.h"
#include "RedisTypes.h"
#include "Misc/IQueuedWork.h"

class FAsyncRedis;

/**
 * Redis Command as ThreadPool's Task 
 */
class FAsyncRedisCommand : public IQueuedWork
{
public:
	FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, const FNativeOnRedisReplyDelegate& InReplyDelegate = FNativeOnRedisReplyDelegate());

	FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, TPromise<FRedisReply>&& Promise);

	// ~Begin interface IQueuedWork
	virtual void DoThreadedWork() override;
	virtual void Abandon() override;
	// ~End Interface IQueuedWork

	bool IsValid() const;

protected:
	FAsyncRedis* AsyncRedis;
	FString Command;
	FNativeOnRedisReplyDelegate OnReply;
	TPromise<FRedisReply> Promise;
};
