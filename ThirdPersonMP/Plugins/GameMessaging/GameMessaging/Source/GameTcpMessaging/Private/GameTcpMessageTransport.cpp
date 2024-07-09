// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameTcpMessageTransport.h"

#include "HAL/RunnableThread.h"
#include "Common/TcpSocketBuilder.h"
#include "Common/TcpListener.h"
#include "IMessageTransportHandler.h"
#include "GameTcpMessagingPrivate.h"

#include "GameTcpDeserializedMessage.h"
#include "GameTcpSerializedMessage.h"
#include "GameTcpMessageTransportConnection.h"
#include "GameTcpSerializeMessageTask.h"

/* FGameTcpMessageTransport structors
 *****************************************************************************/

FGameTcpMessageTransport::FGameTcpMessageTransport(const FIPv4Endpoint& InListenEndpoint, const TArray<FIPv4Endpoint>& InConnectToEndpoints, int32 InConnectionRetryDelay)
	: ListenEndpoint(InListenEndpoint)
	, ConnectToEndpoints(InConnectToEndpoints)
	, ConnectionRetryDelay(InConnectionRetryDelay)
	, bStopping(false)
	, SocketSubsystem(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM))
	, Listener(nullptr)
	, TransportHandler(nullptr)
{
	Thread = FRunnableThread::Create(this, TEXT("FGameTcpMessageTransport"), 128 * 1024, TPri_Normal);
}


FGameTcpMessageTransport::~FGameTcpMessageTransport()
{
	if (Thread != nullptr)
	{
		Thread->Kill(true);
		delete Thread;
	}

	StopTransport();
}


/* IMessageTransport interface
 *****************************************************************************/

bool FGameTcpMessageTransport::StartTransport(IMessageTransportHandler& Handler)
{
	TransportHandler = &Handler;

	if (ListenEndpoint != FIPv4Endpoint::Any)
	{
		Listener = new FTcpListener(ListenEndpoint);
		Listener->OnConnectionAccepted().BindRaw(this, &FGameTcpMessageTransport::HandleListenerConnectionAccepted);
	}

	// outgoing connections
	for (auto& ConnectToEndPoint : ConnectToEndpoints)
	{
		AddOutgoingConnection(ConnectToEndPoint);
	}

	return true;
}

void FGameTcpMessageTransport::AddOutgoingConnection(const FIPv4Endpoint& Endpoint)
{
	FSocket* Socket = FTcpSocketBuilder(TEXT("FGameTcpMessageTransport.RemoteConnection"));
	
	if (Socket == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create outgoing socket for %s"), *Endpoint.ToString());
		return;
	}

	if (!Socket->Connect(*Endpoint.ToInternetAddr()))
	{
		UE_LOG(LogTemp, Error, TEXT("Connect failed on outgoing socket for %s"), *Endpoint.ToString());
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
	else
	{
		PendingConnections.Enqueue(MakeShareable(new FGameTcpMessageTransportConnection(Socket, Endpoint, ConnectionRetryDelay)));
	}
}

void FGameTcpMessageTransport::RemoveOutgoingConnection(const FIPv4Endpoint& Endpoint)
{
	ConnectionEndpointsToRemove.Enqueue(Endpoint);
}

void FGameTcpMessageTransport::StopTransport()
{
	bStopping = true;

	if (Listener)
	{
		delete Listener;
		Listener = nullptr;
	}

	for (auto& Connection : Connections)
	{
		Connection->Close();
	}

	Connections.Empty();
	PendingConnections.Empty();
	ConnectionEndpointsToRemove.Empty();

	TransportHandler = nullptr;
}


bool FGameTcpMessageTransport::TransportMessage(const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context, const TArray<FGuid>& Recipients)
{
	if (Context->GetRecipients().Num() > TCP_MESSAGING_MAX_RECIPIENTS)
	{
		return false;
	}

	// Handle any queued changes to the NodeConnectionMap
	FNodeConnectionMapUpdate UpdateInfo;
	while (NodeConnectionMapUpdates.Dequeue(UpdateInfo))
	{
		check(UpdateInfo.NodeId.IsValid());
		if (UpdateInfo.bNewNode)
		{
			TSharedPtr<FGameTcpMessageTransportConnection> ConnectionPtr = UpdateInfo.Connection.Pin();
			if (ConnectionPtr.IsValid())
			{
				NodeConnectionMap.Add(UpdateInfo.NodeId, ConnectionPtr);
			}
		}
		else
		{
			NodeConnectionMap.Remove(UpdateInfo.NodeId);
		}
	}

	// Work out which connections we need to send this message to.
	TArray<TSharedPtr<FGameTcpMessageTransportConnection>> RecipientConnections;

	if (Recipients.Num() == 0)
	{
		// broadcast the message to all valid connections
		RecipientConnections = Connections.FilterByPredicate([&](const TSharedPtr<FGameTcpMessageTransportConnection>& Connection) -> bool
		{
			return Connection->GetConnectionState() == FGameTcpMessageTransportConnection::STATE_Connected;
		});
	}
	else
	{
		// Find connections for each recipient.  We do not transport unicast messages for unknown nodes.
		for (auto& Recipient : Recipients)
		{
			TSharedPtr<FGameTcpMessageTransportConnection>* RecipientConnection = NodeConnectionMap.Find(Recipient);
			if (RecipientConnection && (*RecipientConnection)->GetConnectionState() == FGameTcpMessageTransportConnection::STATE_Connected)
			{
				RecipientConnections.AddUnique(*RecipientConnection);
			}
		}
	}

	if (RecipientConnections.Num() == 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("No recipients for message %s from %s"), *Context->GetMessageType().ToString(), *Context->GetSender().ToString());
		return false;
	}

	UE_LOG(LogTemp, Verbose, TEXT("Transporting message '%s' to %d connections"), *Context->GetMessageType().ToString(), RecipientConnections.Num());

	FGameTcpSerializedMessageRef SerializedMessage = MakeShareable(new FGameTcpSerializedMessage());

	TGraphTask<FGameTcpSerializeMessageTask>::CreateTask().ConstructAndDispatchWhenReady(Context, SerializedMessage, RecipientConnections);

	return true;
}


