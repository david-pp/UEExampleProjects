// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameTcpSerializeMessageTask.h"
#include "GameTcpMessageTransportConnection.h"
#include "Backends/JsonStructSerializerBackend.h"
#include "StructSerializer.h"


/* FGameTcpSerializeMessageTask interface
 *****************************************************************************/

void FGameTcpSerializeMessageTask::DoTask(ENamedThreads::Type CurrentThread, const FGraphEventRef& MyCompletionGraphEvent)
{
	if (MessageContext->IsValid())
	{
		// Note that some complex values are serialized manually here, so that we can ensure
		// a consistent wire format, if their implementations change. This allows us to sanity
		// check the values during deserialization. @see FGameTcpDeserializeMessage::Deserialize()

		// serialize context
		FArchive& Archive = SerializedMessage.Get();
		{
			const FName& MessageType = MessageContext->GetMessageType();
			Archive << const_cast<FName&>(MessageType);

			const FMessageAddress& Sender = MessageContext->GetSender();
			Archive << const_cast<FMessageAddress&>(Sender);

			const TArray<FMessageAddress>& Recipients = MessageContext->GetRecipients();
			Archive << const_cast<TArray<FMessageAddress>&>(Recipients);

			EMessageScope Scope = MessageContext->GetScope();
			Archive << Scope;

			const FDateTime& TimeSent = MessageContext->GetTimeSent();
			Archive << const_cast<FDateTime&>(TimeSent);

			const FDateTime& Expiration = MessageContext->GetExpiration();
			Archive << const_cast<FDateTime&>(Expiration);

			int32 NumAnnotations = MessageContext->GetAnnotations().Num();
			Archive << NumAnnotations;

			for (const auto& AnnotationPair : MessageContext->GetAnnotations())
			{
				Archive << const_cast<FName&>(AnnotationPair.Key);
				Archive << const_cast<FString&>(AnnotationPair.Value);
			}
		}

		// serialize message body
		UScriptStruct* MessageTypeInfoPtr = MessageContext->GetMessageTypeInfo().Get();
		FJsonStructSerializerBackend Backend(Archive, EStructSerializerBackendFlags::Legacy);
		FStructSerializer::Serialize(MessageContext->GetMessage(), *MessageTypeInfoPtr, Backend);

		// enqueue to recipients
		for (auto& Connection : RecipientConnections)
		{
			Connection->Send(SerializedMessage);
		}
	}
}


ENamedThreads::Type FGameTcpSerializeMessageTask::GetDesiredThread()
{
	return ENamedThreads::AnyThread;
}


TStatId FGameTcpSerializeMessageTask::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FGameTcpSerializeMessageTask, STATGROUP_TaskGraphTasks);
}


ESubsequentsMode::Type FGameTcpSerializeMessageTask::GetSubsequentsMode() 
{ 
	return ESubsequentsMode::FireAndForget; 
}
