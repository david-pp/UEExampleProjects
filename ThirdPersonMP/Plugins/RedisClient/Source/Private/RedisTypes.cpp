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
