// Fill out your copyright notice in the Description page of Project Settings.

#include "GameRpcServer.h"

#include "GameMessages.h"
#include "MessageEndpoint.h"

void FGameRpcServer::SendRpcServerLocationTo(const FMessageAddress& Recipient) const
{
	if (MessageEndpoint)
	{
		FGameRpcServerLocation* Message = FMessageEndpoint::MakeMessage<FGameRpcServerLocation>();
		Message->ServerAddress = MessageEndpoint->GetAddress().ToString();
		MessageEndpoint->Send(Message, Recipient);
	}
}
