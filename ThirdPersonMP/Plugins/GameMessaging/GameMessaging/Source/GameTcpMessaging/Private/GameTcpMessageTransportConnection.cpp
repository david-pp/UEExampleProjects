// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameTcpMessageTransportConnection.h"
#include "Serialization/ArrayWriter.h"
#include "Common/TcpSocketBuilder.h"
#include "GameTcpMessagingPrivate.h"
#include "GameTcpDeserializedMessage.h"
#include "Misc/ScopeLock.h"

/** Header sent over the connection as soon as it's opened */
struct FGameTcpMessageHeader
{
	uint32 MagicNumber;
	uint32 Version;
	FGuid NodeId;

	FGameTcpMessageHeader()
	:	MagicNumber(0)
	,	Version(0)
	{}

	FGameTcpMessageHeader(const FGuid& InNodeId)
	:	MagicNumber(TCP_MESSAGING_TRANSPORT_PROTOCOL_MAGIC)
	,	Version(EGameTcpMessagingVersion::LatestVersion)
	,	NodeId(InNodeId)
	{}

	bool IsValid() const
	{
		return
			MagicNumber == TCP_MESSAGING_TRANSPORT_PROTOCOL_MAGIC &&
			Version == EGameTcpMessagingVersion::OldestSupportedVersion &&
			NodeId.IsValid();
	}

	FGuid GetNodeId() const
	{
		return NodeId;
	}

	uint32 GetVersion() const
	{
		return Version;
	}

	// Serializer
	friend FArchive& operator<<(FArchive& Ar, FGameTcpMessageHeader& H)
	{
		return Ar << H.MagicNumber << H.Version << H.NodeId;
	}
};


/* FGameTcpMessageTransportConnection structors
 *****************************************************************************/

FGameTcpMessageTransportConnection::FGameTcpMessageTransportConnection(FSocket* InSocket, const FIPv4Endpoint& InRemoteEndpoint, int32 InConnectionRetryDelay)
	: ConnectionState(STATE_Connecting)
	, OpenedTime(FDateTime::UtcNow())
	, RemoteEndpoint(InRemoteEndpoint)
	, LocalNodeId(FGuid::NewGuid())
	, bSentHeader(false)
	, bReceivedHeader(false)
	, RemoteProtocolVersion(0)
	, Socket(InSocket)
	, Thread(nullptr)
	, TotalBytesReceived(0)
	, TotalBytesSent(0)
	, bRun(false)
	, ConnectionRetryDelay(InConnectionRetryDelay)
	, RecvMessageDataRemaining(0)
{
	int32 NewSize = 0;
	Socket->SetReceiveBufferSize(TCP_MESSAGING_RECEIVE_BUFFER_SIZE, NewSize);
	Socket->SetSendBufferSize(TCP_MESSAGING_SEND_BUFFER_SIZE, NewSize);
}

FGameTcpMessageTransportConnection::~FGameTcpMessageTransportConnection()
{
	if (Thread != nullptr)
	{
		if(bRun)
		{
			bRun = false;
			Thread->WaitForCompletion();
		}
		delete Thread;
	}

	if (Socket)
	{
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
	}
}

/* FGameTcpMessageTransportConnection interface
 *****************************************************************************/

void FGameTcpMessageTransportConnection::Start()
{
	check(Thread == nullptr);
	bRun = true;
	Thread = FRunnableThread::Create(this, *FString::Printf(TEXT("FGameTcpMessageTransportConnection %s"), *RemoteEndpoint.ToString()), 128 * 1024, TPri_Normal);
}

bool FGameTcpMessageTransportConnection::Receive(TSharedPtr<FGameTcpDeserializedMessage, ESPMode::ThreadSafe>& OutMessage, FGuid& OutSenderNodeId)
{
	if(Inbox.Dequeue(OutMessage))
	{
		OutSenderNodeId = RemoteNodeId;
		return true;
	}
	return false;
}


