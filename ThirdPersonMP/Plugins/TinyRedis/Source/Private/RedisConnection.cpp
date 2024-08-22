#include "RedisConnection.h"
#include "TinyRedisModule.h"

//  -------------------------------------------------------
#if PLATFORM_WINDOWS
#	include "Windows/AllowWindowsPlatformTypes.h"
#	include "Windows/AllowWindowsPlatformAtomics.h"
#endif
THIRD_PARTY_INCLUDES_START

#include "hiredis.h"

#if PLATFORM_WINDOWS
#	include <winsock2.h>
#	pragma comment(lib, "ws2_32.lib")
#endif

THIRD_PARTY_INCLUDES_END
#if PLATFORM_WINDOWS
#	include "Windows/HideWindowsPlatformAtomics.h"
#	include "Windows/HideWindowsPlatformTypes.h"
#endif
//  -------------------------------------------------------

#include <sstream>

// ============================================================================

constexpr int32 RedisCompressBinReservedBytes = 4;

bool RedisCompressBin(const TArray<char>& InValue, TArray<char>& OutValue)
{
	int32 CompressedSize = FCompression::CompressMemoryBound(NAME_Zlib, InValue.Num());

	OutValue.Empty();
	OutValue.SetNumUninitialized(RedisCompressBinReservedBytes + CompressedSize);

	if (FCompression::CompressMemory(NAME_Zlib, OutValue.GetData() + RedisCompressBinReservedBytes, CompressedSize, InValue.GetData(), InValue.Num()))
	{
		OutValue.SetNum(RedisCompressBinReservedBytes + CompressedSize, false);
	}
	else
	{
		UE_LOG(LogRedis, Error, TEXT("数据压缩失败"));
		return false; // 压缩失败
	}

	int32* OriginalSizePtr = reinterpret_cast<int32*>(OutValue.GetData());
	*OriginalSizePtr = InValue.Num();

	return true;
}

bool RedisUncompressBin(const TArray<char>& InValue, TArray<char>& OutValue)
{
	if (InValue.Num() <= RedisCompressBinReservedBytes)
	{
		return false;
	}

	const int32* OriginalSizePtr = reinterpret_cast<const int32*>(InValue.GetData());

	OutValue.Empty();
	OutValue.SetNumUninitialized(*OriginalSizePtr);
	if (!FCompression::UncompressMemory(NAME_Zlib, OutValue.GetData(), OutValue.Num(), InValue.GetData() + RedisCompressBinReservedBytes, InValue.Num() - RedisCompressBinReservedBytes))
	{
		UE_LOG(LogRedis, Error, TEXT("数据解压失败"));
		return false; // 解压失败
	}

	return true;
}

// ============================================================================


FRedisConnection::FRedisConnection()
{
}

FRedisConnection::~FRedisConnection()
{
	DisconnectRedis();
}

bool FRedisConnection::ConnectToRedis(const FString& InHost, int32 InPort, const FString& InPassword)
{
	LastTryConnectTime = FDateTime::UtcNow();
	Host = InHost;
	Port = InPort;
	Password = InPassword;

	timeval TimeOut = {1, 0}; // one sec

	RedisContextPtr = redisConnectWithTimeout(TCHAR_TO_ANSI(*Host), Port, TimeOut);
	if (!RedisContextPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("ConnectFailed!  Host=%s Port=%d"), *Host, Port);
		return false;
	}
	else if (RedisContextPtr->err)
	{
		UE_LOG(LogRedis, Error, TEXT("ConnectFailed! Host=%s Port=%d Error=%d"), *Host, Port, RedisContextPtr->err);
		redisFree(RedisContextPtr);
		RedisContextPtr = nullptr;
		return false;
	}

	if (!InPassword.IsEmpty())
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "AUTH %s", TCHAR_TO_ANSI(*InPassword));
		if (!RedisReplyPtr)
		{
			UE_LOG(LogRedis, Error, TEXT("AuthFailed!  Host=%s Port=%d"), *Host, Port);
			redisFree(RedisContextPtr);
			RedisContextPtr = nullptr;
			return false;
		}

		if (RedisReplyPtr->type == REDIS_REPLY_ERROR)
		{
			if (InPassword.IsEmpty())
			{
				FString ErrorString = RedisReplyPtr->str;
				if (ErrorString.Contains(TEXT("ERR Client sent AUTH, but no password is set")))
				{
					UE_LOG(LogRedis, Display, TEXT("%s Done Host=%s Port=%d"), ANSI_TO_TCHAR(__FUNCTION__), *Host, Port);
					return true;
				}
			}
			UE_LOG(LogRedis, Error, TEXT("AuthFailed! Host=%s Port=%d Error=%d"), *Host, Port, RedisContextPtr->err);
			redisFree(RedisContextPtr);
			RedisContextPtr = nullptr;
			return false;
		}
	}

	bIsConnected = true;
	UE_LOG(LogRedis, Display, TEXT("%s Done Host=%s Port=%d"), ANSI_TO_TCHAR(__FUNCTION__), *Host, Port);
	return true;
}

