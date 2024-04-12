#include "CoreMinimal.h"
#include "IGameServiceEngine.h"
#include "Misc/TypeContainer.h"

class IGameServiceRpcResponder;
class IMessageRpcClient;
class IGameServiceRpcLocator;
class IGameServiceLocator;

class FGameServicesEngine : public IGameServiceEngine
{
public:
	/** Virtual destructor. */
	virtual ~FGameServicesEngine()
	{
	}

	virtual void Init() override
	{
		// InitializeGameServices();
		InitializeGameServers();
	}

	virtual void Start() override;
	virtual void Stop() override;

	virtual TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetServiceBus() const override
	{
		return ServiceBus;
	}

	void InitializeGameServices();

	void InitializeGameServers();

protected:
	/** Game Service RPC client. */
	TSharedPtr<IMessageRpcClient> ServiceRpcClient;

	/** Game Service RPC server locator. */
	TSharedPtr<IGameServiceRpcLocator> ServiceRpcLocator;

	/** Holds a type container for service dependencies. */
	TSharedPtr<FTypeContainer> ServiceDependencies;

	/** Holds registered service instances. */
	TSharedPtr<IGameServiceLocator> ServiceLocator;

protected:
	TSharedPtr<IMessageBus, ESPMode::ThreadSafe> ServiceBus;

	TSharedPtr<IGameServiceRpcResponder> RpcResponder;
};
