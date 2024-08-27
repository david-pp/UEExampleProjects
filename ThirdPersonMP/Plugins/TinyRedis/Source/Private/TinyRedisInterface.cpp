#include "TinyRedisInterface.h"

#include "TinyRedisModule.h"

FRedisReply ITinyRedisInterface::ExecCommand(const FString& InCommand, ERedisCommandType InCommandType)
{
	auto Command = MakeShared<FTinyRedisCommand>(InCommand, InCommandType);
	return ExecCommand(Command);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncExecCommand(const FString& InCommand, ERedisCommandType InCommandType)
{
	auto Command = MakeShared<FTinyRedisCommand>(InCommand, InCommandType);
	return AsyncExecCommand(Command);
}

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

int32 ITinyRedisInterface::DeleteKey(const FString& Key)
{
	if (Key.IsEmpty()) return 0;
	FString Command = FString::Printf(TEXT("DEL %s"), *Key);
	FRedisReply Reply = ExecCommand(Command);
	return Reply.Integer;
}

int32 ITinyRedisInterface::DeleteKeys(const TArray<FString>& Keys)
{
	if (Keys.Num() == 0) return 0;
	FString Command = FString::Printf(TEXT("DEL %s"), *FString::Join(Keys, TEXT(" ")));
	FRedisReply Reply = ExecCommand(Command);
	return Reply.Integer;
}

TArray<FString> ITinyRedisInterface::GetKeys(const FString& Pattern)
{
	if (Pattern.IsEmpty())
	{
		TArray<FString> Empty;
		return MoveTemp(Empty);
	}

	FString Command = FString::Printf(TEXT("KEYS %s"), *Pattern);
	FRedisReply Reply = ExecCommand(Command);
	return MoveTemp(Reply.Elements);
}

ITinyRedisPipelinePtr ITinyRedisInterface::CreatePipeline()
{
	UE_LOG(LogRedis, Warning, TEXT("Pipelining is not supported"))
	return nullptr;
}

FRedisReply ITinyRedisInterface::GetStr(const FString& Key)
{
	return Command<FTinyRedisCommand_Get>(Key, ERedisCommandType::GET_UTF8);
}

FRedisReply ITinyRedisInterface::GetBin(const FString& Key)
{
	return Command<FTinyRedisCommand_Get>(Key, ERedisCommandType::GET_BIN);
}

FRedisReply ITinyRedisInterface::SetStr(const FString& Key, const FString& Value)
{
	return Command<FTinyRedisCommand_Set>(Key, Value, ERedisCommandType::SET_UTF8);
}

FRedisReply ITinyRedisInterface::SetBin(const FString& Key, const TArray<uint8>& Array)
{
	return Command<FTinyRedisCommand_Set>(Key, Array);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncGet(const FString& Key)
{
	return AsyncCommand<FTinyRedisCommand_Get>(Key, ERedisCommandType::GET);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncGetStr(const FString& Key)
{
	return AsyncCommand<FTinyRedisCommand_Get>(Key, ERedisCommandType::GET_UTF8);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncSetStr(const FString& Key, const FString& Value)
{
	return AsyncCommand<FTinyRedisCommand_Set>(Key, Value, ERedisCommandType::SET_UTF8);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncGetBin(const FString& Key)
{
	return AsyncCommand<FTinyRedisCommand_Get>(Key, ERedisCommandType::GET_BIN);
}

TFuture<FRedisReply> ITinyRedisInterface::AsyncSetBin(const FString& Key, const TArray<uint8>& Array)
{
	return AsyncCommand<FTinyRedisCommand_Set>(Key, Array);
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
