﻿// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MessageRpcServer.h"

class GAMEMESSAGING_API FGameRpcServer : public FMessageRpcServer
{
public:
	using FMessageRpcServer::FMessageRpcServer;

	virtual ~FGameRpcServer()
	{
	}

	FName GetDebugName() const;

public:
	// Send Rpc server's address to Recipient
	virtual void SendRpcServerLocationTo(const FMessageAddress& Recipient) const;
};
