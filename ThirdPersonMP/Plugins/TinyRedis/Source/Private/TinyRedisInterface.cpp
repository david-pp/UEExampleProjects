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

FRedisReply ITinyRedisInterface::GetStr(const FString& Key)
{
	FString Command = FString::Printf(TEXT("GET %s"), *Key);
	return ExecCommand(Command, ERedisCommandType::GET_UTF8);
}

FRedisReply ITinyRedisInterface::GetBin(const FString& Key)
{
	FString Command = FString::Printf(TEXT("GET %s"), *Key);
	return ExecCommand(Command, ERedisCommandType::GET_BIN);
}

FRedisReply ITinyRedisInterface::SetStr(const FString& Key, const FString& Value)
{
	return FRedisReply(ERedisReplyType::Nil, TEXT("not implement"));
}

FRedisReply ITinyRedisInterface::SetBin(const FString& Key, TArrayView<const uint8> Array)
{
	return FRedisReply(ERedisReplyType::Nil, TEXT("not implement"));
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncGet(const FString& Key)
{
	FString Command = FString::Printf(TEXT("GET %s"), *Key);
	return AsyncExecCommand(Command, ERedisCommandType::GET);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncGetStr(const FString& Key)
{
	FString Command = FString::Printf(TEXT("GET %s"), *Key);
	return AsyncExecCommand(Command, ERedisCommandType::GET_UTF8);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncSetStr(const FString& Key, const FString& Value)
{
	TPromise<FRedisReply> Promise;
	TFuture<FRedisReply> Future = Promise.GetFuture();
	Promise.SetValue(FRedisReply(ERedisReplyType::Nil, TEXT("not implement")));
	return MoveTemp(Future);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncGetBin(const FString& Key)
{
	FString Command = FString::Printf(TEXT("GET %s"), *Key);
	return AsyncExecCommand(Command, ERedisCommandType::GET_BIN);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncSetBin(const FString& Key, const TArray<uint8>& Array)
{
	TPromise<FRedisReply> Promise;
	TFuture<FRedisReply> Future = Promise.GetFuture();
	Promise.SetValue(FRedisReply(ERedisReplyType::Nil, TEXT("not implement")));
	return MoveTemp(Future);
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
