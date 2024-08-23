#include "AsyncRedisTask.h"

#include "AsyncRedis.h"
#include "RedisConnection.h"
#include "TinyRedisModule.h"

FRedisCommandAsyncTask::FRedisCommandAsyncTask(FAsyncRedis* InAsyncRedis, ITinyRedisCommandPtr InCommand, TPromise<FRedisReply>&& InPromise)
	: AsyncRedis(InAsyncRedis), Command(InCommand), Promise(MoveTemp(InPromise))
{
}

FRedisCommandAsyncTask::FRedisCommandAsyncTask(FAsyncRedis* InAsyncRedis, ITinyRedisCommandPtr InCommand, const FNativeOnRedisReplyDelegate& InReplyDelegate)
	: AsyncRedis(InAsyncRedis), Command(InCommand), OnReplyDelegate(InReplyDelegate)
{
}

bool FRedisCommandAsyncTask::IsValid() const
{
	return AsyncRedis && Command;
}

void FRedisCommandAsyncTask::DoThreadedWork()
{
	FRedisReply Reply;

	if (!IsValid())
	{
		Reply.Error = TEXT("Invalid RedisCommandAsyncTask");
		Promise.SetValue(Reply);
		delete this;
		return;
	}

	// Acquire a connection and execute the command
	FRedisConnectionPtr RedisConnection = AsyncRedis->AcquireRedisConnection();
	if (RedisConnection)
	{
		Command->Exec(RedisConnection, Reply);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}

	// dispatch the reply to the game thread's callback
	AsyncTask(ENamedThreads::GameThread, [this, Reply]
	{
		UE_LOG(LogRedis, Verbose, TEXT("%s -> %s"), *Command->ToDebugString(), *Reply.ToDebugString());
		Command->OnReply.ExecuteIfBound(Reply);
		Promise.SetValue(Reply);
		OnReplyDelegate.ExecuteIfBound(Reply);
		delete this;
	});

	// put the connection back to the pool
	if (RedisConnection)
	{
		AsyncRedis->ReleaseRedisConnection(RedisConnection);
	}
}

void FRedisCommandAsyncTask::Abandon()
{
	delete this;
}
