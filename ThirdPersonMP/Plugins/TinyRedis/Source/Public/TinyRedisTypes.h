#pragma once

#include "CoreMinimal.h"
#include "TinyRedisTypes.generated.h"

class FRedisConnection;

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
 * Redis Command Type (for parsing result)
 */
UENUM(BlueprintType)
enum class ERedisCommandType : uint8
{
	UNKNOWN,

	//
	// String commands
	//

	/** String - GET */
	SET,
	/** Set utf8 string as value */
	SET_UTF8,
	/** Binary array as value */
	SET_BIN,

	/** String - SET */
	GET,
	/** Get utf8 string */
	GET_UTF8,
	/** Get binary value */
	GET_BIN,

	//
	// Hashes commands
	// 
	HGETALL,

	HSET,
	HSET_UTF8,
	HSET_BIN,
	HGET,
	HGET_UTF8,
	HGET_BIN,

	HMGET,
	HMSET,
};

/**
 * Redis Command's Replay 
 */
USTRUCT(BlueprintType)
struct TINYREDIS_API FRedisReply
{
	GENERATED_BODY()

	FRedisReply()
	{
		Type = ERedisReplyType::Nil;
		CommandType = ERedisCommandType::UNKNOWN;
		Integer = 0;
	}

	FRedisReply(const ERedisReplyType InType, const FString& InErrorMsg)
		: Type(InType), Error(InErrorMsg)
	{
		CommandType = ERedisCommandType::UNKNOWN;
		Integer = 0;
	}

	bool HasError() const
	{
		return Type == ERedisReplyType::Nil || Error.Len() > 0;
	}

	/** Parse reply from redis server */
	void ParseReply(const struct redisReply* Reply, ERedisCommandType InCommandType /*= ERedisCommandType::UNKNOWN*/);

	template <typename ValueType>
	ValueType GetStringAs()
	{
		ValueType Value;
		LexFromString(Value, *String);
		return Value;
	}

	FString ToDebugString() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERedisCommandType CommandType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERedisReplyType Type;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Integer;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString String;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<uint8> BinArray;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Status;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FString> Elements;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Error;

	// ---------------As Hash Query Reply -----------
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FString, FString> FieldValues;
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


/**
 * Redis Pipeline Command's replies 
 */
USTRUCT(BlueprintType)
struct TINYREDIS_API FRedisPipelineReply
{
	GENERATED_BODY()

	FRedisPipelineReply()
	{
	}

	FRedisPipelineReply(const FString& InErrorMsg)
		: Error(InErrorMsg)
	{
	}

	bool HasError() const
	{
		return Error.Len() > 0;
	}

	FString ToDebugString() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FRedisReply> Replies;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString Error;
};
