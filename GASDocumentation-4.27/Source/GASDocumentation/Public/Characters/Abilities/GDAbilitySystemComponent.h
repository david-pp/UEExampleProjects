// Copyright 2020 Dan Kestranek.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GASDocumentation/GASDocumentation.h"
#include "GDAbilitySystemComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FReceivedDamageDelegate, UGDAbilitySystemComponent*, SourceASC, float, UnmitigatedDamage, float, MitigatedDamage);


inline EAbilityGenericReplicatedEvent::Type InputID2EventType(EGDAbilityInputID InputID)
{
	switch (InputID)
	{
	case EGDAbilityInputID::Ability1: return EAbilityGenericReplicatedEvent::GameCustom1;
	case EGDAbilityInputID::Ability2: return EAbilityGenericReplicatedEvent::GameCustom2;
	case EGDAbilityInputID::Ability3: return EAbilityGenericReplicatedEvent::GameCustom3;
	case EGDAbilityInputID::Jump: return EAbilityGenericReplicatedEvent::GameCustom4;
	}
	return EAbilityGenericReplicatedEvent::GameCustom6;
}

inline EGDAbilityInputID EventType2InputID(EAbilityGenericReplicatedEvent::Type EventType)
{
	switch (EventType)
	{
	case EAbilityGenericReplicatedEvent::GameCustom1: return EGDAbilityInputID::Ability1;
	case EAbilityGenericReplicatedEvent::GameCustom2: return EGDAbilityInputID::Ability2;
	case EAbilityGenericReplicatedEvent::GameCustom3: return EGDAbilityInputID::Ability3;
	case EAbilityGenericReplicatedEvent::GameCustom4: return EGDAbilityInputID::Jump;
	}
	return EGDAbilityInputID::None;
}


/** Target data with just a source and target location in space */
USTRUCT(BlueprintType)
struct FGameplayAbilityTargetData_Custom : public FGameplayAbilityTargetData
{
	GENERATED_USTRUCT_BODY()

	/** Generic location data for source */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Targeting)
	float Value = 0.f;
	
	virtual UScriptStruct* GetScriptStruct() const override
	{
		return FGameplayAbilityTargetData_Custom::StaticStruct();
	}

	virtual FString ToString() const override
	{
		return TEXT("FGameplayAbilityTargetData_Custom");
	}

	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		Ar << Value;
		bOutSuccess = true;
		return true;
	}
};

template<>
struct TStructOpsTypeTraits<FGameplayAbilityTargetData_Custom> : public TStructOpsTypeTraitsBase2<FGameplayAbilityTargetData_Custom>
{
	enum
	{
		WithNetSerializer = true	// For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
	};
};

/**
 * 
 */
UCLASS()
class GASDOCUMENTATION_API UGDAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	bool CharacterAbilitiesGiven = false;
	bool StartupEffectsApplied = false;

	FReceivedDamageDelegate ReceivedDamage;

	// Called from GDDamageExecCalculation. Broadcasts on ReceivedDamage whenever this ASC receives damage.
	virtual void ReceiveDamage(UGDAbilitySystemComponent* SourceASC, float UnmitigatedDamage, float MitigatedDamage);

	virtual void OnServerPrintDebug_Request() override;
	virtual void OnClientPrintDebug_Response(const TArray<FString>& Strings, int32 GameFlags) override;

	UFUNCTION(BlueprintCallable)
	void DebugServerAsc();

	UFUNCTION(Server, Reliable, BlueprintCallable)
	void DebugRequest(const FString& Command);

	UFUNCTION(Client, Reliable)
	void DebugReply(const FString& Command, const TArray<FString>& Strings);

public:
	virtual void AbilityLocalInputPressed(int32 InputID) override;
	virtual void AbilityLocalInputReleased(int32 InputID) override;


	virtual void NotifyAbilityActivated(const FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability) override;
	virtual void NotifyAbilityEnded(FGameplayAbilitySpecHandle Handle, UGameplayAbility* Ability, bool bWasCancelled) override;


	UPROPERTY()
	class UGDGameplayAbility* CurrentActiveAbility;
	
	FGameplayAbilitySpecHandle ActiveAbilitySpecHandle;
};
