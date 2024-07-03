// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "IMessageContext.h"
#include "GameNatsSerializedMessage.h"
#include "NatsClient.h"

/**
 * Implements an asynchronous task for serializing a message.
 */
class FNatsSerializeMessageTask
{
public:
	/**
	 * Creates and initializes a new instance.
	 *
	 * @param InMessageContext The context of the message to serialize.
	 * @param InSerializedMessage Will hold the serialized message data.
	 */
	FNatsSerializeMessageTask(TSharedRef<IMessageContext, ESPMode::ThreadSafe> InMessageContext, FNatsSerializedMessageRef InSerializedMessage, const TSharedPtr<INatsClient>& InNatsClient, const FGuid& InNatsNodeId, const TArray<FString>& InChannels)
		: MessageContext(InMessageContext), SerializedMessage(InSerializedMessage), NatsClient(InNatsClient), NatsNodeId(InNatsNodeId), NatsChannels(InChannels)
	{
	}

public:
	/**
	 * Performs the actual task.
	 *
	 * @param CurrentThread The thread that this task is executing on.
	 * @param MyCompletionGraphEvent The completion event.
	 */
	void DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent);

	/**
	 * Returns the name of the thread that this task should run on.
	 *
	 * @return Thread name.
	 */
	ENamedThreads::Type GetDesiredThread();

	/**
	 * Gets the task's stats tracking identifier.
	 *
	 * @return Stats identifier.
	 */
	TStatId GetStatId() const;

	/**
	 * Gets the mode for tracking subsequent tasks.
	 *
	 * @return Always track subsequent tasks.
	 */
	static ESubsequentsMode::Type GetSubsequentsMode();

private:
	/** Holds the context of the message to serialize. */
	TSharedRef<IMessageContext, ESPMode::ThreadSafe> MessageContext;

	/** Holds a reference to the serialized message data. */
	FNatsSerializedMessageRef SerializedMessage;

	/** Connections we're going to enqueue the serialized message for */
	TSharedPtr<INatsClient> NatsClient;
	/** Hold a NatsNodeId */
	FGuid NatsNodeId;

	TArray<FString> NatsChannels;
};
