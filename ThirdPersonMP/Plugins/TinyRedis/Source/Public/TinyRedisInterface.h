#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TinyRedisTypes.h"
#include "Async/Future.h"
#include "TinyRedisInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UTinyRedisInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Redis APIs
 */
class TINYREDIS_API ITinyRedisInterface
{
	GENERATED_BODY()

public:
	// Sync API
	virtual FRedisReply ExecCommand(const FString& InCommand) = 0;
	// Async API for Native
	virtual TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand) = 0;
	// Async API for Blueprint
	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category=Redis)
	virtual bool AsyncExecCommand(const FString& InCommand, const FOnRedisReplyDelegate& OnReply);

public:
};

typedef TSharedPtr<ITinyRedisInterface, ESPMode::ThreadSafe> IRedisInterfacePtr;
