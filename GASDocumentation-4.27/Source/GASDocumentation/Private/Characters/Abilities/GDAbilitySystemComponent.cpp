// Copyright 2020 Dan Kestranek.


#include "Characters/Abilities/GDAbilitySystemComponent.h"

#include "Characters/Abilities/GDGameplayAbility.h"
#include "Engine/PackageMapClient.h"

void UGDAbilitySystemComponent::ReceiveDamage(UGDAbilitySystemComponent* SourceASC, float UnmitigatedDamage, float MitigatedDamage)
{
	ReceivedDamage.Broadcast(SourceASC, UnmitigatedDamage, MitigatedDamage);
}

void UGDAbilitySystemComponent::OnServerPrintDebug_Request()
{
	Super::OnServerPrintDebug_Request();
}

void UGDAbilitySystemComponent::OnClientPrintDebug_Response(const TArray<FString>& Strings, int32 GameFlags)
{
	Super::OnClientPrintDebug_Response(Strings, GameFlags);
}

void UGDAbilitySystemComponent::DebugServerAsc()
{
	ServerPrintDebug_Request();

	AActor* Owner = GetOwnerActor();
	if (Owner)
	{
		FNetworkGUID GUID = Owner->GetWorld()->GetNetDriver()->GuidCache->GetNetGUID(Owner);
		ABILITY_LOG(Display, TEXT("GUID=%d, %d"), GUID.Value, Owner->GetUniqueID());
	}
}

void UGDAbilitySystemComponent::AbilityLocalInputPressed(int32 InputID)
{
	FGameplayAbilitySpec* ActiveSpec = FindAbilitySpecFromHandle(ActiveAbilitySpecHandle);
	if (ActiveSpec && ActiveSpec->IsActive())
	{
		EAbilityGenericReplicatedEvent::Type EventType = InputID2EventType((EGDAbilityInputID)InputID);
		// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
		InvokeReplicatedEvent(EventType, ActiveSpec->Handle, ActiveSpec->ActivationInfo.GetActivationPredictionKey());

		// if (Spec->Ability->bReplicateInputDirectly && IsOwnerActorAuthoritative() == false)
		// {
		// 	ServerSetInputPressed(Spec.Handle);
		// }
		//
		// AbilitySpecInputPressed(Spec);
		// return;


		// AbilitySpecInputPressed(*Spec);

		//
		// FVector_NetQuantize100 Payload;
		// Payload.X = InputID;

		// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
		// InvokeReplicatedEventWithPayload(EAbilityGenericReplicatedEvent::InputPressed, Spec->Handle, Spec->ActivationInfo.GetActivationPredictionKey(), FPredictionKey(), Payload);
	}

	// Consume the input if this InputID is overloaded with GenericConfirm/Cancel and the GenericConfim/Cancel callback is bound
	if (IsGenericConfirmInputBound(InputID))
	{
		LocalInputConfirm();
		return;
	}

	if (IsGenericCancelInputBound(InputID))
	{
		LocalInputCancel();
		return;
	}

	// ---------------------------------------------------------

	ABILITYLIST_SCOPE_LOCK();
	for (FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.InputID == InputID)
		{
			if (Spec.Ability)
			{
				Spec.InputPressed = true;
				if (Spec.IsActive())
				{
					if (Spec.Ability->bReplicateInputDirectly && IsOwnerActorAuthoritative() == false)
					{
						ServerSetInputPressed(Spec.Handle);
					}

					AbilitySpecInputPressed(Spec);

					// Invoke the InputPressed event. This is not replicated here. If someone is listening, they may replicate the InputPressed event to the server.
					InvokeReplicatedEvent(EAbilityGenericReplicatedEvent::InputPressed, Spec.Handle, Spec.ActivationInfo.GetActivationPredictionKey());
				}
				else
				{
					// Ability is not active, so try to activate it
					TryActivateAbility(Spec.Handle);
				}
			}
		}
	}
	// Super::AbilityLocalInputPressed(InputID);
}

void UGDAbilitySystemComponent::AbilityLocalInputReleased(int32 InputID)
{
	Super::AbilityLocalInputReleased(InputID);
}

void UGDAbilitySystemComponent::NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability)
{
	Super::NotifyAbilityActivated(Handle, Ability);
	CurrentActiveAbility = Cast<UGDGameplayAbility>(Ability);
	ActiveAbilitySpecHandle = Handle;
}

void UGDAbilitySystemComponent::NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled)
{
	Super::NotifyAbilityEnded(Handle, Ability, bWasCancelled);
	CurrentActiveAbility = nullptr;
	ActiveAbilitySpecHandle = FGameplayAbilitySpecHandle();
}

void UGDAbilitySystemComponent::DebugReply_Implementation(const FString& Command, const TArray<FString>& Strings)
{
	FString DebugTitle;
	const ENetRole AvatarRole = GetAvatarActor()->GetLocalRole();
	DebugTitle += FString::Printf(TEXT("for avatar %s "), *GetAvatarActor()->GetName());
	if (AvatarRole == ROLE_AutonomousProxy)
	{
		DebugTitle += TEXT("(local player) ");
	}
	else if (AvatarRole == ROLE_SimulatedProxy)
	{
		DebugTitle += TEXT("(simulated) ");
	}
	else if (AvatarRole == ROLE_Authority)
	{
		DebugTitle += TEXT("(authority) ");
	}

	ABILITY_LOG(Display, TEXT("============ %s, %s"), *DebugTitle, *Command);
	ABILITY_LOG(Display, TEXT(" "));
	ABILITY_LOG(Display, TEXT("Server State: "));
	for (const FString& Str : Strings)
	{
		ABILITY_LOG(Display, TEXT("%s"), *Str);
	}
}

void UGDAbilitySystemComponent::DebugRequest_Implementation(const FString& Command)
{
	// Find one
	for (TObjectIterator<UAbilitySystemComponent> It; It; ++It)
	{
		if (UAbilitySystemComponent* ASC = *It)
		{
			FAbilitySystemComponentDebugInfo DebugInfo;
			DebugInfo.bShowAbilities = true;
			DebugInfo.bShowAttributes = true;
			DebugInfo.bShowGameplayEffects = true;
			DebugInfo.Accumulate = true;
			DebugInfo.bPrintToLog = true;

			Debug_Internal(DebugInfo);

			DebugReply(Command, MoveTemp(DebugInfo.Strings));

			AActor* Owner = ASC->GetOwnerActor();
			if (Owner)
			{
				FNetworkGUID GUID = Owner->GetWorld()->GetNetDriver()->GuidCache->GetNetGUID(Owner);
				ABILITY_LOG(Display, TEXT("GUID=%d, %d"), GUID.Value, Owner->GetUniqueID());
			}
		}
	}
	// DebugReply(Command);
}
