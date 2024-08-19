#include "TinyRedisModule.h"

#include "AsyncRedis.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogRedis);

#define LOCTEXT_NAMESPACE "TinyRedis"

class FTinyRedisModule : public ITinyRedisModule
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

	virtual IRedisInterfacePtr CreateRedisInterface(const FString& InIP, int InPort, const FString& InPassword, int InPoolSize) const override
	{
		return MakeShared<FAsyncRedis, ESPMode::ThreadSafe>(InIP, InPort, InPassword, InPoolSize);
	}

protected:
	IRedisInterfacePtr Redis;
};

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTinyRedisModule, TinyRedis)
