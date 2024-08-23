#include "AsyncRedis.h"

#include "AsyncRedisTask.h"
#include "RedisPipeline.h"
#include "TinyRedisModule.h"

FAsyncRedis::FAsyncRedis(const FString& InIP, int InPort, const FString& InPassword, int InPoolSize)
	: IP(InIP), Port(InPort), Password(InPassword)
{
	ThreadPool = FQueuedThreadPool::Allocate();
	ThreadPool->Create(InPoolSize, 32 * 1024, TPri_Normal, TEXT("RedisPool"));
}

FAsyncRedis::~FAsyncRedis()
{
	if (ThreadPool)
	{
		ThreadPool->Destroy();
		delete ThreadPool;
		ThreadPool = nullptr;
	}
}

FRedisConnectionPtr FAsyncRedis::AcquireRedisConnection()
{
	FRedisConnectionPtr RedisClient;
	// find a reusable connection
	{
		FScopeLock ScopeLock(&Mutex);
		if (RedisConnections.Num() > 0)
		{
			RedisClient = RedisConnections.Pop();
			return RedisClient;
		}
	}

	// or create a new one
	RedisClient = MakeShared<FRedisConnection>();
	if (RedisClient)
	{
		if (RedisClient->ConnectToRedis(IP, Port, Password))
		{
			return RedisClient;
		}
	}

	return nullptr;
}

void FAsyncRedis::ReleaseRedisConnection(FRedisConnectionPtr RedisClient)
{
	// put back the connection
	FScopeLock ScopeLock(&Mutex);
	RedisConnections.Add(RedisClient);
}

void FAsyncRedis::DispatchCommandTask(IQueuedWork* CommandTask)
{
	if (ThreadPool && CommandTask)
	{
		ThreadPool->AddQueuedWork(CommandTask);
	}
}

FRedisReply FAsyncRedis::ExecCommand(ITinyRedisCommandPtr Command)
{
	FRedisReply Reply;

	// Acquire a connection and execute the command
	FRedisConnectionPtr RedisConnection = AcquireRedisConnection();
	if (RedisConnection)
	{
		if (Command)
		{
			Command->Exec(RedisConnection, Reply);
			UE_LOG(LogRedis, Verbose, TEXT("%s -> %s"), *Command->ToDebugString(), *Reply.ToDebugString());
			Command->OnReply.ExecuteIfBound(Reply);
		}

		// put the connection back to the pool
		ReleaseRedisConnection(RedisConnection);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}

	return Reply;
}

TFuture<FRedisReply> FAsyncRedis::AsyncExecCommand(ITinyRedisCommandPtr Command)
{
	TPromise<FRedisReply> Promise;
	TFuture<FRedisReply> Future = Promise.GetFuture();
	ThreadPool->AddQueuedWork(new FRedisCommandAsyncTask(this, Command, MoveTemp(Promise)));
	return MoveTemp(Future);
}

ITinyRedisPipelinePtr FAsyncRedis::CreatePipeline()
{
	return MakeShared<FRedisPipeline, ESPMode::ThreadSafe>(this);
}
