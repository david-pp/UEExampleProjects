#include "CoreMinimal.h"
#include "TinyRedis.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_Async, "Redis.Async", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_Async::RunTest(const FString& Param)
{
	IRedisInterfacePtr Redis = ITinyRedisModule::GetTinyRedis();
	if (Redis)
	{
		FRedisReply Reply = Redis->ExecCommand("hmset user:xxx name david sex male age 30");
		UE_LOG(LogRedis, Warning, TEXT("hmset - %s"), *Reply.ToDebugString());

		Redis->AsyncExecCommand("hgetall user:xxx").Then([](TFuture<FRedisReply> Future)
		{
			UE_LOG(LogRedis, Warning, TEXT("hgetall -  %s"), *Future.Get().ToDebugString());
		});
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_API, "Redis.API", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_API::RunTest(const FString& Param)
{
	IRedisInterfacePtr Redis = ITinyRedisModule::GetTinyRedis();
	if (Redis)
	{
		Redis->AsyncSet(TEXT("game:title"), TEXT("tinyredis"));
		Redis->AsyncSet(TEXT("game:price"), 200);
		// Redis->AsyncSet(TEXT("game:tinyredis:desc"), TEXT("a simple async redis client wrapper"));
		Redis->SetStr(TEXT("game:tinyredis:desc"), TEXT("a simple async redis client wrapper"));

		FPlatformProcess::Sleep(1.0); // wait for async api is done

		Redis->AsyncGet(TEXT("game:title")).Then([Redis](TFuture<FRedisReply> Future)
		{
			FString Value = Future.Get().String;
			Redis->AsyncGet(FString::Printf(TEXT("game:%s:desc"), *Value)).Then([](TFuture<FRedisReply> Future)
			{
				UE_LOG(LogRedis, Warning, TEXT("desc : %s"), *Future.Get().String);
			});
		});

		auto Future = Redis->AsyncGet(TEXT("game:price")).Then([](TFuture<FRedisReply> Future)
		{
			UE_LOG(LogRedis, Warning, TEXT("prices : %s"), *Future.Get().String);
		});

		Future.WaitFor(FTimespan::FromMilliseconds(500));
	}

	return true;
}