bool FGameTcpMessageTransportConnection::Send(FGameTcpSerializedMessagePtr Message)
{
	FScopeLock SendLock(&SendCriticalSection);

	if (GetConnectionState() == STATE_Connected && bSentHeader)
	{
		int32 BytesSent = 0;
		const TArray<uint8>& Payload = Message->GetDataArray();

		// send the payload size
		FArrayWriter MessagesizeData = FArrayWriter(true);
		uint32 Messagesize = Payload.Num();
		MessagesizeData << Messagesize;

		if (!BlockingSend(MessagesizeData.GetData(), sizeof(uint32)))
		{
			UE_LOG(LogTemp, Verbose, TEXT("Payload size write failed with code %d"), (int32)ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode());
			return false;
		}

		TotalBytesSent += sizeof(uint32);

		// send the payload
		if (!BlockingSend(Payload.GetData(), Payload.Num()))
		{
			UE_LOG(LogTemp, Verbose, TEXT("Payload write failed with code %d"), (int32)ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->GetLastErrorCode());
			return false;
		}

		TotalBytesSent += Payload.Num();

		return true;
	}

	return false;
}


/* FRunnable interface
 *****************************************************************************/

bool FGameTcpMessageTransportConnection::Init()
{
	return (Socket != nullptr);
}

uint32 FGameTcpMessageTransportConnection::Run()
{
	UE_LOG(LogTemp, Log, TEXT("Started Connection to '%s'"), *RemoteEndpoint.ToString());

	while (bRun)
	{
		// Try sending the header if needed, and receiving messages and detect if they fail or if another connection error is reported.
		if ((!SendHeader() || !ReceiveMessages() || Socket->GetConnectionState() == SCS_ConnectionError) && bRun)
		{
			// Disconnected. Reconnect if requested.
			if (ConnectionRetryDelay > 0)
			{
				bool bReconnectPending = false;

				{
				    // Wait for any sending before we close the socket
				    FScopeLock SendLock(&SendCriticalSection);
    
				    Socket->Close();
				    ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
				    Socket = nullptr;
    
				    UE_LOG(LogTemp, Verbose, TEXT("Connection to '%s' failed, retrying..."), *RemoteEndpoint.ToString());
				    FPlatformProcess::Sleep(ConnectionRetryDelay);
    
				    Socket = FTcpSocketBuilder(TEXT("FGameTcpMessageTransport.RemoteConnection"))
						.WithSendBufferSize(TCP_MESSAGING_SEND_BUFFER_SIZE)
						.WithReceiveBufferSize(TCP_MESSAGING_RECEIVE_BUFFER_SIZE);

				    if (Socket && Socket->Connect(RemoteEndpoint.ToInternetAddr().Get()))
				    {
					    bSentHeader = false;
					    bReceivedHeader = false;
						ConnectionState = STATE_DisconnectReconnectPending;
					    RemoteNodeId.Invalidate();
						bReconnectPending = true;
				    }
				    else
				    {
					    if (Socket)
					    {
						    ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
						    Socket = nullptr;
					    }
					    bRun = false;
				    }
				}

				if (bReconnectPending)
				{
					ConnectionStateChangedDelegate.ExecuteIfBound();
				}
			}
			else
			{
				bRun = false;
			}
		}

		FPlatformProcess::SleepNoStats(0.0001f);
	}

	{
		FScopeLock SendLock(&SendCriticalSection);
		ConnectionState = STATE_Disconnected;
	}
	ConnectionStateChangedDelegate.ExecuteIfBound();
	
	RemoteNodeId.Invalidate();
	ClosedTime = FDateTime::UtcNow();

	// Clear the delegate to remove a reference to this connection
	ConnectionStateChangedDelegate.Unbind();
	return 0;
}

void FGameTcpMessageTransportConnection::Stop()
{
	if (Socket)
	{
		Socket->Close();
	}
}

void FGameTcpMessageTransportConnection::Exit()
{
	// do nothing
}


