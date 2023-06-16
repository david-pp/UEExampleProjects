```c++

/** Policy for the duration of this effect */
UPROPERTY(EditDefaultsOnly, Category=GameplayEffect)
EGameplayEffectDurationType DurationPolicy;

/** Duration in seconds. 0.0 for instantaneous effects; -1.0 for infinite duration. */
UPROPERTY(EditDefaultsOnly, Category=GameplayEffect)
FGameplayEffectModifierMagnitude DurationMagnitude;

/** Period in seconds. 0.0 for non-periodic effects */
UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Period)
FScalableFloat	Period;
```


## Modifier

查看属性聚合和Modifier列表信息：


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


Aggreator刷新CurrentValue：

```c++
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
```


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


## Execution


### Definition


### Excute

Stack：

```c++
UGDDamageExecCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters &,FGameplayEffectCustomExecutionOutput &) GDDamageExecCalculation.cpp:46
UGameplayEffectExecutionCalculation::Execute(const FGameplayEffectCustomExecutionParameters &,FGameplayEffectCustomExecutionOutput &) GameplayEffectExecutionCalculation.gen.cpp:256
FActiveGameplayEffectsContainer::ExecuteActiveEffectsFrom(FGameplayEffectSpec &,FPredictionKey) GameplayEffect.cpp:1993
UAbilitySystemComponent::ExecuteGameplayEffect(FGameplayEffectSpec &,FPredictionKey) AbilitySystemComponent.cpp:934
UAbilitySystemComponent::ApplyGameplayEffectSpecToSelf(const FGameplayEffectSpec &,FPredictionKey) AbilitySystemComponent.cpp:839
```

```c++
// Run the custom execution
FGameplayEffectCustomExecutionParameters ExecutionParams(SpecToUse, CurExecDef.CalculationModifiers, Owner, CurExecDef.PassedInTags, PredictionKey);
FGameplayEffectCustomExecutionOutput ExecutionOutput;
ExecCDO->Execute(ExecutionParams, ExecutionOutput);
```


### Capture/Sending Data

```c++

// SetByCallers
const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
float Damage = FMath::Max<float>(Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), false, -1.0f), 0.0f);
```

```c++
// Backing Data Attribute Calculation Modifier
ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageDef, EvaluationParameters, Damage);
```


```c++
// UGDDamageExecCalculation::UGDDamageExecCalculation
ValidTransientAggregatorIdentifiers.AddTag(FGameplayTag::RequestGameplayTag("Data.Damage"));

// Backing Data Temporary Variable Calculation Modifier
float Damage1 = 0.0f;
ExecutionParams.AttemptCalculateTransientAggregatorMagnitude(FGameplayTag::RequestGameplayTag("Data.Damage"),
                                                                EvaluationParameters, Damage1);
```


```c++
const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
FGSGameplayEffectContext* ContextHandle = static_cast<- FGSGameplayEffectContext*>(Spec.GetContext().Get());

FGameplayEffectSpec* MutableSpec = ExecutionParams.GetOwningSpecForPreExecuteMod();
FGSGameplayEffectContext* ContextHandle = static_cast<FGSGameplayEffectContext*>(MutableSpec->GetContext().Get());
```

## Tags


FGameplayEffectSpec:

```c++
	/** Captured Source Tags on GameplayEffectSpec creation */
	UPROPERTY(NotReplicated)
	FTagContainerAggregator	CapturedSourceTags;

	/** Tags from the target, captured during execute */
	UPROPERTY(NotReplicated)
	FTagContainerAggregator	CapturedTargetTags;

    /** Tags that are granted and that did not come from the UGameplayEffect def. These are replicated. */
	UPROPERTY()
	FGameplayTagContainer DynamicGrantedTags;

	/** Tags that are on this effect spec and that did not come from the UGameplayEffect def. These are replicated. */
	UPROPERTY()
	FGameplayTagContainer DynamicAssetTags;


    /** Appends all tags granted by this gameplay effect spec */
	void GetAllGrantedTags(OUT FGameplayTagContainer& Container) const;

	/** Appends all tags that apply to this gameplay effect spec */
	void GetAllAssetTags(OUT FGameplayTagContainer& Container) const;

```

GESpec::Initialize :

```c++

// Add the GameplayEffect asset tags to the source Spec tags
CapturedSourceTags.GetSpecTags().AppendTags(InDef->InheritableGameplayEffectTags.CombinedTags);

```


