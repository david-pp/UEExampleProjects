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

TFuture<FRedisReply> ITinyRedisInterface::AsyncHashGetAll(const FString& InKey)
{
	FString Command = FString::Printf(TEXT("HGETALL %s"), *InKey);
	return AsyncExecCommand(Command, ERedisCommandType::HGETALL);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncHashMultiSet(const FString& InKey, const TMap<FString, FString>& FieldValuePairs)
{
	FString Command = FString::Printf(TEXT("HMSET %s"), *InKey);
	for (auto& Pair : FieldValuePairs)
	{
		Command += FString::Printf(TEXT(" %s %s"), *Pair.Key, *Pair.Value);
	}

	return AsyncExecCommand(Command, ERedisCommandType::HMSET);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncHashMultiGet(const FString& InKey, const TArray<FString>& Fields)
{
	FString Command = FString::Printf(TEXT("HMGET %s"), *InKey);
	for (auto& Field : Fields)
	{
		Command += FString::Printf(TEXT(" %s"), *Field);
	}

	return AsyncExecCommand(Command, ERedisCommandType::HMGET);
}

void ITinyRedisInterface::AsyncHashGetAll(const FString& InKey, const FOnRedisReplyDelegate& OnReply)
{
	AsyncHashGetAll(InKey).Then([OnReply](TFuture<FRedisReply> Future)
	{
		if (Future.IsReady())
		{
			OnReply.ExecuteIfBound(Future.Get());
		}
	});
}

void ITinyRedisInterface::AsyncHashMultiSet(const FString& InKey, const TMap<FString, FString>& FieldValuePairs, const FOnRedisReplyDelegate& OnReply)
{
	AsyncHashMultiSet(InKey, FieldValuePairs).Then([OnReply](TFuture<FRedisReply> Future)
	{
		if (Future.IsReady())
		{
			OnReply.ExecuteIfBound(Future.Get());
		}
	});
}

void ITinyRedisInterface::AsyncHashMultiGet(const FString& InKey, const TArray<FString>& Fields, const FOnRedisReplyDelegate& OnReply)
{
	AsyncHashMultiGet(InKey, Fields).Then([OnReply](TFuture<FRedisReply> Future)
	{
		if (Future.IsReady())
		{
			OnReply.ExecuteIfBound(Future.Get());
		}
	});
}
