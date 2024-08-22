#pragma once
#include "RedisConnection.h"
#include "TinyRedisInterface.h"

class FAsyncRedis : public ITinyRedisInterface
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
	FRedisConnectionPtr AcquireRedisConnection();
	void ReleaseRedisConnection(FRedisConnectionPtr RedisClient);

	void DispatchCommandTask(IQueuedWork* CommandTask);

public:
	virtual FRedisReply ExecCommand(ITinyRedisCommandPtr Command) override;
	virtual TFuture<FRedisReply> AsyncExecCommand(ITinyRedisCommandPtr Command) override;

	// TODO: delete
	virtual FRedisReply ExecCommand(const FString& InCommand, ERedisCommandType InCommandType) override;
	virtual TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand, ERedisCommandType InCommandType) override;

	virtual ITinyRedisPipelinePtr CreatePipeline() override;

	virtual FRedisReply SetStr(const FString& Key, const FString& Value) override;
	virtual FRedisReply SetBin(const FString& Key, TArrayView<const uint8> Array) override;
	virtual TFuture<FRedisReply> AsyncSetStr(const FString& Key, const FString& Value) override;
	virtual TFuture<FRedisReply> AsyncSetBin(const FString& Key, const TArray<uint8>& Array) override;

protected:
	// Redis server info
	FString IP;
	int Port;
	FString Password;

	/** Redis client thread pool */
	class FQueuedThreadPool* ThreadPool;

	/** Redis connection */
	FCriticalSection Mutex;
	TArray<FRedisConnectionPtr> RedisConnections;
};
