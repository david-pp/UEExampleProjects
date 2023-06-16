// Copyright 2020 Dan Kestranek.


#include "Characters/Abilities/AbilityTasks/AbilityTask_WaitInputAction.h"
#include "AbilitySystemComponent.h"
#include "Characters/Abilities/GDAbilitySystemComponent.h"

UAbilityTask_WaitInputAction::UAbilityTask_WaitInputAction(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	StartTime = 0.f;
	bTestInitialState = false;
}

EAbilityGenericReplicatedEvent::Type UAbilityTask_WaitInputAction::GetReplicatedEventType() const
{
	return InputID2EventType(InputID);
}

void UAbilityTask_WaitInputAction::OnPressCallback()
{
	float ElapsedTime = GetWorld()->GetTimeSeconds() - StartTime;

	if (!Ability || !AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->AbilityReplicatedEventDelegate(GetReplicatedEventType(), GetAbilitySpecHandle(), GetActivationPredictionKey()).Remove(DelegateHandle);

	FScopedPredictionWindow ScopedPrediction(AbilitySystemComponent, IsPredictingClient());

	if (IsPredictingClient())
	{
		// Tell the server about this
		AbilitySystemComponent->ServerSetReplicatedEvent(GetReplicatedEventType(), GetAbilitySpecHandle(), GetActivationPredictionKey(), AbilitySystemComponent->ScopedPredictionKey);
	}
	else
	{
		AbilitySystemComponent->ConsumeGenericReplicatedEvent(GetReplicatedEventType(), GetAbilitySpecHandle(), GetActivationPredictionKey());
	}

	// We are done. Kill us so we don't keep getting broadcast messages
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnPress.Broadcast(ElapsedTime);
	}
	EndTask();
}

UAbilityTask_WaitInputAction* UAbilityTask_WaitInputAction::WaitInputAction(class UGameplayAbility* OwningAbility,EGDAbilityInputID TheInputID, bool bTestAlreadyPressed)
{
	UAbilityTask_WaitInputAction* Task = NewAbilityTask<UAbilityTask_WaitInputAction>(OwningAbility);
	Task->bTestInitialState = bTestAlreadyPressed;
	Task->InputID = TheInputID;
	return Task;
}

void UAbilityTask_WaitInputAction::Activate()
{
	StartTime = GetWorld()->GetTimeSeconds();
	if (Ability)
	{
		if (bTestInitialState && IsLocallyControlled())
		{
			FGameplayAbilitySpec* Spec = Ability->GetCurrentAbilitySpec();
			if (Spec && Spec->InputPressed)
			{
				OnPressCallback();
				return;
			}
		}

		DelegateHandle = AbilitySystemComponent->AbilityReplicatedEventDelegate(GetReplicatedEventType(), GetAbilitySpecHandle(), GetActivationPredictionKey()).AddUObject(this, &UAbilityTask_WaitInputAction::OnPressCallback);
		if (IsForRemoteClient())
		{
			if (!AbilitySystemComponent->CallReplicatedEventDelegateIfSet(GetReplicatedEventType(), GetAbilitySpecHandle(), GetActivationPredictionKey()))
			{
				SetWaitingOnRemotePlayerData();
			}
		}
	}
}