void FRedisConnection::DisconnectRedis()
{
	UE_LOG(LogRedis, Display, TEXT("%s"), ANSI_TO_TCHAR(__FUNCTION__));
	if (RedisReplyPtr)
	{
		freeReplyObject(RedisReplyPtr);
		RedisReplyPtr = nullptr;
	}
	if (RedisContextPtr)
	{
		redisFree(RedisContextPtr);
		RedisContextPtr = nullptr;
	}
	bIsConnected = false;
}

void FRedisConnection::Quit()
{
	if (!RedisContextPtr)
	{
		return;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "QUIT");
	DisconnectRedis();
}

bool FRedisConnection::SelectIndex(int32 InIndex)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SELECT %d", InIndex);
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::ExecCommand(const FString& InCommand)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "%s", TCHAR_TO_ANSI(*InCommand));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::ExistsKey(const FString& InKey)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "EXISTS %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = (bool)RedisReplyPtr->integer;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::ExpireKey(const FString& InKey, int32 Sec)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "EXPIRE %s %d", TCHAR_TO_ANSI(*InKey), Sec);
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = (bool)RedisReplyPtr->integer;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::PersistKey(const FString& InKey)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "PERSIST %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = (bool)RedisReplyPtr->integer;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}


bool FRedisConnection::RenameKey(const FString& CurrentKey, const FString& NewKey)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "RENAME %s %s", TCHAR_TO_ANSI(*CurrentKey), TCHAR_TO_ANSI(*NewKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::DelKey(const FString& InKey)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "DEL %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::TypeKey(const FString& InKey, FString& OutType)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "TYPE %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_STATUS)
	{
		OutType = RedisReplyPtr->str; // none string list set zset hash 
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::MSet(TMap<FString, FString>& InMemberMap)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "MSET";
	for (auto& it : InMemberMap)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it.Key)) << " " << TCHAR_TO_ANSI(*(it.Value));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::MGet(const TArray<FString>& InKeyList, TArray<FString>& OutMemberList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "MGET";
	for (auto& it : InKeyList)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_ARRAY)
	{
		bResult = true;
		for (auto i = 0; i < RedisReplyPtr->elements; i++)
		{
			if (RedisReplyPtr->element[i]->str != NULL)
			{
				OutMemberList.Add(RedisReplyPtr->element[i]->str);
			}
		}
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SetInt(const FString& InKey, int64 InValue, int32 ExpireTime)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	if (ExpireTime == 0)
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %lld", TCHAR_TO_ANSI(*InKey), InValue);
	}
	else
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %lld EX %d", TCHAR_TO_ANSI(*InKey), InValue, ExpireTime);
	}
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::GetInt(const FString& InKey, int64& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "GET %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	switch (RedisReplyPtr->type)
	{
	case REDIS_REPLY_INTEGER:
		{
			OutValue = RedisReplyPtr->integer;
			bResult = true;
		}
		break;
	case REDIS_REPLY_STRING:
		{
			OutValue = atol(RedisReplyPtr->str);
			bResult = true;
		}
		break;
	default:
		break;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SetStr(const FString& InKey, const FString& InValue, int32 ExpireTime)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	if (ExpireTime == 0)
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InValue));
	}
	else
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %s EX %d", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InValue), ExpireTime);
	}
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::GetStr(const FString& InKey, FString& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "GET %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		OutValue = RedisReplyPtr->str;
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SetBin(const FString& InKey, const TArray<char>& InValue, int32 ExpireTime)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	if (ExpireTime == 0)
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %b", TCHAR_TO_ANSI(*InKey), InValue.GetData(), size_t(InValue.Num()));
	}
	else
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %b EX %d", TCHAR_TO_ANSI(*InKey), InValue.GetData(), size_t(InValue.Num()), ExpireTime);
	}
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::GetBin(const FString& InKey, TArray<char>& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "GET %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		OutValue.Append(RedisReplyPtr->str, RedisReplyPtr->len);
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}


