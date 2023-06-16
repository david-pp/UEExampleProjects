#pragma once

#include "CoreMinimal.h"
#include "GASDocumentation/GASDocumentation.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitInputAction.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInputActionDelegate, float, TimeWaited);

UCLASS()
class GASDOCUMENTATION_API UAbilityTask_WaitInputAction : public UAbilityTask
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintAssignable)
	FInputActionDelegate OnPress;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	EGDAbilityInputID InputID;


	EAbilityGenericReplicatedEvent::Type GetReplicatedEventType() const;

	UFUNCTION()
	void OnPressCallback();

	virtual void Activate() override;

	/** Wait until the user presses the input button for this ability's activation. Returns time this node spent waiting for the press. Will return 0 if input was already down. */
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitInputAction* WaitInputAction(UGameplayAbility* OwningAbility, EGDAbilityInputID TheInputID, bool bTestAlreadyPressed = false);

protected:
	float StartTime;
	bool bTestInitialState;
	FDelegateHandle DelegateHandle;
};
