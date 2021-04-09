// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloatingActor.generated.h"

//
// GC Type
//
UCLASS()
class UMyGCType : public UObject
{
	GENERATED_BODY()

public:
	UMyGCType();

	virtual ~UMyGCType();
};


//
// Hello Actor
//
UCLASS()
class G6PHYSICSPLUGINDEMO_API AFloatingActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFloatingActor();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called after the C++ constructor and after the properties have been initialized, including those loaded from config.
	// This is called before any serialization or other setup has happened.
	virtual void PostInitProperties() override;


#ifdef WITH_EDITOR
	// Called when a property on this object has been modified externally
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	//
	// C++ -> Editor/BP
	//
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* VisualMesh;

	// Properties 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FloatingActor")
	float FloatSpeed = 20;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FloatingActor")
	float RotationSpeed = 20;

	// Properties for Editor Operation
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	int32 TotalDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float DamageTimeInSeconds;

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Category = "Damage")
	float DamagePerSecond;

	// Functions
	UFUNCTION(BlueprintCallable, Category = "Damage")
	void CalculateValues();

	UFUNCTION(BlueprintCallable, Category = "FloatingActor")
	void CalledByBP(int Op);

public:
	//
	// BP -> C++
	//

	// BlueprintImplementableEvent vs. BlueprintNativeEvent:
	// 
	// - BlueprintImplementableEvent -> C++不需要实现，蓝图没有重载则为空
	// - BlueprintNativeEvent -> C++需要实现一个默认的，蓝图没有重载调用C++实现的，有重载则执行重载的
	// 
	UFUNCTION(BlueprintImplementableEvent, Category = "FloatingActor")
	void CalledFromCppBPImpl();

	UFUNCTION(BlueprintNativeEvent, Category = "FloatingActor")
	void CalledFromCppNative();

	UFUNCTION(BlueprintImplementableEvent, Category = "FloatingActor")
	int CalledFromCppWithReturn(const FString& str);


public:
	//
	// Garbage Collection
	//
	void CreateDoomedObject();

	UPROPERTY()
	UMyGCType* SafeObject;   // safe, reference 

	UMyGCType* DoomedObject; // unsafe, destroyed by GC
	
private:
	void DumpAllProperties();

	void DumpActors();
};
