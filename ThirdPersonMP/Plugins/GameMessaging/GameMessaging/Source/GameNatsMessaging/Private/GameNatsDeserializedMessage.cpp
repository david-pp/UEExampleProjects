// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameNatsDeserializedMessage.h"
#include "UObject/Package.h"
#include "GameNatsMessagingPrivate.h"
#include "Backends/JsonStructDeserializerBackend.h"
#include "StructDeserializer.h"

/* FNatsDeserializedMessage structors
*****************************************************************************/

FNatsDeserializedMessage::FNatsDeserializedMessage(const TSharedPtr<IMessageAttachment, ESPMode::ThreadSafe>& InAttachment)
	: Attachment(InAttachment), MessageData(nullptr)
{
}


FNatsDeserializedMessage::~FNatsDeserializedMessage()
{
	if (MessageData != nullptr)
	{
		if (UScriptStruct* TypeInfoPtr = TypeInfo.Get())
		{
			TypeInfoPtr->DestroyStruct(MessageData);
		}

		FMemory::Free(MessageData);
		MessageData = nullptr;
	}
}


/* FNatsDeserializedMessage interface
 *****************************************************************************/

bool FNatsDeserializedMessage::Deserialize(const TSharedPtr<FArrayReader, ESPMode::ThreadSafe>& Message)
{
	FArrayReader& MessageReader = Message.ToSharedRef().Get();

	// Note that some complex values are deserialized manually here, so that we
	// can sanity check their values. @see FNatsSerializeMessageTask::DoTask()
	MessageReader.ArMaxSerializeSize = NAME_SIZE;

	UScriptStruct* TypeInfoPtr = nullptr;

	// message nats node
	{
		MessageReader << NatsNodeId;
	}

	// message type info
	{
		FName MessageType;
		MessageReader << MessageType;

		// @todo gmp: cache message types for faster lookup
		TypeInfoPtr = FindObjectSafe<UScriptStruct>(ANY_PACKAGE, *MessageType.ToString());

		TypeInfo = TypeInfoPtr;
		if (!TypeInfo.IsValid(false, true))
		{
			return false;
		}
	}
	check(TypeInfoPtr);

	// sender address
	{
		MessageReader << Sender;
	}

	// recipient addresses
	{
		int32 NumRecipients = 0;
		MessageReader << NumRecipients;

		if ((NumRecipients < 0) || (NumRecipients > TCP_MESSAGING_MAX_RECIPIENTS))
		{
			return false;
		}

		Recipients.Empty(NumRecipients);

		while (0 < NumRecipients--)
		{
			MessageReader << *::new(Recipients) FMessageAddress;
		}
	}

	// message scope
	{
		MessageReader << Scope;

		if (Scope > EMessageScope::All)
		{
			return false;
		}
	}

	// time sent & expiration
	{
		MessageReader << TimeSent;
		MessageReader << Expiration;
	}

	// annotations
	{
		int32 NumAnnotations = 0;
		MessageReader << NumAnnotations;

		if (NumAnnotations > TCP_MESSAGING_MAX_ANNOTATIONS)
		{
			return false;
		}

		while (0 < NumAnnotations--)
		{
			FName Key;
			FString Value;

			MessageReader << Key;
			MessageReader << Value;

			Annotations.Add(Key, Value);
		}
	}

	// create message body
	MessageData = FMemory::Malloc(TypeInfoPtr->GetStructureSize());
	TypeInfoPtr->InitializeStruct(MessageData);

	// deserialize message body
	FJsonStructDeserializerBackend Backend(MessageReader);
	return FStructDeserializer::Deserialize(MessageData, *TypeInfoPtr, Backend);
}


/* IMessageContext interface
 *****************************************************************************/

const TMap<FName, FString>& FNatsDeserializedMessage::GetAnnotations() const
{
	return Annotations;
}


TSharedPtr<IMessageAttachment, ESPMode::ThreadSafe> FNatsDeserializedMessage::GetAttachment() const
{
	return Attachment;
}


const FDateTime& FNatsDeserializedMessage::GetExpiration() const
{
	return Expiration;
}


const void* FNatsDeserializedMessage::GetMessage() const
{
	return MessageData;
}


const TWeakObjectPtr<UScriptStruct>& FNatsDeserializedMessage::GetMessageTypeInfo() const
{
	return TypeInfo;
}


TSharedPtr<IMessageContext, ESPMode::ThreadSafe> FNatsDeserializedMessage::GetOriginalContext() const
{
	return nullptr;
}


const TArray<FMessageAddress>& FNatsDeserializedMessage::GetRecipients() const
{
	return Recipients;
}


EMessageScope FNatsDeserializedMessage::GetScope() const
{
	return Scope;
}

EMessageFlags FNatsDeserializedMessage::GetFlags() const
{
	return EMessageFlags::None;
}


const FMessageAddress& FNatsDeserializedMessage::GetSender() const
{
	return Sender;
}


const FMessageAddress& FNatsDeserializedMessage::GetForwarder() const
{
	return Sender;
}


ENamedThreads::Type FNatsDeserializedMessage::GetSenderThread() const
{
	return ENamedThreads::AnyThread;
}


const FDateTime& FNatsDeserializedMessage::GetTimeForwarded() const
{
	return TimeSent;
}


const FDateTime& FNatsDeserializedMessage::GetTimeSent() const
{
	return TimeSent;
}
