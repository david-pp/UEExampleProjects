#pragma once

#include "CoreMinimal.h"
#include "RedisInterface.h"

/** Declares a log category for this module. */
DECLARE_LOG_CATEGORY_EXTERN(LogRedis, Log, All);

class IRedisClientModule : public IModuleInterface
{
public:
	static IRedisClientModule* Get()
	{
		return static_cast<IRedisClientModule*>(FModuleManager::Get().LoadModule("RedisClient"));
	}

	static IRedisInterfacePtr GetRedis()
	{
		IRedisClientModule* Module = Get();
		return Module ? Module->GetRedisInterface() : nullptr;
	}

public:
	/*
     * Get default redis interface
     */
	virtual IRedisInterfacePtr GetRedisInterface() const = 0;

	/**
	 * Create a new redis client impl
	 */
	virtual IRedisInterfacePtr CreateRedisInterface(const FString& InIP, int InPort, const FString& InPassword, int InPoolSize) const = 0;
};