## Query

FGameplayEffectQuery:

```c++
    /** Native delegate for providing custom matching conditions. */
	FActiveGameplayEffectQueryCustomMatch CustomMatchDelegate;

	/** BP-exposed delegate for providing custom matching conditions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Query)
	FActiveGameplayEffectQueryCustomMatch_Dynamic CustomMatchDelegate_BP;

	/** Query that is matched against tags this GE gives */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Query)
	FGameplayTagQuery OwningTagQuery;

	/** Query that is matched against tags this GE has */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Query)
	FGameplayTagQuery EffectTagQuery;

	/** Query that is matched against tags the source of this GE has */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Query)
	FGameplayTagQuery SourceTagQuery;

    /** Matches on GameplayEffects which modify given attribute. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Query)
	FGameplayAttribute ModifyingAttribute;

	/** Matches on GameplayEffects which come from this source */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Query)
	const UObject* EffectSource;

	/** Matches on GameplayEffects with this definition */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Query)
	TSubclassOf<UGameplayEffect> EffectDefinition;

    /** Returns true if Effect matches all specified criteria of this query, including CustomMatch delegates if bound. Returns false otherwise. */
	bool Matches(const FActiveGameplayEffect& Effect) const;

	/** Returns true if Effect matches all specified criteria of this query. This does NOT check FActiveGameplayEffectQueryCustomMatch since this is performed on the spec (possibly prior to applying).
	 *	Note: it would be reasonable to support a custom delegate that operated on the FGameplayEffectSpec itself.
	 */
	bool Matches(const FGameplayEffectSpec& Effect) const;
```


```c++

bool FGameplayEffectQuery::Matches(const FGameplayEffectSpec& Spec) const
{
	if (Spec.Def == nullptr)
	{
		return false;
	}

	if (OwningTagQuery.IsEmpty() == false)
	{
		// Combine tags from the definition and the spec into one container to match queries that may span both
		// static to avoid memory allocations every time we do a query
		check(IsInGameThread());
		static FGameplayTagContainer TargetTags;
		TargetTags.Reset();
		if (Spec.Def->InheritableGameplayEffectTags.CombinedTags.Num() > 0)
		{
			TargetTags.AppendTags(Spec.Def->InheritableGameplayEffectTags.CombinedTags);
		}
		if (Spec.Def->InheritableOwnedTagsContainer.CombinedTags.Num() > 0)
		{
			TargetTags.AppendTags(Spec.Def->InheritableOwnedTagsContainer.CombinedTags);
		}
		if (Spec.DynamicGrantedTags.Num() > 0)
		{
			TargetTags.AppendTags(Spec.DynamicGrantedTags);
		}
		
		if (OwningTagQuery.Matches(TargetTags) == false)
		{
			return false;
		}
	}

	if (EffectTagQuery.IsEmpty() == false)
	{
		// Combine tags from the definition and the spec into one container to match queries that may span both
		// static to avoid memory allocations every time we do a query
		check(IsInGameThread());
		static FGameplayTagContainer GETags;
		GETags.Reset();
		if (Spec.Def->InheritableGameplayEffectTags.CombinedTags.Num() > 0)
		{
			GETags.AppendTags(Spec.Def->InheritableGameplayEffectTags.CombinedTags);
		}
		if (Spec.DynamicAssetTags.Num() > 0)
		{
			GETags.AppendTags(Spec.DynamicAssetTags);
		}

		if (EffectTagQuery.Matches(GETags) == false)
		{
			return false;
		}
	}

	if (SourceTagQuery.IsEmpty() == false)
	{
		FGameplayTagContainer const& SourceTags = Spec.CapturedSourceTags.GetSpecTags();
		if (SourceTagQuery.Matches(SourceTags) == false)
		{
			return false;
		}
	}

	// if we are looking for ModifyingAttribute go over each of the Spec Modifiers and check the Attributes
	if (ModifyingAttribute.IsValid())
	{
		bool bEffectModifiesThisAttribute = false;

		for (int32 ModIdx = 0; ModIdx < Spec.Modifiers.Num(); ++ModIdx)
		{
			const FGameplayModifierInfo& ModDef = Spec.Def->Modifiers[ModIdx];
			const FModifierSpec& ModSpec = Spec.Modifiers[ModIdx];

			if (ModDef.Attribute == ModifyingAttribute)
			{
				bEffectModifiesThisAttribute = true;
				break;
			}
		}
		if (bEffectModifiesThisAttribute == false)
		{
			// effect doesn't modify the attribute we are looking for, no match
			return false;
		}
	}

	// check source object
	if (EffectSource != nullptr)
	{
		if (Spec.GetEffectContext().GetSourceObject() != EffectSource)
		{
			return false;
		}
	}

	// check definition
	if (EffectDefinition != nullptr)
	{
		if (Spec.Def != EffectDefinition.GetDefaultObject())
		{
			return false;
		}
	}

	return true;
}
```

