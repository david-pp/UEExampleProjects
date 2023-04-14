// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CharacterBTComponent.h"
#include "CharacterBBComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeTypes.h"
#include "GameFramework/Character.h"
#include "ThirdPersonMPCharacter.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FCharacterBTDelegate, class UBehaviorTreeComponent& BTC)

struct FCharacterAnimNotifyDelegateInfo
{
	FDelegateHandle Handle;
	FCharacterBTDelegate BTDelegate;
	float Ratio = 0.f;
	bool bIsTriggered = false;
};

UCLASS(config=Game)
class AThirdPersonMPCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;

public:
	AThirdPersonMPCharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

protected:
	/** Resets HMD orientation in VR. */
	void OnResetVR();

	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface


	/** The player's maximum health. This is the highest that their health can be, and the value that their health starts at when spawned.*/
	UPROPERTY(EditDefaultsOnly, Category = "Health")
	float MaxHealth;

	/** The player's current health. When reduced to 0, they are considered dead.*/
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth)
	float CurrentHealth;

	/** RepNotify for changes made to current health.*/
	UFUNCTION()
	void OnRep_CurrentHealth();

	/** Response to health being updated. Called on the server immediately after modification, and on clients in response to a RepNotify*/
	void OnHealthUpdate();

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }


	/** Property replication */
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;


	/** Getter for Max Health.*/
	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }

	/** Getter for Current Health.*/
	UFUNCTION(BlueprintPure, Category = "Health")
	FORCEINLINE float GetCurrentHealth() const { return CurrentHealth; }

	/** Setter for Current Health. Clamps the value between 0 and MaxHealth and calls OnHealthUpdate. Should only be called on the server.*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	void SetCurrentHealth(float healthValue);

	/** Event for taking damage. Overridden from APawn.*/
	UFUNCTION(BlueprintCallable, Category = "Health")
	float TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay|Projectile")
	TSubclassOf<class AThirdPersonMPProjectile> ProjectileClass;

	/** Delay between shots in seconds. Used to control fire rate for our test projectile, but also to prevent an overflow of server functions from binding SpawnProjectile directly to input.*/
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay")
	float FireRate;

	/** If true, we are in the process of firing projectiles. */
	bool bIsFiringWeapon;

	/** Function for beginning weapon fire.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StartFire();

	/** Function for ending weapon fire. Once this is called, the player can use StartFire again.*/
	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	void StopFire();

	/** Server function for spawning projectiles.*/
	UFUNCTION(Server, Reliable)
	void HandleFire();

	/** A timer handle used for providing the fire rate delay in-between spawns.*/
	FTimerHandle FiringTimer;

public:
	//
	// Examples for BT
	// 

	UPROPERTY(BlueprintReadWrite)
	bool bIsMoving = false;

	FCharacterBTDelegate OnMovingDelegates;

	void OnMontageAdvanced(UBehaviorTreeComponent& BTComp, float LastRatio, float CurrentRatio);

	TMultiMap<FDelegateHandle, FCharacterAnimNotifyDelegateInfo> OnAnimNotifyHandles;

public:
	//
	// Examples for BT
	//

	/** Component responsible for behaviors. */
	UPROPERTY(BlueprintReadWrite, Category = AI)
	UCharacterBTComponent* BTComponent;

	/** Component responsible for different action behaviors: <ActionName, BTC> */
	UPROPERTY(BlueprintReadWrite, Category = AI)
	TMap<FName, UCharacterBTComponent*> ActionBTComponents;

	UPROPERTY(BlueprintReadWrite, Category = AI)
	UCharacterBBComponent* Blackboard;


	/** Starts executing behavior tree. */
	UFUNCTION(BlueprintCallable, Category = "AI")
	bool RunBehaviorTree(UBehaviorTree* BTAsset);
	UFUNCTION(BlueprintCallable, Category = "AI")
	bool RunBehaviorTreeSingleRun(UBehaviorTree* BTAsset);
	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopBehaviorTree();

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool RunActionBehaviorTree(FName Action, UBehaviorTree* BTAsset);
	UFUNCTION(BlueprintCallable, Category = "AI")
	void StopActionBehaviorTree(FName Action);
	UFUNCTION(Server, Reliable)
	void RunActionBehaviorTreeOnServer(FName Action, UBehaviorTree* BTAsset);

	// Call by Local
	UFUNCTION(BlueprintCallable, Category = "AI")
	void RunBehaviorTreeEx(UBehaviorTree* BTAsset, bool Autonoumous = true, bool Authority = false, bool Simulated = false);

	UFUNCTION(Server, Reliable)
	void RunBehaviorTreeOnServer(UBehaviorTree* BTAsset, bool Autonoumous = true, bool Authority = false, bool Simulated = false);
	UFUNCTION(NetMulticast, Reliable)
	void RunBehaviorTreeOnClients(UBehaviorTree* BTAsset, bool Autonoumous = true, bool Authority = false, bool Simulated = false);


	virtual bool RunBehaviorTreeImpl(UBehaviorTree* BTAsset, EBTExecutionMode::Type ExecuteMode = EBTExecutionMode::Looped);

	UFUNCTION(BlueprintCallable, Category = "AI")
	bool UseBlackboard(UBlackboardData* BlackboardAsset, UBlackboardComponent*& BlackboardComponent);

	//
	// /** does this AIController allow given UBlackboardComponent sync data with it */
	// virtual bool ShouldSyncBlackboardWith(const UBlackboardComponent& OtherBlackboardComponent) const;
	//
	//
	UFUNCTION(BlueprintImplementableEvent)
	void OnUsingBlackBoard(UBlackboardComponent* BlackboardComp, UBlackboardData* BlackboardAsset);

	virtual bool InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset);
};
