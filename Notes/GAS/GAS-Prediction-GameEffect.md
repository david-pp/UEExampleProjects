

ASC:

```c++
	/** Contains all of the gameplay effects that are currently active on this component */
	UPROPERTY(Replicated)
	FActiveGameplayEffectsContainer ActiveGameplayEffects;

```

### Applying





UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf()

UGameplayAbility::ApplyGameplayEffectToOwner


AbilitySystemComponent->OnActiveGameplayEffectAddedDelegateToSelf.AddUObject(this, &APACharacterBase::OnActiveGameplayEffectAddedCallback);

virtual void OnActiveGameplayEffectAddedCallback(UAbilitySystemComponent* Target, const FGameplayEffectSpec& SpecApplied, FActiveGameplayEffectHandle ActiveHandle);

```c++

if (InPredictionKey.IsLocalClientKey() == false || IsNetAuthority())	// Clients predicting a GameplayEffect must not call MarkItemDirty
{
    MarkItemDirty(*AppliedActiveGE);

    ABILITY_LOG(Verbose, TEXT("Added GE: %s. ReplicationID: %d. Key: %d. PredictionLey: %d"), *AppliedActiveGE->Spec.Def->GetName(), AppliedActiveGE->ReplicationID, AppliedActiveGE->ReplicationKey, InPredictionKey.Current);
}
else
{
    // Clients predicting should call MarkArrayDirty to force the internal replication map to be rebuilt.
    MarkArrayDirty();

    // Once replicated state has caught up to this prediction key, we must remove this gameplay effect.
    InPredictionKey.NewRejectOrCaughtUpDelegate(FPredictionKeyEvent::CreateUObject(Owner, &UAbilitySystemComponent::RemoveActiveGameplayEffect_NoReturn, AppliedActiveGE->Handle, -1));
    
}


/** This only exists so it can be hooked up to a multicast delegate */
void RemoveActiveGameplayEffect_NoReturn(FActiveGameplayEffectHandle Handle, int32 StacksToRemove=-1)
{
    RemoveActiveGameplayEffect(Handle, StacksToRemove);
}
```



### Removing


FActiveGameplayEffectsContainer::RemoveActiveEffects()

AbilitySystemComponent->OnAnyGameplayEffectRemovedDelegate().AddUObject(this, &APACharacterBase::OnRemoveGameplayEffectCallback);

virtual void OnRemoveGameplayEffectCallback(const FActiveGameplayEffect& EffectRemoved);



### AS

```c++
UPROPERTY(Replicated)
TArray<UAttributeSet*>	SpawnedAttributes;
```


自动初始化AS：

```c++
void UAbilitySystemComponent::InitializeComponent()
{
	Super::InitializeComponent();

	// Look for DSO AttributeSets (note we are currently requiring all attribute sets to be subobjects of the same owner. This doesn't *have* to be the case forever.
	AActor *Owner = GetOwner();
	InitAbilityActorInfo(Owner, Owner);	// Default init to our outer owner

	// cleanup any bad data that may have gotten into SpawnedAttributes
	TArray<UAttributeSet*>& SpawnedAttributesRef = GetSpawnedAttributes_Mutable();
	for (int32 Idx = SpawnedAttributesRef.Num()-1; Idx >= 0; --Idx)
	{
		if (SpawnedAttributesRef[Idx] == nullptr)
		{
			SpawnedAttributesRef.RemoveAt(Idx);
		}
	}

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
	/** 
	 * Manually add a new attribute set that is a subobject of this ability system component.
	 * All subobjects of this component are automatically added during initialization.
	 */
	template <class T>
	const T* AddAttributeSetSubobject(T* Subobject)
	{
		GetSpawnedAttributes_Mutable().AddUnique(Subobject);
		return Subobject;
	}
```

