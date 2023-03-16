
### Structure

> ASC 

```c++
/** Current prediction key, set with FScopedPredictionWindow */
FPredictionKey	ScopedPredictionKey;

// Failure tags used by InternalTryActivateAbility (E.g., this stores the  FailureTags of the last call to InternalTryActivateAbility
FGameplayTagContainer InternalTryActivateAbilityFailureTags;

/** PredictionKeys, see more info in GameplayPrediction.h. This has to come *last* in all replicated properties on the AbilitySystemComponent to ensure OnRep/callback order. */
UPROPERTY(Replicated)
FReplicatedPredictionKeyMap ReplicatedPredictionKeyMap;

```

> GASpec


```c++
/**
 *	FGameplayAbilityActivationInfo
 *
 *	Data tied to a specific activation of an ability.
 *		-Tell us whether we are the authority, if we are predicting, confirmed, etc.
 *		-Holds current and previous PredictionKey
 *		-Generally not meant to be subclassed in projects.
 *		-Passed around by value since the struct is small.
 */
 
 /** Activation state of this ability. This is not replicated since it needs to be overwritten locally on clients during prediction. */
UPROPERTY(NotReplicated)
FGameplayAbilityActivationInfo	ActivationInfo;
```


FGameplayAbilityActivationInfo

 ActivationMode ：
 - Predicting
 - Confirmed
 - Rejected


### Process

#### Client-Autonomous : TryActivateAbility

```c++
bool UAbilitySystemComponent::TryActivateAbility(AbilitySpecHandle) {

    return InternalTryActivateAbility(AbilitySpecHandle) {

        InternalTryActivateAbilityFailureTags.Reset();

        // If it's instance once the instanced ability will be set, otherwise it will be null
	    UGameplayAbility* InstancedAbility = Spec->GetPrimaryInstance();

        // If we have an instanced ability, call CanActivateAbility on it.
		// Otherwise we always do a non instanced CanActivateAbility check using the CDO of the Ability.
        UGameplayAbility* const CanActivateAbilitySource = InstancedAbility ? InstancedAbility : Ability;
        if (!CanActivateAbilitySource->CanActivateAbility(Handle, ...))
		{
			NotifyAbilityFailed(Handle, CanActivateAbilitySource, InternalTryActivateAbilityFailureTags);
			return false;
		}

        // Setup a fresh ActivationInfo for this AbilitySpec.
        Spec->ActivationInfo = FGameplayAbilityActivationInfo(ActorInfo->OwnerActor.Get());
        FGameplayAbilityActivationInfo &ActivationInfo = Spec->ActivationInfo;

        // LocalPredicted
        if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalPredicted) {

            // Flush server moves that occurred before this ability activation so that the server receives the RPCs in the correct order
		    // Necessary to prevent abilities that trigger animation root motion or impact movement from causing network corrections
            AvatarCharMoveComp->FlushServerMoves();

            // This execution is now officially EGameplayAbilityActivationMode:Predicting and has a PredictionKey
		    FScopedPredictionWindow ScopedPredictionWindow(this, true);

		    ActivationInfo.SetPredicting(ScopedPredictionKey);
            CallServerTryActivateAbility(Handle, Spec->InputPressed, ScopedPredictionKey) {
                // RPC: C->S
                ServerTryActivateAbility(AbilityHandle, InputPressed, PredictionKey);
            }

            // When this prediction key is caught up, we better know if the ability was confirmed or rejected
		    ScopedPredictionKey.NewCaughtUpDelegate().BindUObject(this, 
                &UAbilitySystemComponent::OnClientActivateAbilityCaughtUp, Handle, ScopedPredictionKey.Current);

            // Executes PreActivate and ActivateAbility 
            Ability->CallActivateAbility(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData) {
                PreActivate(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);
	            ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
            }
        }
    }   
}
```

#### Server-Authority : ServerTryActivateAbility

> ServerTryActivateAbility -> ServerTryActivateAbility_Implementation -> 
InternalServerTryActivateAbility


只有服务器模式下才执行的代码：

```c++
// Handle - 技能ID
// PredictionKey - 客户端生成的PredicateKey
// TriggerEventData - 客户端传过来的事件数据
void InternalServerTryActivateAbility(Handle, InputPressed, PredictionKey, TriggerEventData){
#if WITH_SERVER_CODE

    // Consume any pending target info, to clear out cancels from old executions
	ConsumeAllReplicatedData(Handle, PredictionKey);

	FScopedPredictionWindow ScopedPredictionWindow(this, PredictionKey);

    // Attempt to activate the ability (server side) and tell the client if it succeeded or failed.
	if (InternalTryActivateAbility(Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
	{
		// TryActivateAbility handles notifying the client of success
	}
	else
	{
		// RPC: Rejecting ClientActivation (S->C)
		ClientActivateAbilityFailed(Handle, PredictionKey.Current);
    }

    // FScopedPredictionWindow::~FScopedPredictionWindow()
    //  - 设置ASC.ReplicatedPredictionKeyMap
    //  - 稍后执行属性同步

#endif
}

```

权威模式执行的流程：

