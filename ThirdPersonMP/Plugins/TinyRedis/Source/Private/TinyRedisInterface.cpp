#include "TinyRedisInterface.h"

bool ITinyRedisInterface::AsyncExecCommand(const FString& InCommand, const FOnRedisReplyDelegate& OnReply)
{
	AsyncExecCommand(InCommand).Then([OnReply](TFuture<FRedisReply> Future)
	{
		if (Future.IsReady())
		{
			OnReply.ExecuteIfBound(Future.Get());
		}
	});

	return true;
}
