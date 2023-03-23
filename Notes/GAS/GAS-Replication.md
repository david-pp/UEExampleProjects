
# AbilitySystem Replication

## ASC Replication

支持：

* AS(AttributeSet)同步
* GT(GameplayTag)同步
* GC(GameplayCur)同步
* GE(GameplayEffect)根据ReplicationMode有所差异
* GA(GameplayAbility)同步


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
FActiveGameplayEffectsContainer::NetDeltaSerialize() {
}
```

## Replication Runtime

* Authority
* AutonomousProxy
* SimulatedProxy

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

* `REPNOTIFY_Always` - 
* `GAMEPLAYATTRIBUTE_REPNOTIFY` - 

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

# GC Replication

# GE Replication

# GA Replication