/* FGameTcpMessageTransportConnection implementation
 *****************************************************************************/

void FGameTcpMessageTransportConnection::Close()
{
	// let the thread shutdown on its own
	if (Thread != nullptr)
	{
		bRun = false;
		Thread->WaitForCompletion();
		delete Thread;
		Thread = nullptr;
	}

	// if there a socket, close it so our peer will get a quick disconnect notification
	if (Socket)
	{
		Socket->Close();
	}
}

uint64 FGameTcpMessageTransportConnection::GetTotalBytesReceived() const
{
	return TotalBytesReceived;
}

uint64 FGameTcpMessageTransportConnection::GetTotalBytesSent() const
{
	return TotalBytesSent;
}

FText FGameTcpMessageTransportConnection::GetName() const
{
	return RemoteEndpoint.ToText();
}

FTimespan FGameTcpMessageTransportConnection::GetUptime() const
{
	if (ConnectionState == STATE_Connected)
	{
		return (FDateTime::UtcNow() - OpenedTime);
	}
		
	return (ClosedTime - OpenedTime);
}

FGameTcpMessageTransportConnection::EConnectionState FGameTcpMessageTransportConnection::GetConnectionState() const
{
	return ConnectionState;
}

FGuid FGameTcpMessageTransportConnection::GetRemoteNodeId() const
{
	return RemoteNodeId;
}

bool FGameTcpMessageTransportConnection::ReceiveMessages()
{
	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	uint32 PendingDataSize = 0;
	
	auto GetReadableErrorCode = [SocketSubsystem]() -> FString
	{
		ESocketErrors LastError = SocketSubsystem->GetLastErrorCode();
		return FString::Printf(TEXT("%s (%d)"), SocketSubsystem->GetSocketError(LastError), (int32)LastError);
	};

	// check if the socket has closed
	{
		int32 BytesRead;
		uint8 Dummy;
		if (!Socket->Recv(&Dummy, 1, BytesRead, ESocketReceiveFlags::Peek))
		{
			UE_LOG(LogTemp, Verbose, TEXT("Dummy read failed with code %s"), *GetReadableErrorCode());
			return false;
		}
	}
	
	// Block waiting for some data
	if (!Socket->Wait(ESocketWaitConditions::WaitForRead, FTimespan::FromSeconds(1.0)))
	{
		return (Socket->GetConnectionState() != SCS_ConnectionError);
	}
	
	if (!bReceivedHeader)
	{
		if (Socket->HasPendingData(PendingDataSize) && PendingDataSize >= sizeof(FGameTcpMessageHeader))
		{
			FArrayReader HeaderData = FArrayReader(true);
			HeaderData.SetNumUninitialized(sizeof(FGameTcpMessageHeader));
			int32 BytesRead = 0;
			if (!Socket->Recv(HeaderData.GetData(), sizeof(FGameTcpMessageHeader), BytesRead))
			{
				UE_LOG(LogTemp, Verbose, TEXT("Header read failed with code %s"), *GetReadableErrorCode());
				return false;
			}

			check(BytesRead == sizeof(FGameTcpMessageHeader));
			TotalBytesReceived += BytesRead;

			FGameTcpMessageHeader MessageHeader;
			HeaderData << MessageHeader;

			if (!MessageHeader.IsValid())
			{
				UE_LOG(LogTemp, Verbose, TEXT("Header read failed with invalid header"));
				return false;
			}
			else
			{
				RemoteNodeId = MessageHeader.GetNodeId();
				RemoteProtocolVersion = MessageHeader.GetVersion();
				bReceivedHeader = true;
				OpenedTime = FDateTime::UtcNow();
	            {
		            FScopeLock SendLock(&SendCriticalSection);
		            ConnectionState = STATE_Connected;
				}
	            ConnectionStateChangedDelegate.ExecuteIfBound();
			}
		}
		else
		{
			// no header yet
			return true;
		}
	}

	// keep going until we have no data.
	for(;;)
	{
		int32 BytesRead = 0;
		// See if we're in the process of receiving a (large) message
		if (RecvMessageDataRemaining == 0)
		{
			// no partial message. Try to receive the size of a message
			if (!Socket->HasPendingData(PendingDataSize) || (PendingDataSize < sizeof(uint32)))
			{
				// no messages
				return true;
			}

			FArrayReader MessagesizeData = FArrayReader(true);
			MessagesizeData.SetNumUninitialized(sizeof(uint32));

			// read message size from the stream
			BytesRead = 0;
			if (!Socket->Recv(MessagesizeData.GetData(), sizeof(uint32), BytesRead))
			{
				UE_LOG(LogTemp, Verbose, TEXT("In progress read failed with code %s"), *GetReadableErrorCode());
				return false;
			}

			check(BytesRead == sizeof(uint32));
			TotalBytesReceived += BytesRead;

			// Setup variables to receive the message
			MessagesizeData << RecvMessageDataRemaining;

			if (RecvMessageDataRemaining <= 0)
			{
				UE_LOG(LogTemp, Error, TEXT("Read failed due to invalid Message Size: %d"), RecvMessageDataRemaining);
				return false;
			}

			RecvMessageData = MakeShareable(new FArrayReader(true));
			RecvMessageData->SetNumUninitialized(RecvMessageDataRemaining);
		}

		BytesRead = 0;
		if (!Socket->Recv(RecvMessageData->GetData() + RecvMessageData->Num() - RecvMessageDataRemaining, RecvMessageDataRemaining, BytesRead))
		{
			UE_LOG(LogTemp, Verbose, TEXT("Read failed with code %s"), *GetReadableErrorCode());
			return false;
		}

		if (BytesRead > 0)
		{
			check(BytesRead <= RecvMessageDataRemaining);
			TotalBytesReceived += BytesRead;
			RecvMessageDataRemaining -= BytesRead;
			if (RecvMessageDataRemaining == 0)
			{
				// @todo gmp: move message deserialization into an async task
				FGameTcpDeserializedMessage* DeserializedMessage = new FGameTcpDeserializedMessage(nullptr);
				if (DeserializedMessage->Deserialize(RecvMessageData))
				{
					Inbox.Enqueue(MakeShareable(DeserializedMessage));
				}
				RecvMessageData.Reset();
			}
		}
		else
		{
			// no data
			return true;
		}
	}
}

