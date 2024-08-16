#include "RedisClientModule.h"

#include "AsyncRedis.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogRedis);

#define LOCTEXT_NAMESPACE "FRedisClientModule"

class FRedisClientModule : public IRedisClientModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
		Redis = MakeShared<FAsyncRedis, ESPMode::ThreadSafe>(TEXT("127.0.0.1"), 6379);
	}

	virtual void ShutdownModule() override
	{
		Redis = nullptr;
	}

	virtual IRedisInterfacePtr GetRedisInterface() const override
	{
		return Redis;
	}

protected:
	IRedisInterfacePtr Redis;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FRedisClientModule, RedisClient)
