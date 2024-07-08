// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameUserMessages.h"
#include "IGameService.h"
#include "GameRpcClient.h"
#include "User/IGameUserService.h"

class FGameUserProxy : public IGameService, public IGameUserInterface
{
public:
	using IGameService::IGameService;
	virtual TAsyncResult<FGameUserDetails> GetUserDetails() override
	{
		UE_LOG(LogTemp, Warning, TEXT("GetUserDetails@Proxy -----"));
		return RpcClient->Call<FGameUserGetUserDetails>();
	}
};


