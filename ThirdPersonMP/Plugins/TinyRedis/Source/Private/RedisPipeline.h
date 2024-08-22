#pragma once
#include "TinyRedisInterface.h"

class FAsyncRedis;

/**
 * Pipeline Impl
 */
class FRedisPipeline : public ITinyRedisPipeline, public TSharedFromThis<FRedisPipeline, ESPMode::ThreadSafe>
{
public:
	FRedisPipeline(FAsyncRedis* InRedis);
	virtual ~FRedisPipeline() override;

	virtual void Start() override;
	virtual void AppendCommand(ITinyRedisCommandPtr Command, const FNativeOnRedisReplyDelegate& OnReply) override;
	virtual FRedisPipelineReply Commit() override;
	virtual TFuture<FRedisPipelineReply> AsyncCommit() override;

protected:
	friend class FRedisPipelineAsyncTask;
	class FAsyncRedis* Redis = nullptr;
	bool bHasStarted = false;
	TArray<ITinyRedisCommandPtr> Commands;
};

/**
 * Pipeline Async Task
 */
class FRedisPipelineAsyncTask : public IQueuedWork
{
public:
	FRedisPipelineAsyncTask(TSharedPtr<FRedisPipeline, ESPMode::ThreadSafe> InPipeline, TPromise<FRedisPipelineReply>&& InPromise);

	// ~Begin interface IQueuedWork
	virtual void DoThreadedWork() override;
	virtual void Abandon() override;
	// ~End Interface IQueuedWork

protected:
	TSharedPtr<FRedisPipeline, ESPMode::ThreadSafe> Pipeline;
	TPromise<FRedisPipelineReply> Promise;
};
