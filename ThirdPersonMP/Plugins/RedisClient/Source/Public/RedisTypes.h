#pragma once

#include "CoreMinimal.h"
#include "RedisTypes.generated.h"

/**
 * Redis Reply Type
 */
UENUM(BlueprintType)
enum class ERedisReplyType : uint8
{
	Nil,
	Integer,
	String,
	Array,
	Status,
};

/**
 * Redis Command's Replay 
 */
USTRUCT(BlueprintType)
struct REDISCLIENT_API FRedisReply
{
	GENERATED_BODY()

	FRedisReply()
	{
		Type = ERedisReplyType::Nil;
		Integer = 0;
	}

	bool HasError() const
	{
		return Type == ERedisReplyType::Nil || Error.Len() > 0;
	}

	void ParserReply(const struct redisReply* Reply);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERedisReplyType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString String;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> Elements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Error;
};

/**
 * Native delegates for C++
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FNativeOnRedisReply, const FRedisReply& Reply);
typedef FNativeOnRedisReply::FDelegate FNativeOnRedisReplyDelegate;

/**
 * Dynamic delegates for blueprint
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRedisReply, const FRedisReply&, Reply);

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnRedisReplyDelegate, const FRedisReply&, Reply);
