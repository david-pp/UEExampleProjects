#include "TinyHttpUtils.h"
#include "JsonObjectConverter.h"
#include "TinyHttp.h"

void FTinyHttp::ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload)
{
	int32 StartIndex = OutTCHARPayload.Num();
	OutTCHARPayload.AddUninitialized(FUTF8ToTCHAR_Convert::ConvertedLength((ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR)) * sizeof(TCHAR));
	FUTF8ToTCHAR_Convert::Convert((TCHAR*)(OutTCHARPayload.GetData() + StartIndex), (OutTCHARPayload.Num() - StartIndex) / sizeof(TCHAR), (ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR));
}

void FTinyHttp::ConvertToString(TConstArrayView<uint8> InUTF8Payload, FString& OutString)
{
	FUTF8ToTCHAR TCHARData(reinterpret_cast<const ANSICHAR*>(InUTF8Payload.GetData()), InUTF8Payload.Num());
	OutString = FString(TCHARData.Length(), TCHARData.Get());
}

void FTinyHttp::ConvertToUTF8(TConstArrayView<uint8> InTCHARPayload, TArray<uint8>& OutUTF8Payload)
{
	int32 StartIndex = OutUTF8Payload.Num();
	OutUTF8Payload.AddUninitialized(FTCHARToUTF8_Convert::ConvertedLength((TCHAR*)InTCHARPayload.GetData(), InTCHARPayload.Num() / sizeof(TCHAR)) * sizeof(ANSICHAR));
	FTCHARToUTF8_Convert::Convert((ANSICHAR*)(OutUTF8Payload.GetData() + StartIndex), (OutUTF8Payload.Num() - StartIndex) / sizeof(ANSICHAR), (TCHAR*)InTCHARPayload.GetData(), InTCHARPayload.Num() / sizeof(TCHAR));
}

void FTinyHttp::ConvertToUTF8(const FString& InString, TArray<uint8>& OutUTF8Payload)
{
	int32 StartIndex = OutUTF8Payload.Num();
	OutUTF8Payload.AddUninitialized(FTCHARToUTF8_Convert::ConvertedLength(*InString, InString.Len()) * sizeof(ANSICHAR));
	FTCHARToUTF8_Convert::Convert((ANSICHAR*)(OutUTF8Payload.GetData() + StartIndex), (OutUTF8Payload.Num() - StartIndex) / sizeof(ANSICHAR), *InString, InString.Len());
}

TSharedPtr<FJsonObject> FTinyHttp::JsonPayloadToObject(const TArray<uint8>& InUTF8Payload)
{
	TArray<uint8> TCHARBody;
	ConvertToTCHAR(InUTF8Payload, TCHARBody);

	FMemoryReaderView BodyReader(TCHARBody);
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(&BodyReader);
	if (FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		return JsonObject;
	}
	return nullptr;
}

bool FTinyHttp::JsonPayloadToUStruct(const TArray<uint8>& InUTF8Payload, const UStruct* StructDefinition, void* OutStruct)
{
	TArray<uint8> TCHARBody;
	ConvertToTCHAR(InUTF8Payload, TCHARBody);

	FMemoryReaderView BodyReader(TCHARBody);
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(&BodyReader);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTinyHttp, Warning, TEXT("JsonPayloadToUStruct - Unable to deserialize"));
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), StructDefinition, OutStruct))
	{
		UE_LOG(LogTinyHttp, Warning, TEXT("JsonPayloadToUStruct - Unable to convert to struct"));
		return false;
	}

	return true;
}

bool FTinyHttp::UStructToJsonPayload(TArray<uint8>& OutUTF8Payload, const UStruct* StructDefinition, const void* Struct)
{
	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(StructDefinition, Struct, JsonString))
	{
		return false;
	}

	ConvertToUTF8(JsonString, OutUTF8Payload);
	return true;
}

FString FTinyHttp::RequestVerbToString(EHttpServerRequestVerbs Verb)
{
	switch (Verb)
	{
	case EHttpServerRequestVerbs::VERB_GET:
		return TEXT("GET");
	case EHttpServerRequestVerbs::VERB_POST:
		return TEXT("POST");
	case EHttpServerRequestVerbs::VERB_PUT:
		return TEXT("PUT");
	case EHttpServerRequestVerbs::VERB_PATCH:
		return TEXT("PATCH");
	case EHttpServerRequestVerbs::VERB_DELETE:
		return TEXT("DELETE");
	default:
		break;
	}
	return TEXT("Unkown");
}

FString FTinyHttp::RequestBodyToString(const FHttpServerRequest& Request)
{
	FUTF8ToTCHAR TCHARData(reinterpret_cast<const ANSICHAR*>(Request.Body.GetData()), Request.Body.Num());
	FString BodyString(TCHARData.Length(), TCHARData.Get());
	return MoveTemp(BodyString);
}

FString FTinyHttp::RequestToDebugString(const FHttpServerRequest& Request, bool bShowBody)
{
	FString Result;

	FString QueryParams;
	for (auto It : Request.QueryParams)
	{
		if (!QueryParams.IsEmpty())
		{
			QueryParams.InsertAt(0, TEXT("&"));
		}

		QueryParams += FString::Printf(TEXT("%s=%s"), *It.Key, *It.Value);
	}

	// VERB URL?QueryParams
	Result += FString::Printf(TEXT("%s %s?%s\n"), *RequestVerbToString(Request.Verb), *Request.RelativePath.GetPath(), *QueryParams);

	// Header-Key : Value1,Value2
	for (auto Header : Request.Headers)
	{
		Result += FString::Printf(TEXT("%s : %s\n"), *Header.Key, *FString::Join(Header.Value, TEXT(",")));
	}

	// Body ?
	if (bShowBody)
	{
		FString ContentTypes = FString::Join(Request.Headers.FindRef(TEXT("Content-Type")), TEXT(","));

		if (ContentTypes.Find(TEXT("json")) != INDEX_NONE || ContentTypes.Find(TEXT("text")) != INDEX_NONE)
		{
			Result += FString::Printf(TEXT("\n%s\n"), *RequestBodyToString(Request));
		}
		else
		{
			Result += FString::Printf(TEXT("\nHex, Length=%d\n"), Request.Body.Num());
		}
	}

	// Query Paths
	for (auto It : Request.QueryParams)
	{
		Result += FString::Printf(TEXT("[%s] = %s\n"), *It.Key, *It.Value);
	}

	return Result;
}