bool FRedisConnection::SetCompressBin(const FString& InKey, const TArray<char>& InValue, int32 ExpireTime)
{
	TArray<char> CompressedBuffer;
	if (!RedisCompressBin(InValue, CompressedBuffer))
	{
		return false;
	}
	return SetBin(InKey, CompressedBuffer, ExpireTime);
}

bool FRedisConnection::GetCompressBin(const FString& InKey, TArray<char>& OutValue)
{
	TArray<char> CompressedBuffer;
	if (!GetBin(InKey, CompressedBuffer))
	{
		return false;
	}
	return RedisUncompressBin(CompressedBuffer, OutValue);
}

bool FRedisConnection::Append(const FString& InKey, const FString& InValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "APPEND %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InValue));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SetNxInt(const FString& InKey, int64 InValue, int32 ExpireTime)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	if (ExpireTime == 0)
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %lld NX", TCHAR_TO_ANSI(*InKey), InValue);
	}
	else
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %lld NX EX %d", TCHAR_TO_ANSI(*InKey), InValue, ExpireTime);
	}
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_NIL)
	{
		bResult = false; // 设置失败
	}
	else
	{
		if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
		{
			bResult = true;
		}
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SetNxStr(const FString& InKey, const FString& InValue, int32 ExpireTime)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	if (ExpireTime == 0)
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %s NX", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InValue));
	}
	else
	{
		RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SET %s %s NX EX %d", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InValue), ExpireTime);
	}
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_NIL)
	{
		bResult = false; // 设置失败
	}
	else
	{
		if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
		{
			bResult = true;
		}
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SAdd(const FString& InKey, const TArray<FString>& InMemberList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "SADD " << TCHAR_TO_ANSI(*InKey);
	for (auto& it : InMemberList)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}


bool FRedisConnection::SCard(const FString& InKey, int32& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SCARD %s ", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_INTEGER)
	{
		bResult = true;
		OutValue = RedisReplyPtr->integer;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SRem(const FString& InKey, const TArray<FString>& InMemberList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "SREM " << TCHAR_TO_ANSI(*InKey);
	for (auto& it : InMemberList)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::SMembers(const FString& InKey, TArray<FString>& OutMemberList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "SMEMBERS %s ", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_ARRAY)
	{
		bResult = true;
		for (auto i = 0; i < RedisReplyPtr->elements; i++)
		{
			if (RedisReplyPtr->element[i]->str != NULL)
			{
				OutMemberList.Add(RedisReplyPtr->element[i]->str);
			}
		}
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::HSet(const FString& InKey, const FString& InField, const FString& InValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "HSET %s %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InField), TCHAR_TO_ANSI(*InValue));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::HGet(const FString& InKey, const FString& InField, FString& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 还未初始化"), ANSI_TO_TCHAR(__FUNCTION__));
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "HGET %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InField));
	if (!RedisReplyPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField);
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		OutValue = RedisReplyPtr->str;
		bResult = true;
	}
	else
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s ReplyType=%d"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField, RedisReplyPtr->type);
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::HSetBin(const FString& InKey, const FString& InField, const TArray<char>& InValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 还未初始化"), ANSI_TO_TCHAR(__FUNCTION__));
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "HSET %s %s %b", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InField), InValue.GetData(), size_t(InValue.Num()));
	if (!RedisReplyPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField);
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}
	else
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s ReplyType=%d"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField, RedisReplyPtr->type);
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::HGetBin(const FString& InKey, const FString& InField, TArray<char>& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 还未初始化"), ANSI_TO_TCHAR(__FUNCTION__));
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "HGET %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InField));
	if (!RedisReplyPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField);
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		OutValue.Append(RedisReplyPtr->str, RedisReplyPtr->len);
		bResult = true;
	}
	else
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s ReplyType=%d"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField, RedisReplyPtr->type);
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::HSetCompressBin(const FString& InKey, const FString& Field, const TArray<char>& InValue)
{
	TArray<char> CompressedBuffer;
	if (!RedisCompressBin(InValue, CompressedBuffer))
	{
		return false;
	}
	return HSetBin(InKey, Field, CompressedBuffer);
}

bool FRedisConnection::HGetCompressBin(const FString& InKey, const FString& Field, TArray<char>& OutValue)
{
	TArray<char> CompressedBuffer;
	if (!HGetBin(InKey, Field, CompressedBuffer))
	{
		return false;
	}
	return RedisUncompressBin(CompressedBuffer, OutValue);
}

bool FRedisConnection::HIncrby(const FString& InKey, const FString& InField, int32 Incre, int64& OutVal)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 还未初始化"), ANSI_TO_TCHAR(__FUNCTION__));
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "HINCRBY %s %s %d", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InField), Incre);
	if (!RedisReplyPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField);
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		if (RedisReplyPtr->type == REDIS_REPLY_INTEGER)
		{
			OutVal = RedisReplyPtr->integer;
		}
		bResult = true;
	}
	else
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Key=%s Field=%s ReplyType=%d"), ANSI_TO_TCHAR(__FUNCTION__), *InKey, *InField, RedisReplyPtr->type);
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}


