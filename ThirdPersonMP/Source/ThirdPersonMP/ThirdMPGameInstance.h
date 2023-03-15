// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ThirdMPGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UThirdMPGameInstance : public UGameInstance
{
	GENERATED_BODY()
public:
	virtual void OnStart() override;

	/** Override Init */
	virtual void Init() override;

	/** Override Shutdown */
	virtual void Shutdown() override;
};
