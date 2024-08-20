#include "TinyRedisTypes.h"

#include <hiredis.h>

void FRedisReply::ParseReply(const redisReply* Reply, ERedisCommandType InCommandType)
{
	CommandType = InCommandType;

	switch (Reply->type)
	{
	case REDIS_REPLY_STRING:
		{
			Type = ERedisReplyType::String;
			if (ERedisCommandType::GET_UTF8 == CommandType || ERedisCommandType::HGET_UTF8 == CommandType) // as UTF8 string
			{
				String = UTF8_TO_TCHAR(Reply->str);
			}
			else if (ERedisCommandType::GET_BIN == CommandType || ERedisCommandType::HGET_BIN == CommandType) // as binary
			{
				BinArray.Append((uint8*)(Reply->str), Reply->len);
			}
			else // as string
			{
				// String = Reply->str;
				String = ANSI_TO_TCHAR(Reply->str);
			}
			break;
		}
	case REDIS_REPLY_INTEGER:
		{
			Type = ERedisReplyType::Integer;
			Integer = Reply->integer;
			break;
		}
	case REDIS_REPLY_ARRAY:
		{
			Type = ERedisReplyType::Array;
			for (size_t i = 0; i < Reply->elements; i++)
			{
				// element is string/bin
				redisReply* Element = Reply->element[static_cast<int>(i)];
				Elements.Add(Element->str);
			}
			break;
		}
	case REDIS_REPLY_STATUS:
		{
			Type = ERedisReplyType::Status;
			Status = FString(ANSI_TO_TCHAR(Reply->str));
			// Status = Reply->str;
			break;
		}
	default:
		{
			Type = ERedisReplyType::Nil;
			break;
		}
	}

	switch (CommandType)
	{
	case ERedisCommandType::HMGET:
	case ERedisCommandType::HGETALL:
		{
			for (auto i = 0; i < Elements.Num(); i += 2)
			{
				FieldValues.Add(Elements[i], Elements[i + 1]);
			}
			break;
		}
	default:
		break;
	}
}

FString FRedisReply::ToDebugString() const
{
	if (HasError())
	{
		return FString::Printf(TEXT("Error:%s"), *Error);
	}

	switch (Type)
	{
	case ERedisReplyType::Integer:
		{
			return FString::Printf(TEXT("Integer:%d"), Integer);
		}
		break;
	case ERedisReplyType::String:
		{
			return FString::Printf(TEXT("String:%s"), *String);
		}
		break;
	case ERedisReplyType::Array:
		{
			return FString::Printf(TEXT("Array:[%s]"), *FString::Join(Elements, TEXT(",")));
		}
		break;
	case ERedisReplyType::Status:
		{
			return FString::Printf(TEXT("Status:%s"), *Status);
		}
		break;
	case ERedisReplyType::Nil:
		{
			return TEXT("Nil");
		}
		break;
	}

	return FString::Printf(TEXT("Invaid"));
}