```c++
// Draw the attribute aggregator map.
for (auto It = ActiveGameplayEffects.AttributeAggregatorMap.CreateConstIterator(); It; ++It)
{
	FGameplayAttribute Attribute = It.Key();
	const FAggregatorRef& AggregatorRef = It.Value();
	if (AggregatorRef.Get())
	{
		FAggregator& Aggregator = *AggregatorRef.Get();

		FAggregatorEvaluateParameters EmptyParams;

		TMap<EGameplayModEvaluationChannel, const TArray<FAggregatorMod>*> ModMap;
		Aggregator.EvaluateQualificationForAllMods(EmptyParams);
		Aggregator.GetAllAggregatorMods(ModMap);

		if (ModMap.Num() == 0)
		{
			continue;
		}

		float FinalValue = GetNumericAttribute(Attribute);
		float BaseValue = Aggregator.GetBaseValue();

		FString AttributeString = FString::Printf(TEXT("%s %.2f "), *Attribute.GetName(), GetNumericAttribute(Attribute));
		if (FMath::Abs<float>(BaseValue - FinalValue) > SMALL_NUMBER)
		{
			AttributeString += FString::Printf(TEXT(" (Base: %.2f)"), BaseValue);
		}

		if (Info.Canvas)
		{
			Info.Canvas->SetDrawColor(FColor::White);
		}

		DebugLine(Info, AttributeString, 4.f, 0.f);

		DrawAttributes.Add(Attribute);

		for (const auto& CurMapElement : ModMap)
		{
			const EGameplayModEvaluationChannel Channel = CurMapElement.Key;
			const TArray<FAggregatorMod>* ModArrays = CurMapElement.Value;

			const FString ChannelNameString = UAbilitySystemGlobals::Get().GetGameplayModEvaluationChannelAlias(Channel).ToString();
			for (int32 ModOpIdx = 0; ModOpIdx < EGameplayModOp::Max; ++ModOpIdx)
			{
				const TArray<FAggregatorMod>& CurModArray = ModArrays[ModOpIdx];
				for (const FAggregatorMod& Mod : CurModArray)
				{
					bool IsActivelyModifyingAttribute = Mod.Qualifies();
					if (Info.Canvas)
					{
						Info.Canvas->SetDrawColor(IsActivelyModifyingAttribute ? FColor::Yellow : FColor(128, 128, 128));
					}

					FActiveGameplayEffect* ActiveGE = ActiveGameplayEffects.GetActiveGameplayEffect(Mod.ActiveHandle);
					FString SrcName = ActiveGE ? ActiveGE->Spec.Def->GetName() : FString(TEXT(""));

					if (IsActivelyModifyingAttribute == false)
					{
						if (Mod.SourceTagReqs)
						{
							SrcName += FString::Printf(TEXT(" SourceTags: [%s] "), *Mod.SourceTagReqs->ToString());
						}
						if (Mod.TargetTagReqs)
						{
							SrcName += FString::Printf(TEXT("TargetTags: [%s]"), *Mod.TargetTagReqs->ToString());
						}
					}

					DebugLine(Info, FString::Printf(TEXT("   %s %s\t %.2f - %s"), *ChannelNameString, *EGameplayModOpToString(ModOpIdx), Mod.EvaluatedMagnitude, *SrcName), 7.f, 0.f);
					Info.NewColumnYPadding = FMath::Max<float>(Info.NewColumnYPadding, Info.YPos + Info.YL);
				}
			}
		}
	}
}
```


FActiveGameplayEffect::CheckOngoingTagRequirements


Aggreator刷新当前值：

