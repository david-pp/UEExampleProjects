// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMasterService.h"
#include "DSMasterServiceActor.generated.h"

/**
 * 
 */
UCLASS()
class DSMASTER_API ADSMasterServiceActor : public AActor
{
	GENERATED_BODY()

public:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

protected:
	TSharedPtr<FDSMasterService> DSMasterService;
};
