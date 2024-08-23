#pragma once

#include "CoreMinimal.h"
#include "RedisPipeline.h"
#include "TinyRedisInterface.h"
#include "TinyRedisTypes.h"
#include "Misc/IQueuedWork.h"

class FRedisConnection;
class FAsyncRedis;

/**
 * Redis Command Task 
 */
class FRedisCommandAsyncTask : public IQueuedWork
{
public:
	FRedisCommandAsyncTask(FAsyncRedis* InAsyncRedis, ITinyRedisCommandPtr InCommand, TPromise<FRedisReply>&& InPromise);
	FRedisCommandAsyncTask(FAsyncRedis* InAsyncRedis, ITinyRedisCommandPtr InCommand, const FNativeOnRedisReplyDelegate& InReplyDelegate = FNativeOnRedisReplyDelegate());

	bool IsValid() const;

	// ~Begin interface IQueuedWork
	virtual void DoThreadedWork() override;
	virtual void Abandon() override;
	// ~End Interface IQueuedWork

protected:
	FAsyncRedis* AsyncRedis = nullptr;
	ITinyRedisCommandPtr Command;
	TPromise<FRedisReply> Promise;
	FNativeOnRedisReplyDelegate OnReplyDelegate;
};