/* FRunnable interface
*****************************************************************************/

void FGameTcpMessageTransport::Exit()
{
	// do nothing
}


bool FGameTcpMessageTransport::Init()
{
	return true;
}


uint32 FGameTcpMessageTransport::Run()
{
	while (!bStopping)
	{
		// new connections
		{
			TSharedPtr<FGameTcpMessageTransportConnection> Connection;
			while (PendingConnections.Dequeue(Connection))
			{
				Connection->OnGameTcpMessageTransportConnectionStateChanged().BindRaw(this, &FGameTcpMessageTransport::HandleConnectionStateChanged, Connection);
				Connection->Start();
				Connections.Add(Connection);
			}
		}

		// connections to remove
		{
			FIPv4Endpoint Endpoint;
			while (ConnectionEndpointsToRemove.Dequeue(Endpoint))
			{
				for (int32 Index = 0; Index < Connections.Num(); Index++)
				{
					auto& Connection = Connections[Index];

					if (Connection->GetRemoteEndpoint() == Endpoint)
					{
						Connection->Close();
						break;
					}
				}	
			}
		}

		int32 ActiveConnections = 0;
		for (int32 Index = 0;Index < Connections.Num(); Index++)
		{
			auto& Connection = Connections[Index];

			// handle disconnected by remote
			switch (Connection->GetConnectionState())
			{
			case FGameTcpMessageTransportConnection::STATE_Connected:
				ActiveConnections++;
				break;

			case FGameTcpMessageTransportConnection::STATE_Disconnected:
				Connections.RemoveAtSwap(Index);
				Index--;
				break;

			default:
				break;
			}
		}

		// incoming messages
		{
			for (auto& Connection : Connections)
			{
				TSharedPtr<FGameTcpDeserializedMessage, ESPMode::ThreadSafe> Message;
				FGuid SenderNodeId;

				while (Connection->Receive(Message, SenderNodeId))
				{
					UE_LOG(LogTemp, Verbose, TEXT("Received message '%s'"), *Message->GetMessageType().ToString());
					TransportHandler->ReceiveTransportMessage(Message.ToSharedRef(), SenderNodeId);
				}
			}
		}
				
		FPlatformProcess::Sleep(ActiveConnections > 0 ? 0.01f : 1.f);
	}

	return 0;
}


void FGameTcpMessageTransport::Stop()
{
	bStopping = true;
}


/* FGameTcpMessageTransport callbacks
*****************************************************************************/

bool FGameTcpMessageTransport::HandleListenerConnectionAccepted(FSocket* ClientSocket, const FIPv4Endpoint& ClientEndpoint)
{
	PendingConnections.Enqueue(MakeShareable(new FGameTcpMessageTransportConnection(ClientSocket, ClientEndpoint, 0)));
	
	return true;
}


void FGameTcpMessageTransport::HandleConnectionStateChanged(TSharedPtr<FGameTcpMessageTransportConnection> Connection)
{
	const FGuid NodeId = Connection->GetRemoteNodeId();
	const FIPv4Endpoint RemoteEndpoint = Connection->GetRemoteEndpoint();
	const FGameTcpMessageTransportConnection::EConnectionState State = Connection->GetConnectionState();

	if (State == FGameTcpMessageTransportConnection::STATE_Connected)
	{
		NodeConnectionMapUpdates.Enqueue(FNodeConnectionMapUpdate(true, NodeId, TWeakPtr<FGameTcpMessageTransportConnection>(Connection)));
		TransportHandler->DiscoverTransportNode(NodeId);

		UE_LOG(LogTemp, Log, TEXT("Discovered node '%s' on connection '%s'..."), *NodeId.ToString(), *RemoteEndpoint.ToString());
	}
	else if (NodeId.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Lost node '%s' on connection '%s'..."), *NodeId.ToString(), *RemoteEndpoint.ToString());

		NodeConnectionMapUpdates.Enqueue(FNodeConnectionMapUpdate(false, NodeId, TWeakPtr<FGameTcpMessageTransportConnection>(Connection)));
		TransportHandler->ForgetTransportNode(NodeId);
	}
}