ASC：

```c++

/** Gets time remaining for all effects that match query */
	TArray<float> GetActiveEffectsTimeRemaining(const FGameplayEffectQuery& Query) const;

	/** Gets total duration for all effects that match query */
	TArray<float> GetActiveEffectsDuration(const FGameplayEffectQuery& Query) const;

	/** Gets both time remaining and total duration  for all effects that match query */
	TArray<TPair<float,float>> GetActiveEffectsTimeRemainingAndDuration(const FGameplayEffectQuery& Query) const;

	/** Returns list of active effects, for a query */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "GameplayEffects", meta=(DisplayName = "Get Activate Gameplay Effects for Query"))
	TArray<FActiveGameplayEffectHandle> GetActiveEffects(const FGameplayEffectQuery& Query) const;

    /** Returns debug string describing active gameplay effect */
	FString GetActiveGEDebugString(FActiveGameplayEffectHandle Handle) const;

	/** Gets the GE Handle of the GE that granted the passed in Ability */
	FActiveGameplayEffectHandle FindActiveGameplayEffectHandle(FGameplayAbilitySpecHandle Handle) const;

	/** Returns const pointer to the actual active gamepay effect structure */
	const FActiveGameplayEffect* GetActiveGameplayEffect(const FActiveGameplayEffectHandle Handle) const;

```


## Immunity


ApplyGESpecToSelf:

```c++
    // Are we currently immune to this? (ApplicationImmunity)
	const FActiveGameplayEffect* ImmunityGE=nullptr;
	if (ActiveGameplayEffects.HasApplicationImmunityToSpec(Spec, ImmunityGE))
	{
		OnImmunityBlockGameplayEffect(Spec, ImmunityGE);
		return FActiveGameplayEffectHandle();
	}
```


## Stacking

```c++
	// ----------------------------------------------------------------------
	//	Stacking
	// ----------------------------------------------------------------------
	
	/** How this GameplayEffect stacks with other instances of this same GameplayEffect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingType	StackingType;

	/** Stack limit for StackingType */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	int32 StackLimitCount;

	/** Policy for how the effect duration should be refreshed while stacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingDurationPolicy StackDurationRefreshPolicy;

	/** Policy for how the effect period should be reset (or not) while stacking */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingPeriodPolicy StackPeriodResetPolicy;

	/** Policy for how to handle duration expiring on this gameplay effect */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Stacking)
	EGameplayEffectStackingExpirationPolicy StackExpirationPolicy;
```


```c++
FActiveGameplayEffect* FActiveGameplayEffectsContainer::FindStackableActiveGameplayEffect(const FGameplayEffectSpec& Spec)
{
	FActiveGameplayEffect* StackableGE = nullptr;
	const UGameplayEffect* GEDef = Spec.Def;
	EGameplayEffectStackingType StackingType = GEDef->StackingType;

	if ((StackingType != EGameplayEffectStackingType::None) && (GEDef->DurationPolicy != EGameplayEffectDurationType::Instant))
	{
		// Iterate through GameplayEffects to see if we find a match. Note that we could cache off a handle in a map but we would still
		// do a linear search through GameplayEffects to find the actual FActiveGameplayEffect (due to unstable nature of the GameplayEffects array).
		// If this becomes a slow point in the profiler, the map may still be useful as an early out to avoid an unnecessary sweep.
		UAbilitySystemComponent* SourceASC = Spec.GetContext().GetInstigatorAbilitySystemComponent();
		for (FActiveGameplayEffect& ActiveEffect: this)
		{
			// Aggregate by source stacking additionally requires the source ability component to match
			if (ActiveEffect.Spec.Def == Spec.Def && ((StackingType == EGameplayEffectStackingType::AggregateByTarget) || (SourceASC && SourceASC == ActiveEffect.Spec.GetContext().GetInstigatorAbilitySystemComponent())))
			{
				StackableGE = &ActiveEffect;
				break;
			}
		}
	}

	return StackableGE;
}
```

