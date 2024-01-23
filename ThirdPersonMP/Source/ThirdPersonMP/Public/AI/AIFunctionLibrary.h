// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "AIFunctionLibrary.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONMP_API UAIFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	static class ACharacter* GetBTCompOwnerCharacter(UBehaviorTreeComponent* BTComp);
};
