
# AbilitySystem Replication

## ASC Replication

同步机制（假设所有的修改操作都发生在服务器）：

* AS(AttributeSet) - 同步给所有（Authority/AutonomousProxy/SimulatedProxy）
* GT(GameplayTag) - 同步给所有（Authority/AutonomousProxy/SimulatedProxy）
* GC(GameplayCue) - 同步给所有（Authority/AutonomousProxy/SimulatedProxy）
* GE(GameplayEffect) - 根据ReplicationMode有所差异
	* Full - GE同步给所有客户端
	* Mixed - GE只同步给AutonomousProxy所在的客户端
	* Minimal - GE不同步给任何客户端
* GA(GameplayAbility) - 只同步给AutonomousProxy所在的客户端


仍有两种情况需要考虑：

* 预测性质的执行（比如：AutonomousProxy预测执行GE）
* 本地Only的执行（比如：只添加本地的GA、GT、GC、GE）

## Replication Structure

```c++
    /** List of attribute sets */
	UPROPERTY(Replicated)
	TArray<UAttributeSet*>	SpawnedAttributes;
		
	/** Acceleration map for all gameplay tags (OwnedGameplayTags from GEs and explicit GameplayCueTags) */
	FGameplayTagCountContainer GameplayTagCountContainer;

	UPROPERTY(Replicated)
	FMinimalReplicationTagCountMap MinimalReplicationTags;

	/**
	 *	The abilities we can activate. 
	 *		-This will include CDOs for non instanced abilities and per-execution instanced abilities. 
	 *		-Actor-instanced abilities will be the actual instance (not CDO)
	 */
	UPROPERTY(ReplicatedUsing=OnRep_ActivateAbilities, BlueprintReadOnly, Category = "Abilities")
	FGameplayAbilitySpecContainer ActivatableAbilities;
	
	/** Contains all of the gameplay effects that are currently active on this component */
	UPROPERTY(Replicated)
	FActiveGameplayEffectsContainer ActiveGameplayEffects;

	/** List of all active gameplay cues, including ones applied manually */
	UPROPERTY(Replicated)
	FActiveGameplayCueContainer ActiveGameplayCues;

	/** Replicated gameplaycues when in minimal replication mode. These are cues that would come normally come from ActiveGameplayEffects */
	UPROPERTY(Replicated)
	FActiveGameplayCueContainer MinimalReplicationGameplayCues;
```

## Replication Mode

ReplicationMode: 
* Full - GE同步给所有客户端
* Mixed - GE只同步给AutonomousProxy所在的客户端
* Minimal - GE不同步给任何客户端

David Ratti的建议:

> Both games(Paragon & Fortnite) essentially use Mixed mode for their player controlled characters and Minimal for AI controlled (AI minions, jungle creeps, AI Husks, etc). This is what I would recommend most people using the system in a multiplayer game.

ASC实现：

```c++
    /** When true, we will not replicate active gameplay effects for this ability system component, so attributes and tags */
    virtual void SetReplicationMode(EGameplayEffectReplicationMode NewReplicationMode);
    
    /** How gameplay effects are replicated */
    EGameplayEffectReplicationMode ReplicationMode;
```

处理代码：

```c++
FActiveGameplayEffectsContainer::NetDeltaSerialize() {}
```

# GA Replication

ASC:

```c++
	// ActivatableAbilities同步给AutonomyProxy
	UPROPERTY(ReplicatedUsing=OnRep_ActivateAbilities, BlueprintReadOnly, Category = "Abilities")
	FGameplayAbilitySpecContainer ActivatableAbilities;
```

GASpec:

```c++
	/** Handle for outside sources to refer to this spec by */
	UPROPERTY()
	FGameplayAbilitySpecHandle Handle;
	
	/** Ability of the spec (Always the CDO. This should be const but too many things modify it currently) */
	UPROPERTY()
	UGameplayAbility* Ability;
	
	/** Level of Ability */
	UPROPERTY()
	int32	Level;

	/** InputID, if bound */
	UPROPERTY()
	int32	InputID;

	/** Object this ability was created from, can be an actor or static object. Useful to bind an ability to a gameplay object */
	UPROPERTY()
	UObject* SourceObject;

	/** Optional ability tags that are replicated.  These tags are also captured as source tags by applied gameplay effects. */
	UPROPERTY()
	FGameplayTagContainer DynamicAbilityTags;

	/** Non replicating instances of this ability. */
	UPROPERTY(NotReplicated)
	TArray<UGameplayAbility*> NonReplicatedInstances;

	/** Replicated instances of this ability.. */
	UPROPERTY()
	TArray<UGameplayAbility*> ReplicatedInstances;
```

