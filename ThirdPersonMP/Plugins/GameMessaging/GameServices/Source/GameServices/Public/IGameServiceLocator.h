// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Misc/WildcardString.h"

class IGameService;

class IGameServiceLocator
{
public:
	/**
	 * Configure a service.
	 *
	 * The format of the ProductId is "ProductName_Major.Minor.Patch-Changelist+Branch".
	 * Use the '*` wild card to include multiple versions, i.e. "Fortnite_1.0.*+UE4".
	 *
	 * When looking up services, the first matching configuration entry will be used. If no entry matches,
	 * nullptr will be returned.
	 *
	 * @param ServiceName The name of the service type configure.
	 * @param ProductName The product name that this configuration applies to.
	 * @param ProductVersion The version string of the product that this configuration applies to.
	 * @param ServiceModule The name of the module that implements the service.
	 */
	virtual void Configure(const FString& ServiceName, const FWildcardString& ProductId, const FName& ServiceModule) = 0;

public:
	/**
	 * Get a service of the specified type.
	 *
	 * @param ServiceType The type of service to get.
	 * @return The service instance, or nullptr if unavailable.
	 * @see GetServiceRef
	 */
	template <typename ServiceType>
	TSharedPtr<ServiceType> GetService(const FString& ProductId = TEXT(""))
	{
		return StaticCastSharedPtr<ServiceType>(GetService(TNameOf<ServiceType>::GetName(), ProductId));
	}

	template <typename ServiceType>
	TSharedPtr<ServiceType> GetServiceByName(const FString& ServiceName, const FString& ProductId = TEXT(""))
	{
		return StaticCastSharedPtr<ServiceType>(GetService(ServiceName, ProductId));
	}

	/**
	 * Get a service of the specified type.
	 *
	 * Unlike GetService(), this method will assert if a service instance of
	 * the specified type does not exist.
	 *
	 * @param ServiceType The type of service to get.
	 * @return The service instance.
	 * @see GetService
	 */
	template <typename ServiceType>
	TSharedRef<ServiceType> GetServiceRef(const FString& ProductId = TEXT(""))
	{
		return GetService<ServiceType>(ProductId).ToSharedRef();
	}


	/**
	 * Get a service of the specified type.
	 *
	 * @param ServiceName The name of the service type to get.
	 * @return The service instance, or nullptr if unavailable.
	 */
	virtual TSharedPtr<IGameService> GetService(const FString& ServiceName, const FString& ProductId) = 0;


	/**
	 * Called on start
	 */
	virtual void CreateServicesOnStart()
	{
	}

public:
	/** Virtual destructor. */
	virtual ~IGameServiceLocator()
	{
	}

	/** For debug */
	virtual void DumpServices()
	{
	}
};

typedef TSharedPtr<IGameServiceLocator> IGameServiceLocatorPtr;