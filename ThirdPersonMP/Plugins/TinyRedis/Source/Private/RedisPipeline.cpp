#include "RedisPipeline.h"

#include "AsyncRedis.h"
#include "AsyncRedisCommand.h"
#include "RedisConnection.h"

FRedisPipeline::FRedisPipeline(FAsyncRedis* InRedis) : Redis(InRedis)
{
	bHasStarted = false;
}

FRedisPipeline::~FRedisPipeline()
{
	Commands.Empty();
}

void FRedisPipeline::Start()
{
	bHasStarted = true;
	Commands.Empty();
}

void FRedisPipeline::AppendCommand(ITinyRedisCommandPtr Command, const FNativeOnRedisReplyDelegate& OnReply)
{
	if (Command)
	{
		Command->OnReply = OnReply;
		Commands.Add(Command);
	}
}

FRedisPipelineReply FRedisPipeline::Commit()
{
	FRedisPipelineReply PipelineReply;
	if (!bHasStarted)
	{
		PipelineReply.Error = TEXT("Pipeline hasn't started");
		return PipelineReply;
	}

	FRedisConnectionPtr RedisConnection = Redis->AcquireRedisConnection();
	if (!RedisConnection)
	{
		PipelineReply.Error = TEXT("can't acquire an invalid redis connection");
		return PipelineReply;
	}

	TArray<ITinyRedisCommandPtr> PipedCommands;
	for (ITinyRedisCommandPtr Cmd : Commands)
	{
		if (Cmd && Cmd->AppendPipeline(RedisConnection))
		{
			PipedCommands.Add(Cmd);
		}
	}

	for (ITinyRedisCommandPtr Cmd : PipedCommands)
	{
		FRedisReply Reply;
		RedisConnection->GetPipelineCommandReply(Cmd->GetCommandType(), Reply);
		Cmd->OnReply.ExecuteIfBound(Reply);
		PipelineReply.Replies.Add(Reply);
	}

	Redis->ReleaseRedisConnection(RedisConnection);
	return MoveTemp(PipelineReply);
}

TFuture<FRedisPipelineReply> FRedisPipeline::AsyncCommit()
{
	TPromise<FRedisPipelineReply> Promise;
	TFuture<FRedisPipelineReply> Future = Promise.GetFuture();

	if (bHasStarted && Redis)
	{
		Redis->DispatchCommandTask(new FRedisPipelineAsyncTask(AsShared(), MoveTemp(Promise)));
	}
	else
	{
		Promise.SetValue(FRedisPipelineReply(TEXT("Pipeline hasn't started")));
	}

	return MoveTemp(Future);
}

// --------------------------

FRedisPipelineAsyncTask::FRedisPipelineAsyncTask(TSharedPtr<FRedisPipeline, ESPMode::ThreadSafe> InPipeline, TPromise<FRedisPipelineReply>&& InPromise)
	: Pipeline(InPipeline), Promise(MoveTemp(InPromise))
{
}

void FRedisPipelineAsyncTask::DoThreadedWork()
{
	if (Pipeline && Pipeline->Redis)
	{
		TArray<ITinyRedisCommandPtr> PipedCommands;
		FRedisPipelineReply PipelineReply;

		// Acquire a connection and execute the pipeline command
		FRedisConnectionPtr RedisConnection = Pipeline->Redis->AcquireRedisConnection();
		if (RedisConnection)
		{
			for (ITinyRedisCommandPtr Cmd : Pipeline->Commands)
			{
				if (Cmd && Cmd->AppendPipeline(RedisConnection))
				{
					PipedCommands.Add(Cmd);
				}
			}

			for (ITinyRedisCommandPtr Cmd : PipedCommands)
			{
				FRedisReply Reply;
				RedisConnection->GetPipelineCommandReply(Cmd->GetCommandType(), Reply);
				PipelineReply.Replies.Add(Reply);
			}
		}
		else
		{
			PipelineReply.Error = TEXT("can't acquire an invalid redis connection");
		}

		// dispatch the reply to the game thread's callback
		AsyncTask(ENamedThreads::GameThread, [this, PipelineReply, PipedCommands]
		{
			for (int I = 0; I < PipedCommands.Num(); ++I)
			{
				ITinyRedisCommandPtr Cmd = PipedCommands[I];
				if (PipelineReply.Replies.IsValidIndex(I))
				{
					Cmd->OnReply.ExecuteIfBound(PipelineReply.Replies[I]);
				}
			}
			Promise.SetValue(PipelineReply);
			delete this;
		});

		// put the connection back to the pool
		if (RedisConnection)
		{
			Pipeline->Redis->ReleaseRedisConnection(RedisConnection);
		}
	}
	else
	{
		Promise.SetValue(FRedisPipelineReply(TEXT("Invalid RedisPipeline Async Task")));
		delete this;
	}
}

void FRedisPipelineAsyncTask::Abandon()
{
	delete this;
}