仅同步给自己：

```c++

void UAbilitySystemComponent::GetLifetimeReplicatedProps() {
	// 仅同步给自己
	Params.Condition = COND_ReplayOrOwner;
	DOREPLIFETIME_WITH_PARAMS_FAST(UAbilitySystemComponent, ActivatableAbilities, Params);
}

```

# GE Replication


## 结构

ASC：

```c++
	/** Contains all of the gameplay effects that are currently active on this component */
	UPROPERTY(Replicated)
	FActiveGameplayEffectsContainer ActiveGameplayEffects;
```


ActiveGEContainer：

```c++
```


ActiveGE：


## 同步逻辑


### 同步入口

```c++
bool FActiveGameplayEffectsContainer::NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
{
	// these tests are only necessary when sending
	if (DeltaParms.Writer != nullptr && Owner != nullptr)
	{
		// 此处：关于ReplicationMode的处理
		EGameplayEffectReplicationMode ReplicationMode = Owner->ReplicationMode;
		if (ReplicationMode == EGameplayEffectReplicationMode::Minimal)
		{
			return false;
		}
		else if (ReplicationMode == EGameplayEffectReplicationMode::Mixed)
		{
			// ..
		}
	}

	// 此处：触发FActiveGameplayEffect::PostReplicatedAdd之类的操作
	bool RetVal = FastArrayDeltaSerialize<FActiveGameplayEffect>(GameplayEffects_Internal, DeltaParms, *this);
	
	// 此处：AutonomousProxy对于GC的同步
	if (DeltaParms.Writer == nullptr && Owner != nullptr)
	{
		if (!DeltaParms.bOutHasMoreUnmapped) 
		{
			if (Owner->IsReadyForGameplayCues())
			{
				Owner->HandleDeferredGameplayCues(this);
			}
		}
	}

	return RetVal;
}
```

### 新加GE的同步

```c++
void FActiveGameplayEffect::PostReplicatedAdd(const struct FActiveGameplayEffectsContainer &InArray)
{
	// Handles are not replicated, so create a new one.
	Handle = FActiveGameplayEffectHandle::GenerateNewHandle(InArray.Owner);

	// Do stuff for adding GEs (add mods, tags, *invoke callbacks*
	const_cast<FActiveGameplayEffectsContainer&>(InArray).InternalOnActiveGameplayEffectAdded(*this) {
		SCOPE_CYCLE_COUNTER(STAT_OnActiveGameplayEffectAdded);

		const UGameplayEffect* EffectDef = Effect.Spec.Def;

		// 各种Tag的依赖关系
		// Add our ongoing tag requirements to the dependency map. We will actually check for these tags below.
		for (const FGameplayTag& Tag : EffectDef->OngoingTagRequirements.IgnoreTags)
		{
			ActiveEffectTagDependencies.FindOrAdd(Tag).Add(Effect.Handle);
		}

		for (const FGameplayTag& Tag : EffectDef->OngoingTagRequirements.RequireTags)
		{
			ActiveEffectTagDependencies.FindOrAdd(Tag).Add(Effect.Handle);
		}

		// Add our removal tag requirements to the dependency map. We will actually check for these tags below.
		for (const FGameplayTag& Tag : EffectDef->RemovalTagRequirements.IgnoreTags)
		{
			ActiveEffectTagDependencies.FindOrAdd(Tag).Add(Effect.Handle);
		}

		for (const FGameplayTag& Tag : EffectDef->RemovalTagRequirements.RequireTags)
		{
			ActiveEffectTagDependencies.FindOrAdd(Tag).Add(Effect.Handle);
		}

		// Add any external dependencies that might dirty the effect, if necessary
		AddCustomMagnitudeExternalDependencies(Effect);

		// Check if we should actually be turned on or not (this will turn us on for the first time)
		static FGameplayTagContainer OwnerTags;
		OwnerTags.Reset();

		Owner->GetOwnedGameplayTags(OwnerTags);
		
		Effect.bIsInhibited = true;

		// 应用成功与否的判定和入口
		Effect.CheckOngoingTagRequirements(OwnerTags, *this)
		{
			bool bShouldBeInhibited = !Spec.Def->OngoingTagRequirements.RequirementsMet(OwnerTags);

			if (bIsInhibited != bShouldBeInhibited)
			{
				// All OnDirty callbacks must be inhibited until we update this entire GameplayEffect.
				FScopedAggregatorOnDirtyBatch	AggregatorOnDirtyBatcher;

				// Important to set this prior to adding or removing, so that any delegates that are triggered can query accurately against this GE
				bIsInhibited = bShouldBeInhibited;

				if (bShouldBeInhibited)
				{
					// Remove our ActiveGameplayEffects modifiers with our Attribute Aggregators
					OwningContainer.RemoveActiveGameplayEffectGrantedTagsAndModifiers(*this, bInvokeGameplayCueEvents);
				}
				else
				{
					// 添加GE（更新Tag/Cue）
					OwningContainer.AddActiveGameplayEffectGrantedTagsAndModifiers(*this, bInvokeGameplayCueEvents);
				}
			}
		}
	}
}
```

