// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameTcpDeserializedMessage.h"
#include "UObject/Package.h"
#include "GameTcpMessagingPrivate.h"
#include "Backends/JsonStructDeserializerBackend.h"
#include "StructDeserializer.h"


/* FGameTcpDeserializedMessage structors
*****************************************************************************/

FGameTcpDeserializedMessage::FGameTcpDeserializedMessage(const TSharedPtr<IMessageAttachment, ESPMode::ThreadSafe>& InAttachment)
	: Attachment(InAttachment)
	, MessageData(nullptr)
{ }


FGameTcpDeserializedMessage::~FGameTcpDeserializedMessage()
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


/* FGameTcpDeserializedMessage interface
 *****************************************************************************/

bool FGameTcpDeserializedMessage::Deserialize(const TSharedPtr<FArrayReader, ESPMode::ThreadSafe>& Message)
{
	FArrayReader& MessageReader = Message.ToSharedRef().Get();

	// Note that some complex values are deserialized manually here, so that we
	// can sanity check their values. @see FGameTcpSerializeMessageTask::DoTask()
	MessageReader.ArMaxSerializeSize = NAME_SIZE;

	UScriptStruct* TypeInfoPtr = nullptr;

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

const TMap<FName, FString>& FGameTcpDeserializedMessage::GetAnnotations() const
{
	return Annotations;
}


TSharedPtr<IMessageAttachment, ESPMode::ThreadSafe> FGameTcpDeserializedMessage::GetAttachment() const
{
	return Attachment;
}


const FDateTime& FGameTcpDeserializedMessage::GetExpiration() const
{
	return Expiration;
}


const void* FGameTcpDeserializedMessage::GetMessage() const
{
	return MessageData;
}


const TWeakObjectPtr<UScriptStruct>& FGameTcpDeserializedMessage::GetMessageTypeInfo() const
{
	return TypeInfo;
}


TSharedPtr<IMessageContext, ESPMode::ThreadSafe> FGameTcpDeserializedMessage::GetOriginalContext() const
{
	return nullptr;
}


const TArray<FMessageAddress>& FGameTcpDeserializedMessage::GetRecipients() const
{
	return Recipients;
}


EMessageScope FGameTcpDeserializedMessage::GetScope() const
{
	return Scope;
}

EMessageFlags FGameTcpDeserializedMessage::GetFlags() const
{
	return EMessageFlags::None;
}


const FMessageAddress& FGameTcpDeserializedMessage::GetSender() const
{
	return Sender;
}


const FMessageAddress& FGameTcpDeserializedMessage::GetForwarder() const
{
	return Sender;
}


ENamedThreads::Type FGameTcpDeserializedMessage::GetSenderThread() const
{
	return ENamedThreads::AnyThread;
}


const FDateTime& FGameTcpDeserializedMessage::GetTimeForwarded() const
{
	return TimeSent;
}


const FDateTime& FGameTcpDeserializedMessage::GetTimeSent() const
{
	return TimeSent;
}
