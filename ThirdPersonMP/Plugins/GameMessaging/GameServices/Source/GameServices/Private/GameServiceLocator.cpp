// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameServiceLocator.h"
#include "Misc/WildcardString.h"
#include "Modules/ModuleManager.h"
#include "IGameService.h"
#include "IGameServiceLocator.h"
#include "IGameServiceProvider.h"
#include "IGameServicesModule.h"

class FGameServiceLocatorImpl : public IGameServiceLocator
{
public:
	~FGameServiceLocatorImpl()
	{
	}

public:
	// IGameServiceLocator interface

	virtual void Configure(const FString& ServiceName, const FWildcardString& ProductId, const FName& ServiceModule) override
	{
		TArray<FConfigEntry>& Entries = Configuration.FindOrAdd(ServiceName);
		FConfigEntry Entry;
		{
			Entry.ProductId = ProductId;
			Entry.ServiceModule = ServiceModule;
		}

		Entries.Add(Entry);
	}

	virtual TSharedPtr<IGameService> GetService(const FString& ServiceName, const FString& ProductId) override
	{
		TArray<FConfigEntry>& Entries = Configuration.FindOrAdd(ServiceName);

		for (FConfigEntry& Entry : Entries)
		{
			if (!Entry.ProductId.IsMatch(ProductId))
			{
				continue;
			}

			if (Entry.ServiceInstance.IsValid())
			{
				return Entry.ServiceInstance;
			}

			auto ServiceProvider = FModuleManager::LoadModulePtr<IGameServiceProvider>(Entry.ServiceModule);

			if (ServiceProvider == nullptr)
			{
				continue;
			}

			Entry.ServiceInstance = ServiceProvider->GetService(ServiceName, ServiceDependencies.ToSharedRef());

			if (Entry.ServiceInstance.IsValid())
			{
				return Entry.ServiceInstance;
			}
		}

		return nullptr;
	}

	virtual void DumpServices() override
	{
		for (auto& KV : Configuration)
		{
			FString EntriesString = FString::JoinBy(KV.Value, TEXT("|"), &FConfigEntry::ToDebugString);
			UE_LOG(LogGameServices, Warning, TEXT("ServiceLocator - %s -> %s"), *KV.Key, *EntriesString);
		}
	}

private:
	/**
	 * Create and initialize a new instance.
	 *
	 * @param InServiceDependencies A type container for optional service dependencies.
	 */
	FGameServiceLocatorImpl(const TSharedRef<FTypeContainer>& InServiceDependencies)
		: ServiceDependencies(InServiceDependencies)
	{
	}

private:
	struct FConfigEntry
	{
		FWildcardString ProductId;
		TSharedPtr<IGameService> ServiceInstance;
		FName ServiceModule;

		FString ToDebugString() const
		{
			return FString::Printf(TEXT("Wildcard:%s, Module:%s, ServiceInstance:%s"), *ProductId, *ServiceModule.ToString(), ServiceInstance ? *ServiceInstance->GetDebugName().ToString() : TEXT("nullptr"));
		}
	};

	/** Holds the service configuration entries. */
	TMap<FString, TArray<FConfigEntry>> Configuration;

	/** Optional service dependencies. */
	TSharedPtr<FTypeContainer> ServiceDependencies;

	friend FGameServiceLocatorFactory;
};

TSharedRef<IGameServiceLocator> FGameServiceLocatorFactory::Create(const TSharedRef<FTypeContainer>& ServiceDependencies)
{
	return MakeShareable(new FGameServiceLocatorImpl(ServiceDependencies));
}