### 删除GE的同步

```c++
void FActiveGameplayEffect::PreReplicatedRemove(const struct FActiveGameplayEffectsContainer &InArray)
{
	const_cast<FActiveGameplayEffectsContainer&>(InArray).InternalOnActiveGameplayEffectRemoved(*this, !bIsInhibited, GameplayEffectRemovalInfo) {

	// Mark the effect as pending removal
	Effect.IsPendingRemove = true;

	if (Effect.Spec.Def)
	{
		// Remove our tag requirements from the dependency map
		RemoveActiveEffectTagDependency(Effect.Spec.Def->OngoingTagRequirements.IgnoreTags, Effect.Handle);
		RemoveActiveEffectTagDependency(Effect.Spec.Def->OngoingTagRequirements.RequireTags, Effect.Handle);
		RemoveActiveEffectTagDependency(Effect.Spec.Def->RemovalTagRequirements.IgnoreTags, Effect.Handle);
		RemoveActiveEffectTagDependency(Effect.Spec.Def->RemovalTagRequirements.RequireTags, Effect.Handle);

		// Only Need to update tags and modifiers if the gameplay effect is active.
		if (!Effect.bIsInhibited)
		{
			// 更新Tag和Modifers
			RemoveActiveGameplayEffectGrantedTagsAndModifiers(Effect, bInvokeGameplayCueEvents);
		}

		RemoveCustomMagnitudeExternalDependencies(Effect);
	}
	else
	{
		ABILITY_LOG(Warning, TEXT("InternalOnActiveGameplayEffectRemoved called with no GameplayEffect: %s"), *Effect.Handle.ToString());
	}
}
```

# AS Replication

## AS的注册

自动初始化AS：

```c++
void UAbilitySystemComponent::InitializeComponent()
{
    // ...
    // 组件初始化的时候，查找Owner的所有子对象，把Owner身上挂载的AS自动注册进，SpawnedAttributesRef
	TArray<UObject*> ChildObjects;
	GetObjectsWithOuter(Owner, ChildObjects, false, RF_NoFlags, EInternalObjectFlags::PendingKill);
	for (UObject* Obj : ChildObjects)
	{
		UAttributeSet* Set = Cast<UAttributeSet>(Obj);
		if (Set)  
		{
			SpawnedAttributesRef.AddUnique(Set);
			bIsNetDirty = true;
		}
	}
}
```

手动添加：

```c++
	/** [GAS-Attributes.md](GAS-Attributes.md)
	 * Manually add a new attribute set that is a subobject of this ability system component.
	 * All subobjects of this component are automatically added during initialization.
	 */
	template <class T>
	const T* AddAttributeSetSubobject(T* Subobject);
```

## AS支持同步

* `REPNOTIFY_Always` - 无论属性是否有变化，都需要同步
* `GAMEPLAYATTRIBUTE_REPNOTIFY` - 为了解决预测override问题， 直接把服务器同步过来的值，设置为Base，稍后再重新聚合

```c++
 void UMyHealthSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
 {
 	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
 
 	DOREPLIFETIME_CONDITION_NOTIFY(UMyHealthSet, Health, COND_None, REPNOTIFY_Always);
 }
 
 void UMyHealthSet::OnRep_Health()
 {
 	GAMEPLAYATTRIBUTE_REPNOTIFY(UMyHealthSet, Health);
 }
```

