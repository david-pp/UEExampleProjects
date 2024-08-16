#include "AsyncRedisCommand.h"

#include "AsyncRedis.h"
#include "RedisConnection.h"

FAsyncRedisCommand::FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, const FNativeOnRedisReplyDelegate& InReplyDelegate)
	: AsyncRedis(InAsyncRedis), Command(InCommand), OnReply(InReplyDelegate)
{
}

FAsyncRedisCommand::FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, TPromise<FRedisReply>&& Promise)
	: AsyncRedis(InAsyncRedis), Command(InCommand), Promise(MoveTemp(Promise))
{
}

bool FAsyncRedisCommand::IsValid() const
{
	return AsyncRedis && Command.Len() > 0;
}

void FAsyncRedisCommand::DoThreadedWork()
{
	if (!IsValid())
	{
		// todo: error reply
		delete this;
		return;
	}

	FRedisReply Reply;
	// Acquire a connection and execute the command
	FRedisConnectionPtr RedisConnection = AsyncRedis->AcquireRedisConnection();
	if (RedisConnection)
	{
		RedisConnection->ExecCommandEx(Command, Reply, Reply.Error);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}

	// dispatch the reply to the game thread's callback
	AsyncTask(ENamedThreads::GameThread, [this, Reply]
	{
		OnReply.ExecuteIfBound(Reply);
		Promise.SetValue(Reply);
		delete this;
	});

	// put the connection back to the pool
	AsyncRedis->ReleaseRedisConnection(RedisConnection);
}

void FAsyncRedisCommand::Abandon()
{
	delete this;
}
