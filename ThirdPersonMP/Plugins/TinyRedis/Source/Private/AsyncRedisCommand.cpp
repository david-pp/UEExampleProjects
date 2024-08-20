#include "AsyncRedisCommand.h"

#include "AsyncRedis.h"
#include "RedisConnection.h"
#include "TinyRedisModule.h"

FAsyncRedisCommand::FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, ERedisCommandType InCommandType, const FNativeOnRedisReplyDelegate& InReplyDelegate)
	: AsyncRedis(InAsyncRedis), Command(InCommand), CommandType(InCommandType), OnReply(InReplyDelegate)
{
}

FAsyncRedisCommand::FAsyncRedisCommand(FAsyncRedis* InAsyncRedis, const FString& InCommand, ERedisCommandType InCommandType, TPromise<FRedisReply>&& Promise)
	: AsyncRedis(InAsyncRedis), Command(InCommand), CommandType(InCommandType), Promise(MoveTemp(Promise))
{
}

bool FAsyncRedisCommand::IsValid() const
{
	return AsyncRedis && Command.Len() > 0;
}


void FAsyncRedisCommand::DoThreadedWork()
{
	FRedisReply Reply;

	if (!IsValid())
	{
		Reply.Error = TEXT("Invalid AsyncRedisCommand");
		Promise.SetValue(Reply);
		delete this;
		return;
	}

	// Acquire a connection and execute the command
	FRedisConnectionPtr RedisConnection = AsyncRedis->AcquireRedisConnection();
	if (RedisConnection)
	{
		ExecRedisCommand(RedisConnection, Reply);
		// RedisConnection->ExecCommand(Command, Reply, Reply.Error, CommandType);
	}
	else
	{
		Reply.Error = TEXT("can't acquire an invalid redis connection");
	}

	// dispatch the reply to the game thread's callback
	AsyncTask(ENamedThreads::GameThread, [this, Reply]
	{
		if (bDebugReply)
		{
			UE_LOG(LogRedis, Verbose, TEXT("%s -> %s"), *Command, *Reply.ToDebugString());
		}

		OnReply.ExecuteIfBound(Reply);
		Promise.SetValue(Reply);
		delete this;
	});

	// put the connection back to the pool
	if (RedisConnection)
	{
		AsyncRedis->ReleaseRedisConnection(RedisConnection);
	}
}

void FAsyncRedisCommand::Abandon()
{
	delete this;
}

void FAsyncRedisCommand::ExecRedisCommand(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply)
{
	if (Connection)
	{
		Connection->ExecCommand(Command, Reply, Reply.Error, CommandType);
	}
}

// ---------------------


void FAsyncRedisCommand_SetStr::ExecRedisCommand(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply)
{
	if (Connection)
	{
		Connection->ExecCommandEx(Reply, Reply.Error, ERedisCommandType::SET_UTF8, "SET %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_UTF8(*Value));
	}
}


void FAsyncRedisCommand_SetBin::ExecRedisCommand(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply)
{
	if (Connection)
	{
		Connection->ExecCommandEx(Reply, Reply.Error, ERedisCommandType::SET_BIN, "SET %s %b", TCHAR_TO_ANSI(*Key), (char*)Array.GetData(), (size_t)Array.Num());
	}
}