# GT Replication

两种情况：

* Full Mode - 通过同步GE，GE在Add/Remove时，更新GameplayTagCountContainer
* Minimal Mode - 由于不再同步GE了，相应的Tag必须写入MinimalReplicationTags，然后MinimalReplicationTags做属性同步

相关结构：

```c++
/** Acceleration map for all gameplay tags (OwnedGameplayTags from GEs and explicit GameplayCueTags) */
FGameplayTagCountContainer GameplayTagCountContainer;

UPROPERTY(Replicated)
FMinimalReplicationTagCountMap MinimalReplicationTags;

```

## 同步逻辑

添加GE时同步更新Tag：

```c++
FActiveGameplayEffectsContainer::AddActiveGameplayEffectGrantedTagsAndModifiers() 
{
	// ...
	// Update our owner with the tags this GameplayEffect grants them
	Owner->UpdateTagMap(Effect.Spec.Def->InheritableOwnedTagsContainer.CombinedTags, 1);
	Owner->UpdateTagMap(Effect.Spec.DynamicGrantedTags, 1);
	if (ShouldUseMinimalReplication())
	{
		Owner->AddMinimalReplicationGameplayTags(Effect.Spec.Def->InheritableOwnedTagsContainer.CombinedTags);
		Owner->AddMinimalReplicationGameplayTags(Effect.Spec.DynamicGrantedTags);
	}
}
```

删除GE是同步更新Tag：

```c++
void FActiveGameplayEffectsContainer::RemoveActiveGameplayEffectGrantedTagsAndModifiers()
{
	// Update gameplaytag count and broadcast delegate if we are at 0
	Owner->UpdateTagMap(Effect.Spec.Def->InheritableOwnedTagsContainer.CombinedTags, -1);
	Owner->UpdateTagMap(Effect.Spec.DynamicGrantedTags, -1);

	if (ShouldUseMinimalReplication())
	{
		Owner->RemoveMinimalReplicationGameplayTags(Effect.Spec.Def->InheritableOwnedTagsContainer.CombinedTags);
		Owner->RemoveMinimalReplicationGameplayTags(Effect.Spec.DynamicGrantedTags);
	}
}
```

通过MinimalReplicationTags同步：

```c++
bool FMinimalReplicationTagCountMap::NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
{
	// .. 省略一大段...
	if (bUpdateOwnerTagMap)
	{
		UpdateOwnerTagMap();
	}
}
```


## Full Mode

* Authority的Tag变化（FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec）

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
FGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag &,int) GameplayEffectTypes.cpp:524
UAbilitySystemComponent::UpdateTagMap_Internal(const FGameplayTagContainer &,int) AbilitySystemComponent.cpp:494
FActiveGameplayEffectsContainer::AddActiveGameplayEffectGrantedTagsAndModifiers(FActiveGameplayEffect &,bool) GameplayEffect.cpp:3160
FActiveGameplayEffect::CheckOngoingTagRequirements(const FGameplayTagContainer &,FActiveGameplayEffectsContainer &,bool) GameplayEffect.cpp:1719
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectAdded(FActiveGameplayEffect &) GameplayEffect.cpp:3097


FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec(const FGameplayEffectSpec &,FPredictionKey &,bool &) GameplayEffect.cpp:3040
UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &,FPredictionKey) AbilitySystemComponent.cpp:750
UAbilitySystemComponent::BP_ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpecHandle &) AbilitySystemComponent.cpp:900
```


* Simulated/Autonomous的Tag变化（FActiveGameplayEffect::PostReplicatedAdd）

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
FGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag &,int) GameplayEffectTypes.cpp:524
UAbilitySystemComponent::UpdateTagMap_Internal(const FGameplayTagContainer &,int) AbilitySystemComponent.cpp:494
FActiveGameplayEffectsContainer::AddActiveGameplayEffectGrantedTagsAndModifiers(FActiveGameplayEffect &,bool) GameplayEffect.cpp:3160
FActiveGameplayEffect::CheckOngoingTagRequirements(const FGameplayTagContainer &,FActiveGameplayEffectsContainer &,bool) GameplayEffect.cpp:1719
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectAdded(FActiveGameplayEffect &) GameplayEffect.cpp:3097


FActiveGameplayEffect::PostReplicatedAdd(const FActiveGameplayEffectsContainer &) GameplayEffect.cpp:1836
```

