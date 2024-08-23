#pragma once
#include "TinyRedisCommand.h"

/**
 * Redis Pipeline :
 * - Redis pipelining is a technique for improving performance by issuing multiple commands at once
 *   without waiting for the response to each individual command. 
 * - Read More: https://redis.io/docs/latest/develop/use/pipelining/
 */
class TINYREDIS_API ITinyRedisPipeline
{
public:
	virtual ~ITinyRedisPipeline() = default;

	virtual void Start() = 0;
	virtual void AppendCommand(ITinyRedisCommandPtr Command, const FNativeOnRedisReplyDelegate& OnReply = FNativeOnRedisReplyDelegate()) = 0;
	virtual FRedisPipelineReply Commit() = 0;
	virtual TFuture<FRedisPipelineReply> AsyncCommit() = 0;

	template <typename CommandType, typename... InArgTypes>
	TSharedPtr<CommandType> Command(InArgTypes&&... Args)
	{
		auto Cmd = MakeShared<CommandType>(Args...);
		AppendCommand(Cmd);
		return Cmd;
	}
};

typedef TSharedPtr<ITinyRedisPipeline, ESPMode::ThreadSafe> ITinyRedisPipelinePtr;
