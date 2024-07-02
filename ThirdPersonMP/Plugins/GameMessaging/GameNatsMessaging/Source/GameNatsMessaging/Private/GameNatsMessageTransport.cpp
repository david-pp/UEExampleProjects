#include "GameNatsMessageTransport.h"

#include "GameNatsDeserializedMessage.h"
#include "GameNatsSerializedMessage.h"
#include "GameNatsSerializeMessageTask.h"
#include "IMessageContext.h"
#include "IMessageTransportHandler.h"
#include "NatsClientModule.h"

FGameNatsMessageTransport::FGameNatsMessageTransport(const FString& InNatsURL, const FString& InDefaultSubject)
	: DefaultSubject(InDefaultSubject), NatServerURL(InNatsURL)
{
}

FGameNatsMessageTransport::~FGameNatsMessageTransport()
{
	StopTransport();
}

void FGameNatsMessageTransport::DispatchMessageCallbacks()
{
	if (NatsClient)
	{
		NatsClient->Tick();
	}
}

bool FGameNatsMessageTransport::StartTransport(IMessageTransportHandler& Handler)
{
	TransportHandler = &Handler;

	NatsClient = INatsClientModule::Get().CreateNatsClient();
	if (NatsClient)
	{
		if (!NatsClient->ConnectTo(NatServerURL))
		{
			return false;
		}

		if (!DefaultSubject.IsEmpty())
		{
			bool bOk = NatsClient->Subscribe(DefaultSubject, [this](const char* DataPtr, int32 DataLength)
			{
				this->HandleDefaultSubjectMessage(DataPtr, DataLength);
			});
		}

		UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - StartTransport : Server=%s, DefaultSubject=%s"), *NatServerURL, *DefaultSubject);
	}

	return true;
}

void FGameNatsMessageTransport::StopTransport()
{
	if (NatsClient)
	{
		UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - StopTransport : Server=%s, DefaultSubject=%s"), *NatServerURL, *DefaultSubject);
		NatsClient->Close();
	}
}

bool FGameNatsMessageTransport::TransportMessage(const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context, const TArray<FGuid>& Recipients)
{
	if (Context->GetRecipients().Num() > 1024)
	{
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - TransportMessage: %s to Nats"), *Context->GetMessageType().ToString());

	FNatsSerializedMessageRef SerializedMessage = MakeShareable(new FNatsSerializedMessage());
	TGraphTask<FNatsSerializeMessageTask>::CreateTask().ConstructAndDispatchWhenReady(Context, SerializedMessage, NatsClient);
	return true;
}

void FGameNatsMessageTransport::HandleDefaultSubjectMessage(const char* DataPtr, int32 DataLength)
{
	if (TransportHandler)
	{
		/** Message data we're currently in the process of receiving, if any */
		TSharedPtr<FArrayReader, ESPMode::ThreadSafe> RecvMessageData;
		RecvMessageData = MakeShareable(new FArrayReader(true));
		RecvMessageData->SetNumUninitialized(DataLength);
		FMemory::Memcpy(RecvMessageData->GetData(), DataPtr, DataLength);

		// todo: move message deserialization into an async task
		TSharedPtr<FNatsDeserializedMessage, ESPMode::ThreadSafe> DeserializedMessage = MakeShareable(new FNatsDeserializedMessage(nullptr));
		if (DeserializedMessage->Deserialize(RecvMessageData))
		{
			UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - Received message '%s' from %s"), *DeserializedMessage->GetMessageType().ToString(), *DeserializedMessage->GetSender().ToString());

			FGuid SenderNodeId;
			TransportHandler->ReceiveTransportMessage(DeserializedMessage.ToSharedRef(), SenderNodeId);
		}
		// TransportHandler->ReceiveTransportMessage();	
	}
}