FActiveGameplayEffectsContainer::OnAttributeAggregatorDirty(FAggregator *,FGameplayAttribute,bool) GameplayEffect.cpp:2147
UAbilitySystemComponent::OnAttributeAggregatorDirty(FAggregator *,FGameplayAttribute,bool) AbilitySystemComponent.cpp:1655
UE4Tuple_Private::TTupleBase<TIntegerSequence<unsigned int,0,1>,FGameplayAttribute,bool>::ApplyAfter<void (__cdecl UAbilitySystemComponent::*const &)(FAggregator *,FGameplayAttribute,bool),UAbilitySystemComponent * &,FAggregator * &>(void (UAbilitySystemComponent::*&)(FAggregator *, FGameplayAttribute, bool),UAbilitySystemComponent *&,FAggregator *&) Tuple.h:306
TBaseUObjectMethodDelegateInstance<0,UAbilitySystemComponent,void __cdecl(FAggregator *),FDefaultDelegateUserPolicy,FGameplayAttribute,bool>::ExecuteIfSafe(FAggregator *) DelegateInstancesImpl.h:609
TMulticastDelegate<void __cdecl(FAggregator *),FDefaultDelegateUserPolicy>::Broadcast(FAggregator *) DelegateSignatureImpl.inl:955
FAggregator::BroadcastOnDirty() GameplayEffectAggregator.cpp:604
FScopedAggregatorOnDirtyBatch::EndLock() GameplayEffectAggregator.cpp:670
FScopedAggregatorOnDirtyBatch::~FScopedAggregatorOnDirtyBatch() GameplayEffectAggregator.cpp:655
FActiveGameplayEffect::CheckOngoingTagRequirements(const FGameplayTagContainer &,FActiveGameplayEffectsContainer &,bool) GameplayEffect.cpp:1721
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectAdded(FActiveGameplayEffect &) GameplayEffect.cpp:3097
FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec(const FGameplayEffectSpec &,FPredictionKey &,bool &) GameplayEffect.cpp:3040
UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &,FPredictionKey) AbilitySystemComponent.cpp:749
UGameplayAbility::ApplyGameplayEffectSpecToOwner(FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo *,FGameplayAbilityActivationInfo,FGameplayEffectSpecHandle) GameplayAbility.cpp:1711
UGameplayAbility::ApplyGameplayEffectToOwner(FGameplayAbilitySpecHandle,const FGameplayAbilityActorInfo *,FGameplayAbilityActivationInfo,const UGameplayEffect *,float,int) GameplayAbility.cpp:1690


```c++
void FActiveGameplayEffectsContainer::OnAttributeAggregatorDirty(FAggregator* Aggregator, FGameplayAttribute Attribute, bool bFromRecursiveCall)
{
	check(AttributeAggregatorMap.FindChecked(Attribute).Get() == Aggregator);

	// Our Aggregator has changed, we need to reevaluate this aggregator and update the current value of the attribute.
	// Note that this is not an execution, so there are no 'source' and 'target' tags to fill out in the FAggregatorEvaluateParameters.
	// ActiveGameplayEffects that have required owned tags will be turned on/off via delegates, and will add/remove themselves from attribute
	// aggregators when that happens.
	
	FAggregatorEvaluateParameters EvaluationParameters;

	if (Owner->IsNetSimulating())
	{
		// ..
	}

	float NewValue = Aggregator->Evaluate(EvaluationParameters);

	if (EvaluationParameters.IncludePredictiveMods)
	{
		ABILITY_LOG(Log, TEXT("After Prediction, FinalValue: %.2f"), NewValue);
	}

	InternalUpdateNumericalAttribute(Attribute, NewValue, nullptr, bFromRecursiveCall);
}

```


Aggregator的创建 和 事件注册：

FActiveGameplayEffectsContainer::FindOrCreateAttributeAggregator(FGameplayAttribute) GameplayEffect.cpp:2123
FActiveGameplayEffectsContainer::CaptureAttributeForGameplayEffect(FGameplayEffectAttributeCaptureSpec &) GameplayEffect.cpp:2534
UAbilitySystemComponent::CaptureAttributeForGameplayEffect(FGameplayEffectAttributeCaptureSpec &) AbilitySystemComponent.cpp:233
FGameplayEffectAttributeCaptureSpecContainer::CaptureAttributes(UAbilitySystemComponent *,EGameplayEffectAttributeCaptureSource) GameplayEffect.cpp:1503
FGameplayEffectSpec::CaptureDataFromSource(bool) GameplayEffect.cpp:958
FGameplayEffectSpec::Initialize(const UGameplayEffect *,const FGameplayEffectContextHandle &,float) GameplayEffect.cpp:819
FGameplayEffectSpec::FGameplayEffectSpec(const UGameplayEffect *,const FGameplayEffectContextHandle &,float) GameplayEffect.cpp:675
UAbilitySystemComponent::MakeOutgoingSpec(TSubclassOf<UGameplayEffect>,float,FGameplayEffectContextHandle) AbilitySystemComponent.cpp:323

