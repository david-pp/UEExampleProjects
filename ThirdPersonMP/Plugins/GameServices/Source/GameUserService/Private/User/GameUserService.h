// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameUserMessages.h"
#include "IGameService.h"
#include "GameRpcServer.h"
#include "User/IGameUserService.h"

class FGameUserService : public IGameUserService
{
public:
	using IGameUserService::IGameUserService;
	
	virtual TAsyncResult<FGameUserDetails> GetUserDetails() override
	{
		UE_LOG(LogTemp, Warning, TEXT("GetUserDetails@Service -----"));
		FGameUserDetails UserDetails;
		UserDetails.DisplayName = FText::FromString(TEXT("David"));
		return TAsyncResult<FGameUserDetails>(UserDetails);
	}

public:
	virtual void OnCreate() override
	{
		if (RpcServer)
		{
			RpcServer->RegisterHandler<FGameUserGetUserDetails>(this, &FGameUserService::HandleGetUserDetails);
		}
	}

	TAsyncResult<FGameUserDetails> HandleGetUserDetails(const FGameUserGetUserDetailsRequest& Request)
	{
		return GetUserDetails();
	}
};