bool FRedisConnection::HMSet(const FString& InKey, const TMap<FString, FString>& InMemberMap)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 还未初始化"), ANSI_TO_TCHAR(__FUNCTION__));
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "HMSET " << TCHAR_TO_ANSI(*InKey);
	for (auto& it : InMemberMap)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it.Key)) << " " << TCHAR_TO_ANSI(*(it.Value));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 执行失败 Cmd=%s"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(Cmd.c_str()));
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}
	else
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Cmd=%s ReplyType=%d"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(Cmd.c_str()), RedisReplyPtr->type);
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::HDel(const FString& InKey, const TArray<FString>& InFieldList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "HDEL " << TCHAR_TO_ANSI(*InKey);
	for (auto& it : InFieldList)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}


bool FRedisConnection::HExists(const FString& InKey, const FString& InField)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "HEXISTS %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*InField));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		bResult = (bool)RedisReplyPtr->integer;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::HMGet(const FString& InKey, const TSet<FString>& InFieldList, TMap<FString, FString>& OutMemberMap)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 还未初始化"), ANSI_TO_TCHAR(__FUNCTION__));
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "HMGET " << TCHAR_TO_ANSI(*InKey);
	for (auto& it : InFieldList)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*it);
		OutMemberMap.Add(it);
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		UE_LOG(LogRedis, Error, TEXT("%s 执行失败 Cmd=%s"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(Cmd.c_str()));
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_ARRAY)
	{
		int i = 0;
		for (auto& it : OutMemberMap)
		{
			if (i >= RedisReplyPtr->elements)
			{
				break;
			}

			if (RedisReplyPtr->element[i]->str != NULL)
			{
				it.Value = RedisReplyPtr->element[i]->str;
			}

			++i;
		}
		bResult = true;
	}
	else
	{
		UE_LOG(LogRedis, Error, TEXT("%s 返回失败 Cmd=%s ReplyType=%d"), ANSI_TO_TCHAR(__FUNCTION__), ANSI_TO_TCHAR(Cmd.c_str()), RedisReplyPtr->type);
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}