bool FGameTcpMessageTransportConnection::BlockingSend(const uint8* Data, int32 BytesToSend)
{
	int32 TotalBytes = BytesToSend;
	while (BytesToSend > 0)
	{
		while (!Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromSeconds(1.0)))
		{
			if (Socket->GetConnectionState() == SCS_ConnectionError)
			{
				return false;
			}
		}

		int32 BytesSent = 0;
		if (!Socket->Send(Data, BytesToSend, BytesSent))
		{
			return false;
		}
		BytesToSend -= BytesSent;
		Data += BytesSent;
	}
	return true;
}

bool FGameTcpMessageTransportConnection::SendHeader()
{
	if (bSentHeader)
	{
		return true;
	}

	FScopeLock SendLock(&SendCriticalSection);

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

	// See if we're writable
	if (!Socket->Wait(ESocketWaitConditions::WaitForWrite, FTimespan::FromSeconds(5)))
	{
		return true;
	}

	FArrayWriter HeaderData;
	FGameTcpMessageHeader MessageHeader(LocalNodeId);
	HeaderData << MessageHeader;

	if (!BlockingSend(HeaderData.GetData(), sizeof(FGameTcpMessageHeader)))
	{
		UE_LOG(LogTemp, Verbose, TEXT("Header write failed with code %d"), (int32)SocketSubsystem->GetLastErrorCode());
		return false;
	}

	bSentHeader = true;
	TotalBytesSent += sizeof(FGameTcpMessageHeader);

	return true;
}

