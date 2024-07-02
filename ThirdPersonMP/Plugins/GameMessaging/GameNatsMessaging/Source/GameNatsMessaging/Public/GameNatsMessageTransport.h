#pragma once

#include "IMessageTransport.h"
#include "NatsClient.h"

class GAMENATSMESSAGING_API FGameNatsMessageTransport : public IMessageTransport
{
public:
	/**
	 * Creates and initializes a new instance.
	 */
	FGameNatsMessageTransport(const FString& InNatsURL = TEXT("nats://127.0.0.1:4222"), const FString& InDefaultSubject = TEXT("GameNatsTransport"));

	/** Virtual destructor. */
	virtual ~FGameNatsMessageTransport();

	void DispatchMessageCallbacks();

public:
	//~ IMessageTransport interface

	virtual FName GetDebugName() const override
	{
		return "NatsMessageTransport";
	}

	virtual bool StartTransport(IMessageTransportHandler& Handler) override;
	virtual void StopTransport() override;
	virtual bool TransportMessage(const TSharedRef<IMessageContext, ESPMode::ThreadSafe>& Context, const TArray<FGuid>& Recipients) override;

protected:
	void HandleDefaultSubjectMessage(const char* DataPtr, int32 DataLength);
	
private:
	/** Message transport handler. */
	IMessageTransportHandler* TransportHandler = nullptr;

	/** Nats client */
	TSharedPtr<INatsClient> NatsClient;
	/** Nats client default subject to subscribe */
	FString DefaultSubject;

	/** Nats Server URL */
	FString NatServerURL;
};
