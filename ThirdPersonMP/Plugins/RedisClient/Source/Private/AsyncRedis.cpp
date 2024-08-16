#include "AsyncRedis.h"

#include "AsyncRedisCommand.h"

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

FRedisClientPtr FAsyncRedis::AcquireRedisConnection()
{
	FRedisClientPtr RedisClient;
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
	RedisClient = MakeShared<FRedisClient>();
	if (RedisClient)
	{
		if (RedisClient->ConnectToRedis(IP, Port, Password))
		{
			return RedisClient;
		}
	}

	return nullptr;
}

void FAsyncRedis::ReleaseRedisConnection(FRedisClientPtr RedisClient)
{
	// put back the connection
	FScopeLock ScopeLock(&Mutex);
	RedisConnections.Add(RedisClient);
}

FRedisReply FAsyncRedis::ExecCommand(const FString& InCommand)
{
	FRedisReply Reply;

	TFuture<FRedisReply> Future = AsyncExecCommand(InCommand);
	if (Future.WaitFor(FTimespan::FromSeconds(0.1)))
	{
		if (Future.IsReady())
		{
			return Future.Get();
		}
	}
	else
	{
		Reply.Error = TEXT("exec command timeout");
	}

	return Reply;
}

TFuture<FRedisReply> FAsyncRedis::AsyncExecCommand(const FString& InCommand)
{
	TPromise<FRedisReply> Promise;
	TFuture<FRedisReply> Future = Promise.GetFuture();

	FAsyncRedisCommand* RedisCommand = new FAsyncRedisCommand(this, InCommand, MoveTemp(Promise));
	ThreadPool->AddQueuedWork(RedisCommand);

	return MoveTemp(Future);
}
