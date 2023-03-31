



GA:
* Policy
* Tags
* Tasks

Structure:
* GA
* GASpec

Operations:
* Granting
* Activating
  * TryActive
  * Triggered by Tags
  * Triggered by Event
* Canceling
* Getting

Process:
* Simple
* Complicated

Other:
* Leveling Up
* Cost & Cooldown
* Passing Data
* Batching

Extending:


# GameplayAbility

Extending:

* 控制是否同步给Owner？
* 
```c++
/** Takes in the ability spec and checks if we should allow replication on the ability spec, this will NOT stop replication of the ability UObject just the spec inside the UAbilitySystemComponenet ActivatableAbilities for this ability */
virtual bool ShouldReplicateAbilitySpec(const FGameplayAbilitySpec& AbilitySpec) const
```


# GameplayEvent

## Structure

**FGameplayEventData:**
* EventTag
* Instigator
* InstigatorTags
* Target
* TargetTags
* EventMagnitude
* OptionalObject/OptionalObject2
* ContextHandle - GEContext
* TargetData - TargetHandle


**ASC :** 
```c++
    /** Abilities that are triggered from a gameplay event */
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle > > GameplayEventTriggeredAbilities;
	
	/** Abilities that are triggered from a tag being added to the owner */
	TMap<FGameplayTag, TArray<FGameplayAbilitySpecHandle > > OwnedTagTriggeredAbilities;
```

**GA:**


```c++
	/** Triggers to determine if this ability should execute in response to an event */
	UPROPERTY(EditDefaultsOnly, Category = Triggers)
	TArray<FAbilityTriggerData> AbilityTriggers;
```

**FAbilityTriggerData:**

* TriggerTag
* TriggerSource
  * GameplayEvent - Triggered from a gameplay event, will come with payload (由ASC.GameplayEventTriggeredAbilities管理)
  * OwnedTagAdded - Triggered if the ability's owner gets a tag added, triggered once whenever it's added (ASC.OwnedTagTriggeredAbilities)
  * OwnedTagPresent - Triggered if the ability's owner gets tag added, removed when the tag is removed (ASC.OwnedTagTriggeredAbilities)


## Send

```c++
/**
	 * This function can be used to trigger an ability on the actor in question with useful payload data.
	 * NOTE: The Actor passed in must implement IAbilitySystemInterface! or else this function will silently fail to
	 * send the event.  The actor needs the interface to find the UAbilitySystemComponent, and if the component isn't
	 * found, the event will not be sent.
	 */
	UFUNCTION(BlueprintCallable, Category = Ability, Meta = (Tooltip = "This function can be used to trigger an ability on the actor in question with useful payload data."))
	static void SendGameplayEventToActor(AActor* Actor, FGameplayTag EventTag, FGameplayEventData Payload) {
    {
       
        UAbilitySystemComponent* AbilitySystemComponent = AbilitySystemInterface->GetAbilitySystemComponent();
        if (AbilitySystemComponent != nullptr && !AbilitySystemComponent->IsPendingKill())
        {
            FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
            AbilitySystemComponent->HandleGameplayEvent(EventTag, &Payload);
        }    
	}
```

## Activate

BP: (EventData)

```c++
UFUNCTION(BlueprintImplementableEvent, Category = Ability, DisplayName = "ActivateAbilityFromEvent", meta=(ScriptName = "ActivateAbilityFromEvent"))
	void K2_ActivateAbilityFromEvent(const FGameplayEventData& EventData);
```

C++: (TriggerEventData)

```c++
	/** Actually activate ability, do not call this directly */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData);
```




