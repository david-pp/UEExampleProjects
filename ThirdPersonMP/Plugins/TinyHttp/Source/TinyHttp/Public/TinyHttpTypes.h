// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TinyHttpTypes.generated.h"

enum class TINYHTTP_API ETinyHttpRequestVerb : uint16
{
	VERB_NONE = 0,
	VERB_GET = 1 << 0,
	VERB_POST = 1 << 1,
	VERB_PUT = 1 << 2,
	VERB_PATCH = 1 << 3,
	VERB_DELETE = 1 << 4,
	VERB_OPTIONS = 1 << 5
};

inline const TCHAR* LexToString(ETinyHttpRequestVerb Verb)
{
	switch (Verb)
	{
	case ETinyHttpRequestVerb::VERB_GET:
		return TEXT("GET");
	case ETinyHttpRequestVerb::VERB_POST:
		return TEXT("POST");
	case ETinyHttpRequestVerb::VERB_PUT:
		return TEXT("PUT");
	case ETinyHttpRequestVerb::VERB_DELETE:
		return TEXT("DELETE");
	case ETinyHttpRequestVerb::VERB_PATCH:
		return TEXT("PATCH");
	default:
		return TEXT("GET");
	}
}

USTRUCT()
struct TINYHTTP_API FServiceResponse
{
	GENERATED_BODY()

	FServiceResponse(const int32 InErrorCode = 0, const FString& InErrorMessage = TEXT(""))
		: ErrorCode(InErrorCode), ErrorMessage(InErrorMessage)
	{
	}

	bool HasError() const
	{
		return ErrorCode != 0 || ErrorMessage.Len() > 0;
	}

	int32 ErrorCode;
	FString ErrorMessage;
};

typedef TSharedPtr<FServiceResponse> FServiceResponsePtr;


#define HTTP_CODE_OK 200
#define HTTP_CODE_BAD_REQUEST 400

#define SERVICE_INVALID_REQUEST 100
#define SERVICE_NO_REPLY 101


