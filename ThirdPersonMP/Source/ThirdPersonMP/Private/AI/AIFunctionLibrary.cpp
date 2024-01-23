// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/AIFunctionLibrary.h"

#include "AIController.h"
#include "GameFramework/Character.h"


ACharacter* UAIFunctionLibrary::GetBTCompOwnerCharacter(UBehaviorTreeComponent* BTComp)
{
	if (BTComp)
	{
		// first, for AI
		AAIController* AIController = BTComp->GetAIOwner();
		if (AIController && AIController->GetPawn())
		{
			return Cast<ACharacter>(AIController->GetPawn());
		}

		// second, check BTComp's Owner
		return Cast<ACharacter>(BTComp->GetOwner());
	}

	return nullptr;
}
