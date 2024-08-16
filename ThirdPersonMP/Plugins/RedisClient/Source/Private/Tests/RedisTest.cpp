#include "CoreMinimal.h"
#include "RedisClientModule.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_Async, "Redis.Async", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_Async::RunTest(const FString& Param)
{
	IRedisInterfacePtr Redis = IRedisClientModule::GetRedis();
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
