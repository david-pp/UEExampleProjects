#pragma once
#include "TinyRedisTypes.h"
#include "RedisConnection.h"

/**
 * Redis Command
 */
class TINYREDIS_API ITinyRedisCommand
{
public:
	virtual ~ITinyRedisCommand()
	{
	}

	virtual ERedisCommandType GetCommandType() const
	{
		return ERedisCommandType::UNKNOWN;
	}

	virtual FString ToDebugString() const { return TEXT("RedisCommand"); }

	// Execute redis command by connection
	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) = 0;

	// Append the command to the pipeline
	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) = 0;

	// Callback when command replied from redis server
	FNativeOnRedisReplyDelegate OnReply;
};

typedef TSharedPtr<ITinyRedisCommand> ITinyRedisCommandPtr;


/**
 * Redis General Command
 */
class FTinyRedisCommand : public ITinyRedisCommand
{
public:
	FTinyRedisCommand(const FString& InCommand = TEXT(""), ERedisCommandType InCommandType = ERedisCommandType::UNKNOWN)
		: Command(InCommand), CommandType(InCommandType)
	{
	}

	virtual ERedisCommandType GetCommandType() const override
	{
		return ERedisCommandType::UNKNOWN;
	}

	virtual FString ToDebugString() const override
	{
		return Command;
	}

	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override
	{
		return Connection->ExecCommandEx(Reply, Reply.Error, GetCommandType(), TCHAR_TO_ANSI(*Command));
	}

	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) override
	{
		return Connection->AppendPipelineCommand(GetCommandType(), TCHAR_TO_ANSI(*Command));
	}

protected:
	FString Command;
	ERedisCommandType CommandType;
};

/**
 * Redis : Set Key Value(String/UTF8/Binary)
 */
class FTinyRedisCommand_Set : public ITinyRedisCommand
{
public:
	FTinyRedisCommand_Set(const FString& InKey, const FString& InValue, ERedisCommandType InCommandType = ERedisCommandType::SET)
		: CommandType(InCommandType), Key(InKey), String(InValue)
	{
	}

	FTinyRedisCommand_Set(const FString& InKey, const TArray<uint8>& InArray)
		: CommandType(ERedisCommandType::SET_BIN), Key(InKey), Array(InArray)
	{
	}

	virtual ERedisCommandType GetCommandType() const override
	{
		return CommandType;
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("SET %s .."), *Key);
	}

	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override
	{
		if (ERedisCommandType::SET == CommandType)
		{
			return Connection->ExecCommandEx(GetCommandType(), Reply, "SET %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*String));
		}
		if (ERedisCommandType::SET_UTF8 == CommandType)
		{
			return Connection->ExecCommandEx(GetCommandType(), Reply, "SET %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_UTF8(*String));
		}
		if (ERedisCommandType::SET_BIN == CommandType)
		{
			return Connection->ExecCommandEx(GetCommandType(), Reply, "SET %s %b", TCHAR_TO_ANSI(*Key), (char*)Array.GetData(), (size_t)Array.Num());
		}
		return false;
	}

	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) override
	{
		if (ERedisCommandType::SET == CommandType)
		{
			return Connection->AppendPipelineCommand(GetCommandType(), "SET %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*String));
		}
		if (ERedisCommandType::SET_UTF8 == CommandType)
		{
			return Connection->AppendPipelineCommand(GetCommandType(), "SET %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_UTF8(*String));
		}
		if (ERedisCommandType::SET_BIN == CommandType)
		{
			return Connection->AppendPipelineCommand(GetCommandType(), "SET %s %b", TCHAR_TO_ANSI(*Key), (char*)Array.GetData(), (size_t)Array.Num());
		}
		return false;
	}

protected:
	ERedisCommandType CommandType;
	FString Key;
	FString String;
	TArray<uint8> Array;
};

/**
 * Redis : Get Key as String/UTF8/Binary
 */
class FTinyRedisCommand_Get : public ITinyRedisCommand
{
public:
	FTinyRedisCommand_Get(const FString& InKey, ERedisCommandType InCommandType = ERedisCommandType::GET)
		: Key(InKey), CommandType(InCommandType)
	{
	}

	virtual ERedisCommandType GetCommandType() const override
	{
		return CommandType;
	}

	virtual FString ToDebugString() const override
	{
		return FString::Printf(TEXT("GET %s"), *Key);
	}

	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override
	{
		return Connection->ExecCommandEx(GetCommandType(), Reply, "GET %s", TCHAR_TO_ANSI(*Key));
	}

	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) override
	{
		return Connection->AppendPipelineCommand(GetCommandType(), "GET %s", TCHAR_TO_ANSI(*Key));
	}

protected:
	FString Key;
	ERedisCommandType CommandType;
};

/**
 * Redis - HGet
 */
class FTinyRedisCommand_HGet : public FTinyRedisCommand
{
public:
	FTinyRedisCommand_HGet(const FString& InKey, const FString& InField) : Key(InKey), Field(InField)
	{
		Command = FString::Printf(TEXT("HGET %s %s"), *InKey, *InField);
	}

protected:
	FString Key;
	FString Field;
};


/**
 * Redis - HSet
 */
class FTinyRedisCommand_HSet : public ITinyRedisCommand
{
public:
	FTinyRedisCommand_HSet(const FString& InKey, const FString& InField, const FString& InValue) : Key(InKey), Field(InField), Value(InValue)
	{
	}

	virtual bool Exec(TSharedPtr<FRedisConnection> Connection, FRedisReply& Reply) override
	{
		// Set Binary : SET %s %b 
		// Set UTF8 : SET %s %s
		// Set String : SET %s %s
		return Connection->ExecCommandEx(Reply, Reply.Error, GetCommandType(), "HSET %s %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*Field), TCHAR_TO_ANSI(*Value));
	}

	virtual bool AppendPipeline(TSharedPtr<FRedisConnection> Connection) override
	{
		// Set Binary : SET %s %b 
		// Set UTF8 : SET %s %s
		// Set String : SET %s %s

		return Connection->AppendPipelineCommand(GetCommandType(), "HSET %s %s %s", TCHAR_TO_ANSI(*Key), TCHAR_TO_ANSI(*Field), TCHAR_TO_ANSI(*Value));
	}

protected:
	FString Key;
	FString Field;
	FString Value;
};