处理Stacking的逻辑：

```c++
// Check if there's an active GE this application should stack upon
	if (ExistingStackableGE)
	{
		if (!IsNetAuthority())
		{
			// Don't allow prediction of stacking for now
			return nullptr;
		}
		else
		{
			// Server invalidates the prediction key for this GE since client is not predicting it
			InPredictionKey = FPredictionKey();
		}

		bFoundExistingStackableGE = true;

		FGameplayEffectSpec& ExistingSpec = ExistingStackableGE->Spec;
		StartingStackCount = ExistingSpec.StackCount;

		// This is now the global "being applied GE"
		UAbilitySystemGlobals::Get().SetCurrentAppliedGE(&ExistingSpec);
		
		// How to apply multiple stacks at once? What if we trigger an overflow which can reject the application?
		// We still want to apply the stacks that didn't push us over, but we also want to call HandleActiveGameplayEffectStackOverflow.
		
		// For now: call HandleActiveGameplayEffectStackOverflow only if we are ALREADY at the limit. Else we just clamp stack limit to max.
		if (ExistingSpec.StackCount == ExistingSpec.Def->StackLimitCount)
		{
			if (!HandleActiveGameplayEffectStackOverflow(*ExistingStackableGE, ExistingSpec, Spec))
			{
				return nullptr;
			}
		}
		
		NewStackCount = ExistingSpec.StackCount + Spec.StackCount;
		if (ExistingSpec.Def->StackLimitCount > 0)
		{
			NewStackCount = FMath::Min(NewStackCount, ExistingSpec.Def->StackLimitCount);
		}

		// Need to unregister callbacks because the source aggregators could potentially be different with the new application. They will be
		// re-registered later below, as necessary.
		ExistingSpec.CapturedRelevantAttributes.UnregisterLinkedAggregatorCallbacks(ExistingStackableGE->Handle);

		// @todo: If dynamically granted tags differ (which they shouldn't), we'll actually have to diff them
		// and cause a removal and add of only the ones that have changed. For now, ensure on this happening and come
		// back to this later.
		ensureMsgf(ExistingSpec.DynamicGrantedTags == Spec.DynamicGrantedTags, TEXT("While adding a stack of the gameplay effect: %s, the old stack and the new application had different dynamically granted tags, which is currently not resolved properly!"), *Spec.Def->GetName());
		
		// We only grant abilities on the first apply. So we *dont* want the new spec's GrantedAbilitySpecs list
		TArray<FGameplayAbilitySpecDef>	GrantedSpecTempArray(MoveTemp(ExistingStackableGE->Spec.GrantedAbilitySpecs));

		// @todo: If dynamic asset tags differ (which they shouldn't), we'll actually have to diff them
		// and cause a removal and add of only the ones that have changed. For now, ensure on this happening and come
		// back to this later.
		ensureMsgf(ExistingSpec.DynamicAssetTags == Spec.DynamicAssetTags, TEXT("While adding a stack of the gameplay effect: %s, the old stack and the new application had different dynamic asset tags, which is currently not resolved properly! Existing: %s. New: %s"), *Spec.Def->GetName(), *ExistingSpec.DynamicAssetTags.ToStringSimple(), *Spec.DynamicAssetTags.ToStringSimple() );

		ExistingStackableGE->Spec = Spec;
		ExistingStackableGE->Spec.StackCount = NewStackCount;

		// Swap in old granted ability spec
		ExistingStackableGE->Spec.GrantedAbilitySpecs = MoveTemp(GrantedSpecTempArray);
		
		AppliedActiveGE = ExistingStackableGE;

		const UGameplayEffect* GEDef = ExistingSpec.Def;

		// Make sure the GE actually wants to refresh its duration
		if (GEDef->StackDurationRefreshPolicy == EGameplayEffectStackingDurationPolicy::NeverRefresh)
		{
			bSetDuration = false;
		}
		else
		{
			RestartActiveGameplayEffectDuration(*ExistingStackableGE);
		}

		// Make sure the GE actually wants to reset its period
		if (GEDef->StackPeriodResetPolicy == EGameplayEffectStackingPeriodPolicy::NeverReset)
		{
			bSetPeriod = false;
		}
	}

    ```

    Stacking到期检查的逻辑：
    ```c++
    
void FActiveGameplayEffectsContainer::CheckDuration(FActiveGameplayEffectHandle Handle)
{
	GAMEPLAYEFFECT_SCOPE_LOCK();
	// Intentionally iterating through only the internal list since we need to pass the index for removal
	// and pending effects will never need to be checked for duration expiration (They will be added to the real list first)
	for (int32 ActiveGEIdx = 0; ActiveGEIdx < GameplayEffects_Internal.Num(); ++ActiveGEIdx)
	{
		FActiveGameplayEffect& Effect = GameplayEffects_Internal[ActiveGEIdx];
		if (Effect.Handle == Handle)
		{
			if (Effect.IsPendingRemove)
			{
				// break is this effect is pending remove. 
				// (Note: don't combine this with the above if statement that is looking for the effect via handle, since we want to stop iteration if we find a matching handle but are pending remove).
				break;
			}

			FTimerManager& TimerManager = Owner->GetWorld()->GetTimerManager();

			// The duration may have changed since we registered this callback with the timer manager.
			// Make sure that this effect should really be destroyed now
			float Duration = Effect.GetDuration();
			float CurrentTime = GetWorldTime();

			int32 StacksToRemove = -2;
			bool RefreshStartTime = false;
			bool RefreshDurationTimer = false;
			bool CheckForFinalPeriodicExec = false;

			if (Duration > 0.f && (((Effect.StartWorldTime + Duration) < CurrentTime) || FMath::IsNearlyZero(CurrentTime - Duration - Effect.StartWorldTime, KINDA_SMALL_NUMBER)))
			{
				// Figure out what to do based on the expiration policy
				switch(Effect.Spec.Def->StackExpirationPolicy)
				{
				case EGameplayEffectStackingExpirationPolicy::ClearEntireStack:
					StacksToRemove = -1; // Remove all stacks
					CheckForFinalPeriodicExec = true;					
					break;

				case EGameplayEffectStackingExpirationPolicy::RemoveSingleStackAndRefreshDuration:
					StacksToRemove = 1;
					CheckForFinalPeriodicExec = (Effect.Spec.StackCount == 1);
					RefreshStartTime = true;
					RefreshDurationTimer = true;
					break;
				case EGameplayEffectStackingExpirationPolicy::RefreshDuration:
					RefreshStartTime = true;
					RefreshDurationTimer = true;
					break;
				};					
			}
			else
			{
				// Effect isn't finished, just refresh its duration timer
				RefreshDurationTimer = true;
			}

			if (CheckForFinalPeriodicExec)
			{
				// This gameplay effect has hit its duration. Check if it needs to execute one last time before removing it.
				if (Effect.PeriodHandle.IsValid() && TimerManager.TimerExists(Effect.PeriodHandle))
				{
					float PeriodTimeRemaining = TimerManager.GetTimerRemaining(Effect.PeriodHandle);
					if (PeriodTimeRemaining <= KINDA_SMALL_NUMBER && !Effect.bIsInhibited)
					{
						InternalExecutePeriodicGameplayEffect(Effect);

						// The call to ExecuteActiveEffectsFrom in InternalExecutePeriodicGameplayEffect could cause this effect to be explicitly removed
						// (for example it could kill the owner and cause the effect to be wiped via death).
						// In that case, we need to early out instead of possibly continuing to the below calls to InternalRemoveActiveGameplayEffect
						if ( Effect.IsPendingRemove )
						{
							break;
						}
					}

					// Forcibly clear the periodic ticks because this effect is going to be removed
					TimerManager.ClearTimer(Effect.PeriodHandle);
				}
			}

			if (StacksToRemove >= -1)
			{
				InternalRemoveActiveGameplayEffect(ActiveGEIdx, StacksToRemove, false);
			}

			if (RefreshStartTime)
			{
				RestartActiveGameplayEffectDuration(Effect);
			}

			if (RefreshDurationTimer)
			{
				// Always reset the timer, since the duration might have been modified
				FTimerDelegate Delegate = FTimerDelegate::CreateUObject(Owner, &UAbilitySystemComponent::CheckDurationExpired, Effect.Handle);

				float NewTimerDuration = (Effect.StartWorldTime + Duration) - CurrentTime;
				TimerManager.SetTimer(Effect.DurationHandle, Delegate, NewTimerDuration, false);

				if (Effect.DurationHandle.IsValid() == false)
				{
					ABILITY_LOG(Warning, TEXT("Failed to set new timer in ::CheckDuration. Timer trying to be set for: %.2f. Removing GE instead"), NewTimerDuration);
					if (!Effect.IsPendingRemove)
					{
						InternalRemoveActiveGameplayEffect(ActiveGEIdx, -1, false);
					}
					check(Effect.IsPendingRemove);
				}
			}

			break;
		}
	}
}

    ```


    Overflow的逻辑：