bool FRedisConnection::HGetAll(const FString& InKey, TMap<FString, FString>& OutMemberMap)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "HGETALL %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_ARRAY)
	{
		for (auto i = 0; i < RedisReplyPtr->elements; i += 2)
		{
			if (RedisReplyPtr->element[i]->str != NULL && RedisReplyPtr->element[i + 1]->str != NULL)
			{
				OutMemberMap.Add(RedisReplyPtr->element[i]->str, RedisReplyPtr->element[i + 1]->str);
			}
		}
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LIndex(const FString& InKey, int32 InIndex, FString& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LINDEX %s %d", TCHAR_TO_ANSI(*InKey), InIndex);
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		OutValue = RedisReplyPtr->str;
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LInsertBefore(const FString& InKey, const FString& Pivot, const FString& InValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LINSERT %s BEFORE %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*Pivot), TCHAR_TO_ANSI(*InValue));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LInsertAfter(const FString& InKey, const FString& Pivot, const FString& InValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LINSERT %s AFTER %s %s", TCHAR_TO_ANSI(*InKey), TCHAR_TO_ANSI(*Pivot), TCHAR_TO_ANSI(*InValue));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LLen(const FString& InKey, int32& Len)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LLEN %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	switch (RedisReplyPtr->type)
	{
	case REDIS_REPLY_INTEGER:
		{
			Len = RedisReplyPtr->integer;
			bResult = true;
		}
		break;
	case REDIS_REPLY_STRING:
		{
			Len = atoi(RedisReplyPtr->str);
			bResult = true;
		}
		break;
	default:
		break;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LPop(const FString& InKey, FString& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LPOP %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}
	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		OutValue = RedisReplyPtr->str;
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LPush(const FString& InKey, const TArray<FString>& InFieldList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "LPUSH " << TCHAR_TO_ANSI(*InKey);
	for (auto& it : InFieldList)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LRange(const FString& InKey, int32 Start, int32 End, TArray<FString>& OutMemberList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LRANGE %s %d %d", TCHAR_TO_ANSI(*InKey), Start, End);
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type == REDIS_REPLY_ARRAY)
	{
		bResult = true;
		for (auto i = 0; i < RedisReplyPtr->elements; i++)
		{
			if (RedisReplyPtr->element[i]->str != NULL)
			{
				OutMemberList.Add(RedisReplyPtr->element[i]->str);
			}
		}
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LRem(const FString& InKey, const FString& InValue, int32 Count /*= 0*/)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LREM %s %d %s", TCHAR_TO_ANSI(*InKey), Count, TCHAR_TO_ANSI(*InValue));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LSet(const FString& InKey, int32 InIndex, const FString& InValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LSET %s %d %s", TCHAR_TO_ANSI(*InKey), InIndex, TCHAR_TO_ANSI(*InValue));
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::LTrim(const FString& InKey, int32 Start, int32 Stop)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "LTRIM %s %d %d", TCHAR_TO_ANSI(*InKey), Start, Stop);
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::RPop(const FString& InKey, FString& OutValue)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, "RPOP %s", TCHAR_TO_ANSI(*InKey));
	if (!RedisReplyPtr)
	{
		return bResult;
	}
	if (RedisReplyPtr->type == REDIS_REPLY_STRING)
	{
		OutValue = RedisReplyPtr->str;
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

bool FRedisConnection::RPush(const FString& InKey, const TArray<FString>& InFieldList)
{
	bool bResult = false;

	if (!RedisContextPtr)
	{
		return bResult;
	}

	std::ostringstream redisCmd;
	redisCmd << "RPUSH " << TCHAR_TO_ANSI(*InKey);
	for (auto& it : InFieldList)
	{
		redisCmd << " " << TCHAR_TO_ANSI(*(it));
	}

	std::string Cmd = redisCmd.str();
	RedisReplyPtr = (redisReply*)redisCommand(RedisContextPtr, Cmd.c_str());
	if (!RedisReplyPtr)
	{
		return bResult;
	}

	if (RedisReplyPtr->type != REDIS_REPLY_ERROR)
	{
		bResult = true;
	}

	freeReplyObject(RedisReplyPtr);
	RedisReplyPtr = nullptr;

	return bResult;
}

// ------------------------ Interfaces for Async -----------------------

bool FRedisConnection::IsConnected() const
{
	return bIsConnected && RedisContextPtr;
}

bool FRedisConnection::IsServerClosed() const
{
	return RedisContextPtr && RedisContextPtr->err == REDIS_ERR_EOF;
}

bool FRedisConnection::GetError(FString& Err, redisReply* Reply) const
{
	if (RedisContextPtr == nullptr)
	{
		Err = FString(TEXT("invalid connection"));
		return true;
	}

	if (IsServerClosed())
	{
		Err = FString(TEXT("redis server is closed"));
		return true;
	}

	// redis connection error
	if (RedisContextPtr->err != 0)
	{
		Err = FString(ANSI_TO_TCHAR(RedisContextPtr->errstr));
		return true;
	}

	// redis command reply error
	if (Reply && Reply->type == REDIS_REPLY_ERROR)
	{
		Err = FString(ANSI_TO_TCHAR(Reply->str));
		return true;
	}

	Err = FString("");
	return false;
}


bool FRedisConnection::ExecCommand(const FString& Command, FRedisReply& Value, FString& Err, ERedisCommandType CommandType)
{
	if (!IsConnected())
	{
		Err = FString(TEXT("redis client not connected"));
		TryReconnectToRedis(Host, Port, Password, 0.0f);
		return false;
	}

	redisReply* Reply = static_cast<redisReply*>(redisCommand(RedisContextPtr, TCHAR_TO_ANSI(*Command)));

	// Server error
	if (RedisContextPtr->err)
	{
		GetError(Err, Reply);
		freeReplyObject(Reply);
		// disconnect & reconnect
		DisconnectRedis();
		TryReconnectToRedis(Host, Port, Password, 0.0f);
		return false;
	}

	// Command has error
	if (GetError(Err, Reply))
	{
		freeReplyObject(Reply);
		return false;
	}

	Value.ParseReply(Reply, CommandType);
	freeReplyObject(Reply);
	return true;
}

bool FRedisConnection::ExecCommandEx(FRedisReply& Value, FString& Err, ERedisCommandType CommandType, const char* format, ...)
{
	if (!IsConnected())
	{
		Err = FString(TEXT("redis client not connected"));
		TryReconnectToRedis(Host, Port, Password, 0.0f);
		return false;
	}

	va_list ap;
	va_start(ap, format);
	redisReply* Reply = static_cast<redisReply*>(redisvCommand(RedisContextPtr, format, ap));
	va_end(ap);

	// Server error
	if (RedisContextPtr->err)
	{
		GetError(Err, Reply);
		freeReplyObject(Reply);
		// disconnect & reconnect
		DisconnectRedis();
		TryReconnectToRedis(Host, Port, Password, 0.0f);
		return false;
	}

	// Command has error
	if (GetError(Err, Reply))
	{
		freeReplyObject(Reply);
		return false;
	}

	Value.ParseReply(Reply, CommandType);
	freeReplyObject(Reply);
	return true;
}

bool FRedisConnection::ExecPipelineCommandsEx(const TArray<FString>& PipelineCommands, TArray<FRedisReply>& Values, FString& Err)
{
	if (!IsConnected())
	{
		Err = FString(TEXT("redis client not connected"));
		return false;
	}

	int PipelineCommandNum = 0;
	for (const FString& Command : PipelineCommands)
	{
		redisAppendCommand(RedisContextPtr, TCHAR_TO_ANSI(*Command));
		PipelineCommandNum++;
	}

	for (int i = 0; i < PipelineCommandNum; i++)
	{
		redisReply* Reply;
		FRedisReply Value;
		redisGetReply(RedisContextPtr, reinterpret_cast<void**>(&Reply));

		// Server error
		if (RedisContextPtr->err)
		{
			GetError(Err, Reply);
			freeReplyObject(Reply);
			// disconnect & reconnect
			DisconnectRedis();
			TryReconnectToRedis(Host, Port, Password, 0.0f);
			return false;
		}

		// Command has error
		if (GetError(Err, Reply))
		{
			freeReplyObject(Reply);
			return false;
		}

		Value.ParseReply(Reply, ERedisCommandType::UNKNOWN);
		Values.Add(MoveTemp(Value));
		freeReplyObject(Reply);
	}

	return true;
}

bool FRedisConnection::AppendPipelineCommand(ERedisCommandType CommandType, const char* CommandFormat, ...)
{
	va_list ap;
	va_start(ap, CommandFormat);
	int Ret = redisvAppendCommand(RedisContextPtr, CommandFormat, ap);
	va_end(ap);
	return Ret == REDIS_OK;
}

bool FRedisConnection::GetPipelineCommandReply(ERedisCommandType CommandType, FRedisReply& Value)
{
	redisReply* Reply = nullptr;
	if (redisGetReply(RedisContextPtr, reinterpret_cast<void**>(&Reply)) == REDIS_OK)
	{
		Value.ParseReply(Reply, CommandType);
		freeReplyObject(Reply);
		return true;
	}

	if (Reply)
	{
		GetError(Value.Error, Reply);
		freeReplyObject(Reply);
	}

	return false;
}

bool FRedisConnection::Ping(FString& Err)
{
	FRedisReply Reply;

	if (RedisContextPtr)
	{
		// ping timeout
		redisSetTimeout(RedisContextPtr, timeval{1, 0});
	}

	return ExecCommand("PING", Reply, Err);
}

bool FRedisConnection::TryReconnectToRedis(const FString& InHost, int32 InPort, const FString& InPassword, float IntervalSeconds)
{
	if (!IsConnected() || IsServerClosed())
	{
		// reconnect to redis server 
		if (FMath::IsNearlyZero(IntervalSeconds))
		{
			ConnectToRedis(InHost, InPort, InPassword);
			return true;
		}
		else
		{
			const FDateTime Now = FDateTime::UtcNow();
			if (Now > LastTryConnectTime + FTimespan::FromSeconds(IntervalSeconds))
			{
				ConnectToRedis(InHost, InPort, InPassword);
				LastTryConnectTime = Now;
				return true;
			}
		}
	}


	return false;
}
