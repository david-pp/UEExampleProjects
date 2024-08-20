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
		RedisConnection->ExecCommand(InCommand, Reply, Reply.Error, InCommandType);
		// put the connection back to the pool
		ReleaseRedisConnection(RedisConnection);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}

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
	FRedisConnectionPtr RedisConnection = AcquireRedisConnection();
	if (RedisConnection)
	{
		RedisConnection->ExecCommandEx(Reply, Reply.Error, ERedisCommandType::SET_UTF8, "SET %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_UTF8(*Value));
		ReleaseRedisConnection(RedisConnection);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}
	return Reply;
}

FRedisReply FAsyncRedis::SetBin(const FString& Key, TArrayView<const uint8> Array)
{
	FRedisReply Reply;
	FRedisConnectionPtr RedisConnection = AcquireRedisConnection();
	if (RedisConnection)
	{
		RedisConnection->ExecCommandEx(Reply, Reply.Error, ERedisCommandType::SET_BIN, "SET %s %b", TCHAR_TO_ANSI(*Key), (char*)Array.GetData(), (size_t)Array.Num());
		ReleaseRedisConnection(RedisConnection);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}
	return Reply;
}
