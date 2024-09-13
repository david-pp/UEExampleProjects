// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TinyHttpTypes.generated.h"

USTRUCT()
struct TINYHTTP_API FServiceResponse
{
	GENERATED_BODY()

	FServiceResponse(const int32 InErrorCode = 0, const FString& InErrorMessage = TEXT(""))
		: ErrorCode(InErrorCode), ErrorMessage(InErrorMessage)
	{
	}

	bool IsOK() const
	{
		return ErrorCode == 0;
	}

	bool HasError() const
	{
		return ErrorCode != 0 || ErrorMessage.Len() > 0;
	}

	int32 ErrorCode;
	FString ErrorMessage;
};

typedef TSharedPtr<FServiceResponse> FServiceResponsePtr;

#define SERVICE_INVALID_REQUEST 100
#define SERVICE_NO_REPLY 101