```c++
FAggregatorRef& FActiveGameplayEffectsContainer::FindOrCreateAttributeAggregator(FGameplayAttribute Attribute)
{
	FAggregatorRef* RefPtr = AttributeAggregatorMap.Find(Attribute);
	if (RefPtr)
	{
		return *RefPtr;
	}

	// Create a new aggregator for this attribute.
	float CurrentBaseValueOfProperty = Owner->GetNumericAttributeBase(Attribute);
	ABILITY_LOG(Log, TEXT("Creating new entry in AttributeAggregatorMap for %s. CurrentValue: %.2f"), *Attribute.GetName(), CurrentBaseValueOfProperty);

	FAggregator* NewAttributeAggregator = new FAggregator(CurrentBaseValueOfProperty);
	
	if (Attribute.IsSystemAttribute() == false)
	{
		NewAttributeAggregator->OnDirty.AddUObject(Owner, &UAbilitySystemComponent::OnAttributeAggregatorDirty, Attribute, false);
		NewAttributeAggregator->OnDirtyRecursive.AddUObject(Owner, &UAbilitySystemComponent::OnAttributeAggregatorDirty, Attribute, true);

		// Callback in case the set wants to do something
		const UAttributeSet* Set = Owner->GetAttributeSubobject(Attribute.GetAttributeSetClass());
		Set->OnAttributeAggregatorCreated(Attribute, NewAttributeAggregator);
	}

	return AttributeAggregatorMap.Add(Attribute, FAggregatorRef(NewAttributeAggregator));
}
```

```c++
void FGameplayEffectSpec::Initialize(const UGameplayEffect* InDef, const FGameplayEffectContextHandle& InEffectContext, float InLevel)
{
	Def = InDef;
	check(Def);	
	// SetContext requires the level to be set before it runs
	// however, there are code paths in SetLevel that can potentially (depends on game data setup) require the context to be set
	Level = InLevel;
	SetContext(InEffectContext);
	SetLevel(InLevel);

	// Init our ModifierSpecs
	Modifiers.SetNum(Def->Modifiers.Num());

	// 准备AttributeCapture信息
	// Prep the spec with all of the attribute captures it will need to perform
	SetupAttributeCaptureDefinitions();
	
	// Add the GameplayEffect asset tags to the source Spec tags
	CapturedSourceTags.GetSpecTags().AppendTags(InDef->InheritableGameplayEffectTags.CombinedTags);

	// 计算Capture数据
	// Prepare source tags before accessing them in ConditionalGameplayEffects
	CaptureDataFromSource();

	//..
}
```

GE同步的关键：

```c++
	void PreReplicatedRemove(const struct FActiveGameplayEffectsContainer &InArray);
	void PostReplicatedAdd(const struct FActiveGameplayEffectsContainer &InArray);
	void PostReplicatedChange(const struct FActiveGameplayEffectsContainer &InArray);
```

InternalOnActiveGameplayEffectAdded -> 在不同端都会执行


## GT


```c++
/** Acceleration map for all gameplay tags (OwnedGameplayTags from GEs and explicit GameplayCueTags) */
FGameplayTagCountContainer GameplayTagCountContainer;

UPROPERTY(Replicated)
FMinimalReplicationTagCountMap MinimalReplicationTags;


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

### Full Mode

Authority Stack：

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
TBaseUObjectMethodDelegateInstance<0,AGDPlayerState,void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::ExecuteIfSafe(FGameplayTag,int) DelegateInstancesImpl.h:609
TMulticastDelegate<void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::Broadcast(FGameplayTag,int) DelegateSignatureImpl.inl:955
TBaseFunctorDelegateInstance<void __cdecl(void),FDefaultDelegateUserPolicy,<lambda_b7068143640515ba05ce316b14425dc9> >::Execute() DelegateInstancesImpl.h:830
FGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag &,int) GameplayEffectTypes.cpp:524
UAbilitySystemComponent::UpdateTagMap_Internal(const FGameplayTagContainer &,int) AbilitySystemComponent.cpp:494
FActiveGameplayEffectsContainer::AddActiveGameplayEffectGrantedTagsAndModifiers(FActiveGameplayEffect &,bool) GameplayEffect.cpp:3160
FActiveGameplayEffect::CheckOngoingTagRequirements(const FGameplayTagContainer &,FActiveGameplayEffectsContainer &,bool) GameplayEffect.cpp:1719
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectAdded(FActiveGameplayEffect &) GameplayEffect.cpp:3097


FActiveGameplayEffectsContainer::ApplyGameplayEffectSpec(const FGameplayEffectSpec &,FPredictionKey &,bool &) GameplayEffect.cpp:3040
UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &,FPredictionKey) AbilitySystemComponent.cpp:750
UAbilitySystemComponent::BP_ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpecHandle &) AbilitySystemComponent.cpp:900
```