```c++
    bool FActiveGameplayEffectsContainer::HandleActiveGameplayEffectStackOverflow(const FActiveGameplayEffect& ActiveStackableGE, const FGameplayEffectSpec& OldSpec, const FGameplayEffectSpec& OverflowingSpec)
{
	const UGameplayEffect* StackedGE = OldSpec.Def;
	const bool bAllowOverflowApplication = !(StackedGE->bDenyOverflowApplication);

	FPredictionKey PredictionKey;
	for (TSubclassOf<UGameplayEffect> OverflowEffect : StackedGE->OverflowEffects)
	{
		if (const UGameplayEffect* CDO = OverflowEffect.GetDefaultObject())
		{
			FGameplayEffectSpec NewGESpec;
			NewGESpec.InitializeFromLinkedSpec(CDO, OverflowingSpec);
			Owner->ApplyGameplayEffectSpecToSelf(NewGESpec, PredictionKey);
		}
	}

	if (!bAllowOverflowApplication && StackedGE->bClearStackOnOverflow)
	{
		Owner->RemoveActiveGameplayEffect(ActiveStackableGE.Handle);
	}

	return bAllowOverflowApplication;
}
 ```

动态修改GE的Duration：

```c++
bool UPAAbilitySystemComponent::SetGameplayEffectDurationHandle(FActiveGameplayEffectHandle Handle, float NewDuration)
{
	if (!Handle.IsValid())
	{
		return false;
	}

	const FActiveGameplayEffect* ActiveGameplayEffect = GetActiveGameplayEffect(Handle);
	if (!ActiveGameplayEffect)
	{
		return false;
	}

	FActiveGameplayEffect* AGE = const_cast<FActiveGameplayEffect*>(ActiveGameplayEffect);
	if (NewDuration > 0)
	{
		AGE->Spec.Duration = NewDuration;
	}
	else
	{
		AGE->Spec.Duration = 0.01f;
	}

	AGE->StartServerWorldTime = ActiveGameplayEffects.GetServerWorldTime();
	AGE->CachedStartServerWorldTime = AGE->StartServerWorldTime;
	AGE->StartWorldTime = ActiveGameplayEffects.GetWorldTime();
	ActiveGameplayEffects.MarkItemDirty(*AGE);
	ActiveGameplayEffects.CheckDuration(Handle);

	AGE->EventSet.OnTimeChanged.Broadcast(AGE->Handle, AGE->StartWorldTime, AGE->GetDuration());
	OnGameplayEffectDurationChange(*AGE);

	return true;
}

```




    ## Helper


    ```c++
	// -------------------------------------------------------------------------------
	//		GameplayEffectSpec
	// -------------------------------------------------------------------------------

	/** Sets a raw name Set By Caller magnitude value, the tag version should normally be used */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AssignSetByCallerMagnitude(FGameplayEffectSpecHandle SpecHandle, FName DataName, float Magnitude);

	/** Sets a gameplay tag Set By Caller magnitude value */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AssignTagSetByCallerMagnitude(FGameplayEffectSpecHandle SpecHandle, FGameplayTag DataTag, float Magnitude);

	/** Manually sets the duration on a specific effect */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle SetDuration(FGameplayEffectSpecHandle SpecHandle, float Duration);

	/** This instance of the effect will now grant NewGameplayTag to the object that this effect is applied to */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AddGrantedTag(FGameplayEffectSpecHandle SpecHandle, FGameplayTag NewGameplayTag);

	/** This instance of the effect will now grant NewGameplayTags to the object that this effect is applied to */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AddGrantedTags(FGameplayEffectSpecHandle SpecHandle, FGameplayTagContainer NewGameplayTags);

	/** Adds NewGameplayTag to this instance of the effect */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AddAssetTag(FGameplayEffectSpecHandle SpecHandle, FGameplayTag NewGameplayTag);

	/** Adds NewGameplayTags to this instance of the effect */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AddAssetTags(FGameplayEffectSpecHandle SpecHandle, FGameplayTagContainer NewGameplayTags);

	/** Adds LinkedGameplayEffectSpec to SpecHandles. LinkedGameplayEffectSpec will be applied when/if SpecHandle is applied successfully. LinkedGameplayEffectSpec will not be modified here. Returns the ORIGINAL SpecHandle (legacy decision) */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AddLinkedGameplayEffectSpec(FGameplayEffectSpecHandle SpecHandle, FGameplayEffectSpecHandle LinkedGameplayEffectSpec);

	/** Adds LinkedGameplayEffect to SpecHandles. LinkedGameplayEffectSpec will be applied when/if SpecHandle is applied successfully. This will initialize the LinkedGameplayEffect's Spec for you. Returns to NEW linked spec in case you want to add more to it. */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle AddLinkedGameplayEffect(FGameplayEffectSpecHandle SpecHandle, TSubclassOf<UGameplayEffect> LinkedGameplayEffect);

	/** Sets the GameplayEffectSpec's StackCount to the specified amount (prior to applying) */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle SetStackCount(FGameplayEffectSpecHandle SpecHandle, int32 StackCount);

	/** Sets the GameplayEffectSpec's StackCount to the max stack count defined in the GameplayEffect definition */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectSpecHandle SetStackCountToMax(FGameplayEffectSpecHandle SpecHandle);

	/** Gets the GameplayEffectSpec's effect context handle */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static FGameplayEffectContextHandle GetEffectContext(FGameplayEffectSpecHandle SpecHandle);

	/** Returns handles for all Linked GE Specs that SpecHandle may apply. Useful if you want to append additional information to them. */
	UFUNCTION(BlueprintPure, Category = "Ability|GameplayEffect")
	static TArray<FGameplayEffectSpecHandle> GetAllLinkedGameplayEffectSpecHandles(FGameplayEffectSpecHandle SpecHandle);

	// -------------------------------------------------------------------------------
	//		GameplayEffectSpec
	// -------------------------------------------------------------------------------

	/** Gets the magnitude of change for an attribute on an APPLIED GameplayEffectSpec. */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static float GetModifiedAttributeMagnitude(FGameplayEffectSpecHandle SpecHandle, FGameplayAttribute Attribute);

	/** Helper function that may be useful to call from native as well */
	static float GetModifiedAttributeMagnitude(const FGameplayEffectSpec& SpecHandle, FGameplayAttribute Attribute);

	// -------------------------------------------------------------------------------
	//		FActiveGameplayEffectHandle
	// -------------------------------------------------------------------------------
	
	/** Returns current stack count of an active Gameplay Effect. Will return 0 if the GameplayEffect is no longer valid. */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static int32 GetActiveGameplayEffectStackCount(FActiveGameplayEffectHandle ActiveHandle);

	/** Returns stack limit count of an active Gameplay Effect. Will return 0 if the GameplayEffect is no longer valid. */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static int32 GetActiveGameplayEffectStackLimitCount(FActiveGameplayEffectHandle ActiveHandle);

	/** Returns the start time (time which the GE was added) for a given GameplayEffect */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static float GetActiveGameplayEffectStartTime(FActiveGameplayEffectHandle ActiveHandle);

	/** Returns the expected end time (when we think the GE will expire) for a given GameplayEffect (note someone could remove or change it before that happens!) */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static float GetActiveGameplayEffectExpectedEndTime(FActiveGameplayEffectHandle ActiveHandle);

	/** Returns the total duration for a given GameplayEffect */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect")
	static float GetActiveGameplayEffectTotalDuration(FActiveGameplayEffectHandle ActiveHandle);

	/** Returns the total duration for a given GameplayEffect, basically ExpectedEndTime - Current Time */
	UFUNCTION(BlueprintCallable, Category = "Ability|GameplayEffect", meta = (WorldContext = "WorldContextObject"))
	static float GetActiveGameplayEffectRemainingDuration(UObject* WorldContextObject, FActiveGameplayEffectHandle ActiveHandle);

	/** Returns a debug string for display */
	UFUNCTION(BlueprintPure, Category = "Ability|GameplayEffect", Meta = (DisplayName = "Get Active GameplayEffect Debug String "))
	static FString GetActiveGameplayEffectDebugString(FActiveGameplayEffectHandle ActiveHandle);

    ```