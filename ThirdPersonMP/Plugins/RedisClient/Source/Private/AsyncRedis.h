#pragma once
#include "RedisClient.h"
#include "RedisInterface.h"

class FAsyncRedis : public IRedisInterface
{
public:
	/**
	 * 
	 * @param InIP  - Redis server IP
	 * @param InPort - Redis server Port
	 * @param InPassword - Redis server Password
	 * @param InPoolSize  - Thread pool thread number & redis client number
	 */
	FAsyncRedis(const FString& InIP = TEXT("127.0.0.1"), int InPort = 6379, const FString& InPassword = TEXT(""), int InPoolSize = 2);

	virtual ~FAsyncRedis() override;

	// Acquire/Release reusable redis connection
	FRedisClientPtr AcquireRedisConnection();
	void ReleaseRedisConnection(FRedisClientPtr RedisClient);

public:
	virtual FRedisReply ExecCommand(const FString& InCommand) override;
	virtual TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand) override;

protected:
	// Redis server info
	FString IP;
	int Port;
	FString Password;

	/** Redis client thread pool */
	class FQueuedThreadPool* ThreadPool;

	/** Redis connection */
	FCriticalSection Mutex;
	TArray<FRedisClientPtr> RedisConnections;
};
