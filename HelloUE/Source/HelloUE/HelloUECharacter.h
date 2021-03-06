// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MyActor.h"
#include "MyObject.h"
#include "HelloUECharacter.generated.h"

UENUM()
enum EMyEnum
{
	Enum_1,
	Enum_2,
};


UCLASS(config=Game)
class AHelloUECharacter : public ACharacter
{
	GENERATED_BODY()

	/** Camera boom positioning the camera behind the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

	/** Follow camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FollowCamera;
public:
	AHelloUECharacter();

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UFUNCTION(BlueprintCallable, Category=Hello)
	void HelloWorld();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Hello)
	float MoveSpeed;

	// 当前关卡创建一个Actor
	UFUNCTION(BlueprintCallable, Category=Hello)
	AMyActor* SpawnMyActor(TSubclassOf<AMyActor> MyActorClass, const FName& MyName, const FVector& Location, const FRotator& Rotator = FRotator::ZeroRotator);


	// 支持GC
	UPROPERTY()
	UMyObject* MyObject1;
	// 不支持GC
	UMyObject* MyObject2;
	
	UMyObject* MyObject3;
	
	UFUNCTION(BlueprintCallable, Category=Hello)
	void CreateMyObjects();

	UFUNCTION(BlueprintCallable, Category=Hello)
	void DestroMyObjects();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Hello)
	UMyObject* MyObject;
	
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

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
};