Simulated/Autonomous Stack：

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
TBaseUObjectMethodDelegateInstance<0,AGDPlayerState,void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::ExecuteIfSafe(FGameplayTag,int) DelegateInstancesImpl.h:609
TMulticastDelegate<void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::Broadcast(FGameplayTag,int) DelegateSignatureImpl.inl:955
TBaseFunctorDelegateInstance<void __cdecl(void),FDefaultDelegateUserPolicy,<lambda_b7068143640515ba05ce316b14425dc9> >::Execute() DelegateInstancesImpl.h:830
FGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag &,int) GameplayEffectTypes.cpp:524
UAbilitySystemComponent::UpdateTagMap_Internal(const FGameplayTagContainer &,int) AbilitySystemComponent.cpp:494
FActiveGameplayEffectsContainer::AddActiveGameplayEffectGrantedTagsAndModifiers(FActiveGameplayEffect &,bool) GameplayEffect.cpp:3160
FActiveGameplayEffect::CheckOngoingTagRequirements(const FGameplayTagContainer &,FActiveGameplayEffectsContainer &,bool) GameplayEffect.cpp:1719
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectAdded(FActiveGameplayEffect &) GameplayEffect.cpp:3097


FActiveGameplayEffect::PostReplicatedAdd(const FActiveGameplayEffectsContainer &) GameplayEffect.cpp:1836
```

### Mixed Mode

Authority Stack： 同上（Full Mode）
Autonomous Stack：同上 (Full Mode)
Simulated Stack：

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
TBaseUObjectMethodDelegateInstance<0,AGDPlayerState,void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::ExecuteIfSafe(FGameplayTag,int) DelegateInstancesImpl.h:609
TMulticastDelegate<void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::Broadcast(FGameplayTag,int) DelegateSignatureImpl.inl:955
TBaseFunctorDelegateInstance<void __cdecl(void),FDefaultDelegateUserPolicy,<lambda_b7068143640515ba05ce316b14425dc9> >::Execute() DelegateInstancesImpl.h:830
FGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag &,int) GameplayEffectTypes.cpp:524
FMinimalReplicationTagCountMap::UpdateOwnerTagMap() GameplayEffectTypes.cpp:1203
FMinimalReplicationTagCountMap::NetSerialize(FArchive &,UPackageMap *,bool &) GameplayEffectTypes.cpp:1164
```

### Minimal Mode

Authority Stack： 同上（Full Mode）
Autonomous Stack：同上
Simulated Stack：同上


### Removal

定时：
```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
TBaseUObjectMethodDelegateInstance<0,AGDPlayerState,void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::ExecuteIfSafe(FGameplayTag,int) DelegateInstancesImpl.h:609
TMulticastDelegate<void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::Broadcast(FGameplayTag,int) DelegateSignatureImpl.inl:955
TBaseFunctorDelegateInstance<void __cdecl(void),FDefaultDelegateUserPolicy,<lambda_b7068143640515ba05ce316b14425dc9> >::Execute() DelegateInstancesImpl.h:830
UAbilitySystemComponent::UpdateTagMap_Internal(const FGameplayTagContainer &,int) AbilitySystemComponent.cpp:524
FActiveGameplayEffectsContainer::RemoveActiveGameplayEffectGrantedTagsAndModifiers(const FActiveGameplayEffect &,bool) GameplayEffect.cpp:3453
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectRemoved(FActiveGameplayEffect &,bool,const FGameplayEffectRemovalInfo &) GameplayEffect.cpp:3418
FActiveGameplayEffectsContainer::InternalRemoveActiveGameplayEffect(int,int,bool) GameplayEffect.cpp:3338
FActiveGameplayEffectsContainer::CheckDuration(FActiveGameplayEffectHandle) GameplayEffect.cpp:3985
UAbilitySystemComponent::CheckDurationExpired(FActiveGameplayEffectHandle) AbilitySystemComponent.cpp:939
```

