// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMasterService.h"
#include "DSMasterServiceSubsystem.generated.h"

/**
 * 
 */
UCLASS(Blueprintable)
class DSMASTER_API UDSMasterServiceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// ~BEGIN USubsystem Interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// ~BEGIN USubsystem Interface

	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	void OnWorldInitialized(UWorld* World, const UWorld::InitializationValues IVS);
	void OnWorldBeginPlay();

	/** Called as part of UWorld::BeginTearingDown */
	void OnBeginTearingDown(UWorld* World);

protected:
	TSharedPtr<FDSMasterService> DSMasterService;
};
