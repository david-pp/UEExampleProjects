// Fill out your copyright notice in the Description page of Project Settings.

#include "GameRpcServer.h"

#include "GameMessages.h"
#include "MessageEndpoint.h"

FName FGameRpcServer::GetDebugName() const
{
	if (MessageEndpoint)
	{
		return MessageEndpoint->GetDebugName();
	}
	return NAME_None;
}

void FGameRpcServer::SendRpcServerLocationTo(const FMessageAddress& Recipient) const
{
	if (MessageEndpoint)
	{
		FGameRpcServerLocation* Message = FMessageEndpoint::MakeMessage<FGameRpcServerLocation>();
		Message->ServerAddress = MessageEndpoint->GetAddress().ToString();
		MessageEndpoint->Send(Message, Recipient);
	}
}
