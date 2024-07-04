#pragma once

#include "GameNatsMessages.h"
#include "IMessageTransport.h"
#include "NatsClient.h"

class GAMENATSMESSAGING_API FGameNatsMessageTransport : public IMessageTransport
{
public:
	/**
	 * Creates and initializes a new instance.
	 */
	FGameNatsMessageTransport(const FString& InNatsNodeName, const FString& InNatsURL = TEXT("nats://127.0.0.1:4222"), const FGuid& InNodeId = FGuid::NewGuid());

	/** Virtual destructor. */
	virtual ~FGameNatsMessageTransport();

	void DispatchMessageCallbacks();

	FString GetClientStatusChannel() const
	{
		return NATS_CLIENT_STATUS_CHANNEL;
	}

	FString GetPublicMessageChannel() const
	{
		return NATS_PUBLIC_CHANNEL;
	}

	FString GetPrivateMessageChannel() const
	{
		return FString::Printf(TEXT("%s.%s"), NATS_CLIENT_PRIVATE_CHANNEL, *NatsNodeName);
	}

	void PublishClientStatus();

public:
	//~ IMessageTransport interface
	virtual FName GetDebugName() const override
	{
		return FName(NatsNodeName);
	}

	virtual bool StartTransport(IMessageTransportHandler& Handler) override;
	virtual void StopTransport() override;
	virtual bool TransportMessage(const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context, const TArray<FGuid>& Recipients) override;

protected:
	void HandleGameMessage(const char* DataPtr, int32 DataLength);
	void HandleClientStatusMessage(const char* DataPtr, int32 DataLength);

	void UpdateRemoteClientStatus();

private:
	/** Message transport handler. */
	IMessageTransportHandler* TransportHandler = nullptr;

	/** Nats client name */
	FString NatsNodeName;
	/** Local Node Id */
	FGuid NatsNodeId;

	/** Nats client */
	TSharedPtr<INatsClient> NatsClient;
	/** Nats Server URL */
	FString NatServerURL;

	/** Nats client status broadcast time interval */
	float NatsNodeStatusBroadcastInterval = 1.0;
	FDelegateHandle NatsNodeStatusTicker;

	/** Remote Nats Node */
	TMap<FGuid, FGameNatsNodeStatus> RemoteNatsNodes;
	float RemoteNatsNodeTimeoutSeconds = 10.0;
};
