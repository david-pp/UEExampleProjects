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

FRedisReply FAsyncRedis::ExecCommand(const FString& InCommand, ERedisCommandType InCommandType)
{
	FRedisReply Reply;

	// Acquire a connection and execute the command
	FRedisConnectionPtr RedisConnection = AcquireRedisConnection();
	if (RedisConnection)
	{
		RedisConnection->ExecCommandEx(InCommand, Reply, Reply.Error);
		Reply.ParseReplyByCommand(InCommandType);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}

	// put the connection back to the pool
	ReleaseRedisConnection(RedisConnection);
	return Reply;
}

TFuture<FRedisReply> FAsyncRedis::AsyncExecCommand(const FString& InCommand, ERedisCommandType InCommandType)
{
	TPromise<FRedisReply> Promise;
	TFuture<FRedisReply> Future = Promise.GetFuture();

	FAsyncRedisCommand* RedisCommand = new FAsyncRedisCommand(this, InCommand, InCommandType, MoveTemp(Promise));
	ThreadPool->AddQueuedWork(RedisCommand);

	return MoveTemp(Future);
}

FRedisReply FAsyncRedis::SetStr(const FString& Key, const FString& Value)
{
	FRedisReply Reply;

	// Acquire a connection and execute the command
	FRedisConnectionPtr RedisConnection = AcquireRedisConnection();
	if (RedisConnection)
	{
		RedisConnection->ExecCommandEx(Reply, Reply.Error, "SET %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*Value));
		Reply.ParseReplyByCommand(ERedisCommandType::SET);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}

	// put the connection back to the pool
	ReleaseRedisConnection(RedisConnection);
	return Reply;
}
