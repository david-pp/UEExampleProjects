
# 结构

ASC:


```c++

/** Data structure for montages that were instigated locally (everything if server, predictive if client. replicated if simulated proxy) */
UPROPERTY()
FGameplayAbilityLocalAnimMontage LocalAnimMontageInfo;

/** Data structure for replicating montage info to simulated clients */
UPROPERTY(ReplicatedUsing=OnRep_ReplicatedAnimMontage)
FGameplayAbilityRepAnimMontage RepAnimMontageInfo;

```

GA:

```c++
/** Active montage being played by this ability */
UPROPERTY()
class UAnimMontage* CurrentMontage;
```

# 关键操作

## 蒙太奇的播放

* AutonomousProxy模式下
    * 播放蒙太奇
    * 更新ASC.LocalAnimMontageInfo信息
    * 绑定预测失败的回调（UAbilitySystemComponent::OnPredictiveMontageRejected）
        * 若预测失败，直接停止蒙太奇
* Authority模式下
    * 播放蒙太奇
    * 更新ASC.LocalAnimMontageInfo信息
    * 更新ASC.RepAnimMontageInfo（支持属性同步，同步给SimulatedProxy）
* SimulatedProxy模式（UAbilitySystemComponent::OnRep_ReplicatedAnimMontage）
    * 根据同步过来的蒙塔奇信息，进行播放
        * 全新
        * 速率有变化 


AutonomousProxy/Authority情况下执行：

```c++
/** Plays a montage and handles replication and prediction based on passed in ability/activation info */
float UAbilitySystemComponent::PlayMontage(UGameplayAbility* InAnimatingAbility, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* NewAnimMontage, ...){
	float Duration = -1.f;

	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
	
    // 播放蒙太奇
	Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate, EMontagePlayReturnType::MontageLength, StartTimeSeconds);
	
	// 设置当前播放的蒙太奇信息		
    LocalAnimMontageInfo.AnimMontage = NewAnimMontage;
    LocalAnimMontageInfo.AnimatingAbility = InAnimatingAbility;
    LocalAnimMontageInfo.PlayBit = !LocalAnimMontageInfo.PlayBit;
			
    if (InAnimatingAbility)
    {
        InAnimatingAbility->SetCurrentMontage(NewAnimMontage);
    }
    
    // Start at a given Section.
    if (StartSectionName != NAME_None)
    {
        AnimInstance->Montage_JumpToSection(StartSectionName, NewAnimMontage);
    }

    // Server权威模式下执行
    if (IsOwnerActorAuthoritative())
    {
        FGameplayAbilityRepAnimMontage& MutableRepAnimMontageInfo = GetRepAnimMontageInfo_Mutable();

        // Those are static parameters, they are only set when the montage is played. They are not changed after that.
        MutableRepAnimMontageInfo.AnimMontage = NewAnimMontage;
        MutableRepAnimMontageInfo.ForcePlayBit = !bool(MutableRepAnimMontageInfo.ForcePlayBit);

        MutableRepAnimMontageInfo.SectionIdToPlay = 0;
        if (MutableRepAnimMontageInfo.AnimMontage && StartSectionName != NAME_None)
        {
            // we add one so INDEX_NONE can be used in the on rep
            MutableRepAnimMontageInfo.SectionIdToPlay = MutableRepAnimMontageInfo.AnimMontage->GetSectionIndex(StartSectionName) + 1;
        }

        // Update parameters that change during Montage life time.
        AnimMontage_UpdateReplicatedData();

        // Force net update on our avatar actor
        if (AbilityActorInfo->AvatarActor != nullptr)
        {
            AbilityActorInfo->AvatarActor->ForceNetUpdate();
        }
    }
    // AutonomyProxy下执行
    else
    {
        // If this prediction key is rejected, we need to end the preview
        FPredictionKey PredictionKey = GetPredictionKeyForNewAction();
        if (PredictionKey.IsValidKey())
        {
            PredictionKey.NewRejectedDelegate().BindUObject(this, &UAbilitySystemComponent::OnPredictiveMontageRejected, NewAnimMontage);
        }
    }
}

```


SimulatedProxy模式下有效：

```c++
/**	Replicated Event for AnimMontages */
void UAbilitySystemComponent::OnRep_ReplicatedAnimMontage() {

    // SimulatedProxy
    if (!AbilityActorInfo->IsLocallyControlled()) {

        // New Montage to play
        if ((LocalAnimMontageInfo.AnimMontage != ConstRepAnimMontageInfo.AnimMontage) || (LocalAnimMontageInfo.PlayBit != ReplicatedPlayBit))
        {
            LocalAnimMontageInfo.PlayBit = ReplicatedPlayBit;

            // 播放蒙太奇
            PlayMontageSimulated(ConstRepAnimMontageInfo.AnimMontage, ConstRepAnimMontageInfo.PlayRate) {
                UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
                if (AnimInstance && NewAnimMontage)
                {
                    Duration = AnimInstance->Montage_Play(NewAnimMontage, InPlayRate);
                    if (Duration > 0.f)
                    {
                        LocalAnimMontageInfo.AnimMontage = NewAnimMontage;
                    }
                }
            }
        }

        // Play Rate has changed
        if (AnimInstance->Montage_GetPlayRate(LocalAnimMontageInfo.AnimMontage) != ConstRepAnimMontageInfo.PlayRate)
        {
            AnimInstance->Montage_SetPlayRate(LocalAnimMontageInfo.AnimMontage, ConstRepAnimMontageInfo.PlayRate);
        }
        // ..
    }
}
```


AutonomousProxy模式下收到预测失败的处理：

```c++
void UAbilitySystemComponent::OnPredictiveMontageRejected(UAnimMontage* PredictiveMontage) {
	static const float MONTAGE_PREDICTION_REJECT_FADETIME = 0.25f;

	UAnimInstance* AnimInstance = AbilityActorInfo.IsValid() ? AbilityActorInfo->GetAnimInstance() : nullptr;
	if (AnimInstance && PredictiveMontage)
	{
		// If this montage is still playing: kill it
		if (AnimInstance->Montage_IsPlaying(PredictiveMontage))
		{
			AnimInstance->Montage_Stop(MONTAGE_PREDICTION_REJECT_FADETIME, PredictiveMontage);
		}
	}
}
```