```c++
bool InternalTryActivateAbility(AbilitySpecHandle) {

    InternalTryActivateAbilityFailureTags.Reset();

    // If it's instance once the instanced ability will be set, otherwise it will be null
    UGameplayAbility* InstancedAbility = Spec->GetPrimaryInstance();

    // If we have an instanced ability, call CanActivateAbility on it.
    // Otherwise we always do a non instanced CanActivateAbility check using the CDO of the Ability.
    UGameplayAbility* const CanActivateAbilitySource = InstancedAbility ? InstancedAbility : Ability;
    if (!CanActivateAbilitySource->CanActivateAbility(Handle, ...))
    {
        NotifyAbilityFailed(Handle, CanActivateAbilitySource, InternalTryActivateAbilityFailureTags);
        return false;
    }

    // Setup a fresh ActivationInfo for this AbilitySpec.
    Spec->ActivationInfo = FGameplayAbilityActivationInfo(ActorInfo->OwnerActor.Get());
    FGameplayAbilityActivationInfo &ActivationInfo = Spec->ActivationInfo;

    // LocalPredicted
    // If we are the server or this is local only
	if (Ability->GetNetExecutionPolicy() == EGameplayAbilityNetExecutionPolicy::LocalOnly || (NetMode == ROLE_Authority))
	{
        // we may have changed the prediction key so we need to update the scoped key to match
		FScopedPredictionWindow ScopedPredictionWindow(this, ActivationInfo.GetActivationPredictionKey()); 

        // Tell the client that you activated it (if we're not local and not server only)
        // RPC: S->C
        ClientActivateAbilitySucceed(Handle, ActivationInfo.GetActivationPredictionKey());

        // Call ActivateAbility (note this could end the ability too!)
        Ability->CallActivateAbility(Handle, ActorInfo, ActivationInfo, OnGameplayAbilityEndedDelegate, TriggerEventData);
    }
 }

```

FScopedPredictionWindow结束时（Authority模式下），会把当前的PredictionKey向客户端做属性同步，让客户端追上（CatchUp）。


```c++
 FScopedPredictionWindow::~FScopedPredictionWindow()
{
	if (UAbilitySystemComponent* OwnerPtr = Owner.Get())
	{
        // Server调用时SetReplicatedPredictionKey为true
		if (SetReplicatedPredictionKey)
		{
			// It is important to not set the ReplicatedPredictionKey unless it is valid (>0).
			// If we werent given a new prediction key for this scope from the client, then setting the
			// replicated prediction key back to 0 could cause OnReps to be missed on the client during high PL.
			// (for example, predict w/ key 100 -> prediction key replication dropped -> predict w/ invalid key -> next rep of prediction key is 0).
			if (OwnerPtr->ScopedPredictionKey.IsValidKey())
			{
				OwnerPtr->ReplicatedPredictionKeyMap.ReplicatePredictionKey(OwnerPtr->ScopedPredictionKey);
				OwnerPtr->bIsNetDirty = true;
			}
		}
		if (ClearScopedPredictionKey)
		{
			OwnerPtr->ScopedPredictionKey = RestoreKey;
		}
	}
}
```



#### Client-Autonomous ： Succeed/Failed/Catchup


> ClientActivateAbilitySucceed->ClientActivateAbilitySucceed_Implementation->ClientActivateAbilitySucceedWithEventData_Implementation

```c++
void ClientActivateAbilitySucceedWithEventData_Implementation(Handle, PredictionKey, TriggerEventData) {

    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);
    Spec->ActivationInfo.SetActivationConfirmed();
}

```


> ClientActivateAbilityFailed->ClientActivateAbilityFailed_Implementation

```c++
void ClientActivateAbilityFailed_Implementation(Handle, PredictionKey) {

    FPredictionKeyDelegates::BroadcastRejectedDelegate(PredictionKey);

    // Find the actual UGameplayAbility		
	FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(Handle);

    // The ability should be either confirmed or rejected by the time we get here
	if (Spec->ActivationInfo.GetActivationPredictionKey().Current == PredictionKey)
	{
		Spec->ActivationInfo.SetActivationRejected();
	}

    Ability->K2_EndAbility();
}
```

> Catchup

```c++
FReplicatedPredictionKeyItem::OnRep() {
    // Every predictive action we've done up to and including the current value of ReplicatedPredictionKey needs to be wiped
    // UAbilitySystemComponent::OnClientActivateAbilityCaughtUp
	FPredictionKeyDelegates::CatchUpTo(PredictionKey.Current); 

    // Sanity checking
	int32 Index = PredictionKey.Current % FReplicatedPredictionKeyMap::KeyRingBufferSize;
	for (auto MapIt = FPredictionKeyDelegates::Get().DelegateMap.CreateIterator(); MapIt; ++MapIt)
	{
		// If older key
		if (MapIt.Key() <= PredictionKey.Current)
		{	
			// Older key that would have gone in this slot
			if (MapIt.Key() % FReplicatedPredictionKeyMap::KeyRingBufferSize == Index)
			{
				// Message the log, this can happen during normal gameplay due to replication order, but can also indicate an ability-specific issue
				ABILITY_LOG(Log, TEXT("Passed PredictionKey %d in Delegate map while OnRep'ing %s"), MapIt.Key(), *PredictionKey.ToString());

				// Execute CaughtUp delegates
				for (auto& Delegate : MapIt.Value().CaughtUpDelegates)
				{
					Delegate.ExecuteIfBound();
				}

				// Cleanup
				MapIt.RemoveCurrent();
			}
		}
	}
}

```


