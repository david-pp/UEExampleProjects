#include "RedisClientPrivate.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY(LogRedis);

#define LOCTEXT_NAMESPACE "FRedisClientModule"

class FRedisClientModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override
	{
	}
	
	virtual void ShutdownModule() override
	{
	}
};

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRedisClientModule, RedisClient)
