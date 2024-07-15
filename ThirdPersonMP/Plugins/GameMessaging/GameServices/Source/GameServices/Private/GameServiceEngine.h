#pragma once

#include "CoreMinimal.h"

#include "GameRpcServerResponder.h"
#include "GameServiceSettings.h"
#include "IGameServiceEngine.h"
#include "IGameServiceLocator.h"
#include "IMessageBridge.h"
#include "Misc/TypeContainer.h"

class IMessageRpcClient;
class IGameServiceRpcLocator;
class IGameServiceLocator;
class IGameRpcServerResponder;

class FGameTcpMessageTransport;
class FGameNatsMessageTransport;

class FGameServicesEngine : public IGameServiceEngine
{
public:
	FGameServicesEngine()
	{
		bLoadSettingFromConfigFile = true;
	}

	FGameServicesEngine(const FGameServiceEngineSettings& Settings)
	{
		bLoadSettingFromConfigFile = false;
		EngineSettings = Settings;
	}
	
	/** Virtual destructor. */
	virtual ~FGameServicesEngine()
	{
	}

	virtual bool Init() override;
	virtual void Start() override;
	virtual void Tick() override;
	virtual void Stop() override;

	virtual TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetServiceBus() const override
	{
		return ServiceBus;
	}

	virtual TSharedPtr<IGameServiceLocator> GetServiceLocator() const override
	{
		return ServiceLocator;
	}

	virtual TSharedPtr<IGameServiceLocator> GetProxyLocator() const override
	{
		return ProxyLocator;
	}

	virtual TSharedPtr<IGameRpcServerResponder> GetRpcServerResponder() const override
	{
		return RpcServerResponder;
	}

	bool InitServiceBus(const FGameServiceMessageBusSettings& BusSettings);
	bool InitServices();
	bool InitRpcServerResponder();

	void DumpServices();

protected:
	static FString GetSettingFileName();
	bool LoadSettingFromJsonFile(const FString& JsonFileName);
	bool SaveSettingToJsonFile(const FString& JsonFileName);

	bool bLoadSettingFromConfigFile = true;

	/** Service settings */
	FGameServiceEngineSettings EngineSettings;

protected:
	/** Rpc server responder */
	TSharedPtr<IGameRpcServerResponder> RpcServerResponder;

	/** Holds a type container for service dependencies. */
	TSharedPtr<FTypeContainer> ServiceDependencies;

	/** Holds registered service instances. */
	TSharedPtr<IGameServiceLocator> ServiceLocator;
	/** Holds registered proxy instances. */
	TSharedPtr<IGameServiceLocator> ProxyLocator;

protected:
	TSharedPtr<IMessageBus, ESPMode::ThreadSafe> ServiceBus;

	TSharedPtr<IMessageBridge, ESPMode::ThreadSafe> TcpBridge;
	TSharedPtr<FGameTcpMessageTransport, ESPMode::ThreadSafe> TcpTransport;

	TSharedPtr<IMessageBridge, ESPMode::ThreadSafe> NatsBridge;
	TSharedPtr<FGameNatsMessageTransport, ESPMode::ThreadSafe> NatsTransport;

	TSharedPtr<IGameRpcServerResponder> RpcResponder;

	/** Handle to the registered ticker. */
	FDelegateHandle TickerHandle;
};
