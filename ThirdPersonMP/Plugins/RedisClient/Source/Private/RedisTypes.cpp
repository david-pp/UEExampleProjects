#include "RedisTypes.h"

#include <hiredis.h>

void FRedisReply::ParserReply(const redisReply* Reply)
{
	switch (Reply->type)
	{
	case REDIS_REPLY_STRING:
		{
			Type = ERedisReplyType::String;
			String = Reply->str; // string or binary
			// String = FString(ANSI_TO_TCHAR(Reply->str));
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
