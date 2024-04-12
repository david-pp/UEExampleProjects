// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "User/IGameUserService.h"
#include "RpcMessage.h"
#include "GameUserMessages.generated.h"

USTRUCT()
struct FGameUserGetUserDetailsRequest : public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	FGameUserGetUserDetailsRequest()
	{
	}
};


USTRUCT()
struct FGameUserGetUserDetailsResponse : public FRpcMessage
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, Category = "Message")
	FGameUserDetails Result;

	FGameUserGetUserDetailsResponse()
	{
	}

	FGameUserGetUserDetailsResponse(const FGameUserDetails& InResult) : Result(InResult)
	{
	}
};

DECLARE_RPC(FGameUserGetUserDetails, FGameUserDetails)
