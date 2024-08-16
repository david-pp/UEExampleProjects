#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RedisTypes.h"
#include "Async/Future.h"
#include "RedisInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class URedisInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class REDISCLIENT_API IRedisInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FRedisReply ExecCommand(const FString& InCommand) = 0;
	virtual TFuture<FRedisReply> AsyncExecCommand(const FString& InCommand) = 0;

	UFUNCTION(BlueprintCallable, BlueprintCosmetic, Category=Redis)
	virtual bool AsyncExecCommand(const FString& InCommand, const FOnRedisReplyDelegate& OnReply);
};

typedef TSharedPtr<IRedisInterface, ESPMode::ThreadSafe> IRedisInterfacePtr;