## Mixed Mode

* Authority的Tag变化（FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec）
* AutonomousProxy的Tag变化（FActiveGameplayEffect::PostReplicatedAdd）
* SimulatedProxy的Tag变化（FMinimalReplicationTagCountMap::NetSerialize）

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
FGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag &,int) GameplayEffectTypes.cpp:524

FMinimalReplicationTagCountMap::UpdateOwnerTagMap() GameplayEffectTypes.cpp:1203
FMinimalReplicationTagCountMap::NetSerialize(FArchive &,UPackageMap *,bool &) GameplayEffectTypes.cpp:1164
```


## Minimal Mode

* Authority的Tag变化（FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec）
* AutonomousProxy的Tag变化（FActiveGameplayEffect::PostReplicatedAdd）
* SimulatedProxy的Tag变化（FMinimalReplicationTagCountMap::NetSerialize）


# GC Replication

同样分两种情况：
* FullMode（同步了GE）
* MinmalMode（不同步GE）

与GT有区别的地方，在于GC的触发会有EventType，OnActive/WhileActive在不同模式下触发会有所差异。下面以最复杂的MixedMode为例，看下同步流程。


相关结构：


```c++
	/** List of all active gameplay cues, including ones applied manually */
	UPROPERTY(Replicated)
	FActiveGameplayCueContainer ActiveGameplayCues;

	/** Replicated gameplaycues when in minimal replication mode. These are cues that would come normally come from ActiveGameplayEffects */
	UPROPERTY(Replicated)
	FActiveGameplayCueContainer MinimalReplicationGameplayCues;

