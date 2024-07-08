// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameRpcClient.h"
#include "GameRpcServer.h"
#include "IGameServiceLocator.h"

class IGameService;
class IMessageBus;
class IGameServiceLocator;
class IGameRpcServerResponder;

class IGameServiceEngine
{
public:
	/** Virtual destructor. */
	virtual ~IGameServiceEngine()
	{
	}

	template <typename ServiceType>
	TSharedPtr<ServiceType> GetServiceByName(const FString& ServiceName)
	{
		TSharedPtr<IGameServiceLocator> ServiceLocator = GetServiceLocator();
		if (ServiceLocator)
		{
			return StaticCastSharedPtr<ServiceType>(ServiceLocator->GetService(ServiceName, TEXT("")));
		}
		return nullptr;
	}

	template <typename ServiceType>
	TSharedPtr<ServiceType> GetProxyByName(const FString& ServiceName)
	{
		TSharedPtr<IGameServiceLocator> ProxyLocator = GetProxyLocator();
		if (ProxyLocator)
		{
			return StaticCastSharedPtr<ServiceType>(ProxyLocator->GetService(ServiceName, TEXT("")));
		}
		return nullptr;
	}

	template <typename ServiceType>
	TSharedPtr<ServiceType> CreateRPCService(const FString& ServiceName)
	{
		TSharedPtr<IMessageBus, ESPMode::ThreadSafe> ServiceBus = GetServiceBus();
		if (ServiceBus)
		{
			TSharedPtr<FGameRpcServer> RpcServer = MakeShared<FGameRpcServer>(ServiceName, ServiceBus.ToSharedRef());
			if (RpcServer)
			{
				TSharedPtr<ServiceType> Service = MakeShared<ServiceType>(RpcServer);
				if (Service)
				{
					Service->OnCreate();
					return Service;
				}
			}
		}
		return nullptr;
	}

	template <typename ServiceType>
	TSharedPtr<ServiceType> CreateRPCProxy(const FString& ServiceName)
	{
		TSharedPtr<IMessageBus, ESPMode::ThreadSafe> ServiceBus = GetServiceBus();
		if (ServiceBus)
		{
			TSharedPtr<FGameRpcClient> RpcClient = MakeShared<FGameRpcClient>(ServiceName, ServiceName, ServiceBus.ToSharedRef());
			if (RpcClient)
			{
				TSharedPtr<ServiceType> Proxy = MakeShared<ServiceType>(RpcClient);
				if (Proxy)
				{
					Proxy->OnCreate();
					return Proxy;
				}
			}
		}
		return nullptr;
	}

public:
	virtual bool Init() = 0;
	virtual void Start() = 0;
	virtual void Tick() = 0;
	virtual void Stop() = 0;

	/** Service message bus */
	virtual TSharedPtr<IMessageBus, ESPMode::ThreadSafe> GetServiceBus() const = 0;
	/** Service locator */
	virtual TSharedPtr<IGameServiceLocator> GetServiceLocator() const = 0;
	/** Proxy locator*/
	virtual TSharedPtr<IGameServiceLocator> GetProxyLocator() const = 0;

	/** Rpc server responder for RPC server discover */
	virtual TSharedPtr<IGameRpcServerResponder> GetRpcServerResponder() const = 0;
};