Autonoumous：

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
TBaseUObjectMethodDelegateInstance<0,AGDPlayerState,void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::ExecuteIfSafe(FGameplayTag,int) DelegateInstancesImpl.h:609
TMulticastDelegate<void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::Broadcast(FGameplayTag,int) DelegateSignatureImpl.inl:955
TBaseFunctorDelegateInstance<void __cdecl(void),FDefaultDelegateUserPolicy,<lambda_b7068143640515ba05ce316b14425dc9> >::Execute() DelegateInstancesImpl.h:830
UAbilitySystemComponent::UpdateTagMap_Internal(const FGameplayTagContainer &,int) AbilitySystemComponent.cpp:524
FActiveGameplayEffectsContainer::RemoveActiveGameplayEffectGrantedTagsAndModifiers(const FActiveGameplayEffect &,bool) GameplayEffect.cpp:3453
FActiveGameplayEffectsContainer::InternalOnActiveGameplayEffectRemoved(FActiveGameplayEffect &,bool,const FGameplayEffectRemovalInfo &) GameplayEffect.cpp:3418

FActiveGameplayEffect::PreReplicatedRemove(const FActiveGameplayEffectsContainer &) GameplayEffect.cpp:1768
```

Simulated：

```c++
AGDPlayerState::StunTagChanged(FGameplayTag,int) GDPlayerState.cpp:392
TBaseUObjectMethodDelegateInstance<0,AGDPlayerState,void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::ExecuteIfSafe(FGameplayTag,int) DelegateInstancesImpl.h:609
TMulticastDelegate<void __cdecl(FGameplayTag,int),FDefaultDelegateUserPolicy>::Broadcast(FGameplayTag,int) DelegateSignatureImpl.inl:955
TBaseFunctorDelegateInstance<void __cdecl(void),FDefaultDelegateUserPolicy,<lambda_b7068143640515ba05ce316b14425dc9> >::Execute() DelegateInstancesImpl.h:830
FGameplayTagCountContainer::UpdateTagMap_Internal(const FGameplayTag &,int) GameplayEffectTypes.cpp:524
FMinimalReplicationTagCountMap::UpdateOwnerTagMap() GameplayEffectTypes.cpp:1203
FMinimalReplicationTagCountMap::NetSerialize(FArchive &,UPackageMap *,bool &) GameplayEffectTypes.cpp:1164
```


## GC


```c++
	/** List of all active gameplay cues, including ones applied manually */
	UPROPERTY(Replicated)
	FActiveGameplayCueContainer ActiveGameplayCues;

	/** Replicated gameplaycues when in minimal replication mode. These are cues that would come normally come from ActiveGameplayEffects */
	UPROPERTY(Replicated)
	FActiveGameplayCueContainer MinimalReplicationGameplayCues;

```


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
		ReplicationInterface->Call_InvokeGameplayCueAdded_WithParams(GameplayCueTag, PredictionKeyForRPC, GameplayCueParameters);
	}
}
```


#### Mixed - Auto：

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


Mixed - Simulated：


Remove - 同上：

WhileActive - 同上：

OnActive：

```c++
AGameplayCueNotify_Actor::HandleGameplayCue(AActor *,Type,const FGameplayCueParameters &) GameplayCueNotify_Actor.cpp:149
UGameplayCueSet::HandleGameplayCueNotify_Internal(AActor *,int,Type,FGameplayCueParameters &) GameplayCueSet.cpp:276
UGameplayCueSet::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &) GameplayCueSet.cpp:42
UGameplayCueManager::RouteGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:211
UGameplayCueManager::HandleGameplayCue(AActor *,FGameplayTag,Type,const FGameplayCueParameters &,EGameplayCueExecutionOptions) GameplayCueManager.cpp:153
UAbilitySystemComponent::InvokeGameplayCueEvent(FGameplayTag,Type,const FGameplayCueParameters &) AbilitySystemComponent.cpp:1161

UAbilitySystemComponent::NetMulticast_InvokeGameplayCueAdded_WithParams_Implementation(FGameplayTag,FPredictionKey,FGameplayCueParameters) AbilitySystemComponent.cpp:1368
```


关键：

```c++
	void PreReplicatedRemove(const struct FActiveGameplayCueContainer &InArray);
	void PostReplicatedAdd(const struct FActiveGameplayCueContainer &InArray);
	void PostReplicatedChange(const struct FActiveGameplayCueContainer &InArray) { }
```