```

## 同步逻辑


添加GC：

```c++
FActiveGameplayEffectsContainer::AddActiveGameplayEffectGrantedTagsAndModifiers() 
{
	// Update GameplayCue tags and events
	if (!Owner->bSuppressGameplayCues)
	{
		for (const FGameplayEffectCue& Cue : Effect.Spec.Def->GameplayCues)
		{
			Owner->UpdateTagMap(Cue.GameplayCueTags, 1);

			if (bInvokeGameplayCueEvents)
			{
				Owner->InvokeGameplayCueEvent(Effect.Spec, EGameplayCueEvent::OnActive);
				Owner->InvokeGameplayCueEvent(Effect.Spec, EGameplayCueEvent::WhileActive);
			}

			if (ShouldUseMinimalReplication())
			{
				for (const FGameplayTag& CueTag : Cue.GameplayCueTags)
				{
					// We are now replicating the EffectContext in minimally replicated cues. It may be worth allowing this be determined on a per cue basis one day.
					// (not sending the EffectContext can make things wrong. E.g, the EffectCauser becomes the target of the GE rather than the source)
					Owner->AddGameplayCue_MinimalReplication(CueTag, Effect.Spec.GetEffectContext());
				}
			}
		}
	}
}
```


```c++
UAbilitySystemComponent::AddGameplayCue_Internal() 
{
	// Finally, call the RPC to play the OnActive event
	if (IAbilitySystemReplicationProxyInterface* ReplicationInterface = GetReplicationInterface())
	{
		// NetMulticast广播
		ReplicationInterface->Call_InvokeGameplayCueAdded_WithParams(GameplayCueTag, PredictionKeyForRPC, GameplayCueParameters);
	}
}
```


删除GC：

```c++
void FActiveGameplayEffectsContainer::RemoveActiveGameplayEffectGrantedTagsAndModifiers()
{
	// Update GameplayCue tags and events
	if (!Owner->bSuppressGameplayCues)
	{
		for (const FGameplayEffectCue& Cue : Effect.Spec.Def->GameplayCues)
		{
			Owner->UpdateTagMap(Cue.GameplayCueTags, -1);

			if (bInvokeGameplayCueEvents)
			{
				Owner->InvokeGameplayCueEvent(Effect.Spec, EGameplayCueEvent::Removed);
			}

			if (ShouldUseMinimalReplication())
			{
				for (const FGameplayTag& CueTag : Cue.GameplayCueTags)
				{
					Owner->RemoveGameplayCue_MinimalReplication(CueTag);
				}
			}
		}
	}
}
```


## Mixed-AutonomousProxy

OnActive/WhileActive：

```c++
AGameplayCueNotify_Actor::HandleGameplayCue(AActor *,Type,const FGameplayCueParameters &) GameplayCueNotify_Actor.cpp:149
UGameplayCueSet::HandleGameplayCueNotify_Internal(AActor *,int,Type,FGameplayCueParameters &) GameplayCueSet.cpp:276
UGameplayCueSet::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &) GameplayCueSet.cpp:42
UGameplayCueManager::RouteGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:211
UGameplayCueManager::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:153
UGameplayCueManager::HandleGameplayCues(AActor *,const FGameplayTagContainer &,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:126
UAbilitySystemComponent::InvokeGameplayCueEvent(const FGameplayEffectSpecForRPC &,Type) AbilitySystemComponent.cpp:1141

UAbilitySystemComponent::HandleDeferredGameplayCues(const FActiveGameplayEffectsContainer *) AbilitySystemComponent.cpp:1720
FActiveGameplayEffectsContainer::NetDeltaSerialize(FNetDeltaSerializeInfo &) GameplayEffect.cpp:3858
```

Remove：

```c++
AGameplayCueNotify_Actor::HandleGameplayCue(AActor *,Type,const FGameplayCueParameters &) GameplayCueNotify_Actor.cpp:151
UGameplayCueSet::HandleGameplayCueNotify_Internal(AActor *,int,Type,FGameplayCueParameters &) GameplayCueSet.cpp:276
UGameplayCueSet::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &) GameplayCueSet.cpp:42
UGameplayCueManager::RouteGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:211
UGameplayCueManager::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:153
UGameplayCueManager::HandleGameplayCues(AActor *,const FGameplayTagContainer &,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:126
UAbilitySystemComponent::InvokeGameplayCueEvent(const FGameplayEffectSpecForRPC &,Type) AbilitySystemComponent.cpp:1141

FActiveGameplayEffectsContainer::RemoveActiveGameplayEffectGrantedTagsAndModifiers(const FActiveGameplayEffect &,bool) GameplayEffect.cpp:3509
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectRemoved(FActiveGameplayEffect &,bool,const FGameplayEffectRemovalInfo &) GameplayEffect.cpp:3418
FActiveGameplayEffect::PreReplicatedRemove(const FActiveGameplayEffectsContainer &) GameplayEffect.cpp:1768
```


## Mixed-SimulatedProxy

OnActive（UAbilitySystemComponent::NetMulticast_InvokeGameplayCueAdded_WithParams_Implementatio）：

```c++
AGameplayCueNotify_Actor::HandleGameplayCue(AActor *,Type,const FGameplayCueParameters &) GameplayCueNotify_Actor.cpp:149
UGameplayCueSet::HandleGameplayCueNotify_Internal(AActor *,int,Type,FGameplayCueParameters &) GameplayCueSet.cpp:276
UGameplayCueSet::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &) GameplayCueSet.cpp:42
UGameplayCueManager::RouteGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:211
UGameplayCueManager::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:153
UAbilitySystemComponent::InvokeGameplayCueEvent(FGameplayTag,Type,const FGameplayCueParameters &) AbilitySystemComponent.cpp:1161

UAbilitySystemComponent::NetMulticast_InvokeGameplayCueAdded_WithParams_Implementation(FGameplayTag,FPredictionKey,FGameplayCueParameters) AbilitySystemComponent.cpp:1368
```

WhileActive（UAbilitySystemComponent::HandleDeferredGameplayCues）:

```c++
AGameplayCueNotify_Actor::HandleGameplayCue(AActor *,Type,const FGameplayCueParameters &) GameplayCueNotify_Actor.cpp:149
UGameplayCueSet::HandleGameplayCueNotify_Internal(AActor *,int,Type,FGameplayCueParameters &) GameplayCueSet.cpp:276
UGameplayCueSet::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &) GameplayCueSet.cpp:42
UGameplayCueManager::RouteGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:211
UGameplayCueManager::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:153
UGameplayCueManager::HandleGameplayCues(AActor *,const FGameplayTagContainer &,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:126
UAbilitySystemComponent::InvokeGameplayCueEvent(const FGameplayEffectSpecForRPC &,Type) AbilitySystemComponent.cpp:1141

UAbilitySystemComponent::HandleDeferredGameplayCues(const FActiveGameplayEffectsContainer *) AbilitySystemComponent.cpp:1720
FActiveGameplayEffectsContainer::NetDeltaSerialize(FNetDeltaSerializeInfo &) GameplayEffect.cpp:3858
```





