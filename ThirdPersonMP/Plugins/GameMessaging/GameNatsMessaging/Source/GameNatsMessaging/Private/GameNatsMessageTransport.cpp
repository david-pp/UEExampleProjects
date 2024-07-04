#include "GameNatsMessageTransport.h"

#include "GameNatsDeserializedMessage.h"
#include "GameNatsSerializedMessage.h"
#include "GameNatsSerializeMessageTask.h"
#include "IMessageContext.h"
#include "IMessageTransportHandler.h"
#include "NatsClientModule.h"
#include "Serialization/ArrayWriter.h"

FGameNatsMessageTransport::FGameNatsMessageTransport(const FString& InNatsClientName, const FString& InNatsURL, const FGuid& InNodeId)
	: NatsClientName(InNatsClientName), NatServerURL(InNatsURL), NatsNodeId(InNodeId)
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

		NatsClient->Subscribe(GetPublicMessageChannel(), [this](const char* DataPtr, int32 DataLength)
		{
			this->HandleGameMessage(DataPtr, DataLength);
		});

		NatsClient->Subscribe(GetPrivateMessageChannel(), [this](const char* DataPtr, int32 DataLength)
		{
			this->HandleGameMessage(DataPtr, DataLength);
		});

		NatsClient->Subscribe(GetClientStatusChannel(), [this](const char* DataPtr, int32 DataLength)
		{
			this->HandleClientStatusMessage(DataPtr, DataLength);
		});

		NatsClientStatusTicker = FTicker::GetCoreTicker().AddTicker(TEXT("NatsClientStatusTicker"), NatsClientStatusInterval, [this](float DeltaTime) -> bool
		{
			this->UpdateRemoteClientStatus();
			this->PublishClientStatus();
			return true;
		});

		UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - StartTransport : connect to  nats server=%s"), *NatServerURL);
	}

	return true;
}

void FGameNatsMessageTransport::StopTransport()
{
	if (NatsClient)
	{
		UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - StopTransport : disconnect from nats server=%s"), *NatServerURL);
		NatsClient->Close();
		NatsClient = nullptr;
	}

	if (NatsClientStatusTicker.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(NatsClientStatusTicker);
	}
}

bool FGameNatsMessageTransport::TransportMessage(const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context, const TArray<FGuid>& Recipients)
{
	if (Context->GetRecipients().Num() > 1024)
	{
		return false;
	}

	TArray<FString> Channels;
	if (Recipients.Num() == 0)
	{
		Channels.Add(GetPublicMessageChannel());
	}
	else
	{
		// Find connections for each recipient. (as NatsNode)
		for (auto& NodeId : Recipients)
		{
			FGameNatsClientStatus* RemoteNatsNode = RemoteNatsClients.Find(NodeId);
			if (RemoteNatsNode)
			{
				Channels.Add(FString::Printf(TEXT("%s.%s"), NATS_CLIENT_PRIVATE_CHANNEL, *RemoteNatsNode->NodeName));
			}
		}

		if (Channels.Num() == 0)
		{
			UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - TransportMessage: %s to Nats: Recipients are invalids"), *Context->GetMessageType().ToString());
			return false;
		}
	}

	// log
	FString ChannelsStr = FString::Join(Channels, TEXT("+"));
	UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - TransportMessage: %s to Nats channel=%s"), *Context->GetMessageType().ToString(), *ChannelsStr);

	// send message to nats server
	FNatsSerializedMessageRef SerializedMessage = MakeShareable(new FNatsSerializedMessage());
	TGraphTask<FNatsSerializeMessageTask>::CreateTask().ConstructAndDispatchWhenReady(Context, SerializedMessage, NatsClient, NatsNodeId, Channels);
	return true;
}

void FGameNatsMessageTransport::HandleGameMessage(const char* DataPtr, int32 DataLength)
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
			UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - Received message '%s' from %s@natsnode:%s"), *DeserializedMessage->GetMessageType().ToString(), *DeserializedMessage->GetSender().ToString(), *DeserializedMessage->GetNatsNodeId().ToString());

			FGuid SenderNodeId = DeserializedMessage->GetNatsNodeId();
			if (SenderNodeId != NatsNodeId) // very important !!!
			{
				TransportHandler->ReceiveTransportMessage(DeserializedMessage.ToSharedRef(), SenderNodeId);
			}
		}
		// TransportHandler->ReceiveTransportMessage();	
	}
}

void FGameNatsMessageTransport::PublishClientStatus()
{
	// current client status
	FGameNatsClientStatus ClientStatus(NatsNodeId);
	ClientStatus.Status = 200;
	ClientStatus.NodeName = NatsClientName;

	FArrayWriter MessageData;
	MessageData << ClientStatus;

	if (NatsClient)
	{
		NatsClient->Publish(GetClientStatusChannel(), (char*)MessageData.GetData(), sizeof(FGameNatsClientStatus));
	}
}

void FGameNatsMessageTransport::HandleClientStatusMessage(const char* DataPtr, int32 DataLength)
{
	if (DataLength >= sizeof(FGameNatsClientStatus))
	{
		FArrayReader MessageData = FArrayReader(true);
		MessageData.SetNumUninitialized(sizeof(FGameNatsClientStatus));
		FMemory::Memcpy(MessageData.GetData(), DataPtr, DataLength);

		FGameNatsClientStatus InClientStatus;
		MessageData << InClientStatus;

		// new remote client status
		if (!RemoteNatsClients.Contains(InClientStatus.NodeId))
		{
			RemoteNatsClients.Add(InClientStatus.NodeId, InClientStatus);
			if (TransportHandler)
			{
				TransportHandler->DiscoverTransportNode(InClientStatus.NodeId);
			}

			UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - Discover nats node,  status : %s"), *InClientStatus.ToDebugString());
		}
		else
		{
			// update remote client status
			FGameNatsClientStatus& ClientStatus = RemoteNatsClients.FindOrAdd(InClientStatus.NodeId);
			ClientStatus = InClientStatus;
			// UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - Update nats node,  status : %s"), *InClientStatus.ToDebugString());
		}
	}
}

void FGameNatsMessageTransport::UpdateRemoteClientStatus()
{
	TArray<FGuid> TimeoutClients;
	for (auto& Pair : RemoteNatsClients)
	{
		FDateTime Now = FDateTime::Now();

		// time out
		if (Now > Pair.Value.Timestamp + FTimespan::FromSeconds(RemoteNatsClientTimeoutSeconds))
		{
			TimeoutClients.Add(Pair.Key);
		}
	}

	for (auto& NodeId : TimeoutClients)
	{
		FGameNatsClientStatus* ClientStatus = RemoteNatsClients.Find(NodeId);
		if (ClientStatus)
		{
			if (TransportHandler)
			{
				TransportHandler->ForgetTransportNode(ClientStatus->NodeId);
			}
			UE_LOG(LogTemp, Log, TEXT("GameNatsMessaging - Forget nats node,  status : %s"), *ClientStatus->ToDebugString());
			RemoteNatsClients.Remove(NodeId);
		}
	}
}
