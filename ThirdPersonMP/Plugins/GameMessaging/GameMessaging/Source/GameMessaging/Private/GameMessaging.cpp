// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameMessaging.h"

#include "GameRpcServerLocator.h"
#include "GameRpcServerResponder.h"

#define LOCTEXT_NAMESPACE "FGameMessagingModule"

class FGameMessagingModuleImpl : public IGameMessagingModule
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

public:
	virtual TSharedRef<IGameRpcServerLocator> CreateLocator(const FName& InName, const FString& ServiceKey, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& ServiceBus) override
	{
		return IGameRpcServerLocator::Create(InName, ServiceKey, ServiceBus);
	}

	virtual TSharedRef<IGameRpcServerResponder> CreateResponder(const FName& InName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& ServiceBus) override
	{
		return IGameRpcServerResponder::Create(InName, ServiceBus);
	}

	// virtual TSharedRef<IGameServiceRpcServer> CreateServer(const FString& DebugName, const TSharedRef<IMessageBus, ESPMode::ThreadSafe>& MessageBus) override
	// {
	// 	return FGameServiceRpcServerFactory::Create(DebugName, MessageBus);
	// }
	//
	// virtual TSharedRef<IGameServiceRpcClient> CreateClient(const FString& ClientName, const FString& ServiceKey, const FServiceMessageBusRef& ServiceBus) override
	// {
	// 	return MakeShared<FGameServiceRpcClient>(ClientName, ServiceKey, ServiceBus);
	// }
};

void FGameMessagingModuleImpl::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FGameMessagingModuleImpl::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameMessagingModuleImpl, GameMessaging)
