// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor.generated.h"

UCLASS()
class HELLOUE_API AMyActor : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMyActor();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// per-instance modified values
	virtual void PostInitProperties() override;

#ifdef WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	// Called when removed from the level
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	void CalculateDPS();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=HelloUE)
	int32 TotalDamage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HelloUE")
	float DamageTimeInSeconds;

	// 中间变量(Transient-不保存设置)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category=HelloUE)
	float DamagePerSecond;

	UPROPERTY(BlueprintReadWrite, Category="HelloUE", meta=(ExposeOnSpawn=true))
	FName MyName;

public:
	// ...... Asset Reference Snippets - Hard Reference 

	/** Hard Object Reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HelloUE")
	UStaticMeshComponent* BodyMesh;

	/** Hard Object Reference: 前面有class时，可以不用包含所在头文件，或前向声明 */
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HelloUE")
	// class UGameplayAbility* GrandAbility;

	/** Hard Class Reference */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HelloUE")
	UClass* ClassRef;

	/** Hard Class Reference: TSubclassOf 是安全类型，同时具有筛选功能 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="HelloUE")
	TSubclassOf<AActor> ActorClassRef;

	// ...... Asset Reference Snippets - Soft Reference

	/** Soft Object Reference - FSoftObjectPath */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObject", meta = (AllowedClasses = "SkeletalMesh, StaticMesh" ))
	FSoftObjectPath SoftObjectPath1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObject", meta = (AllowedClasses = "Texture2D"))
	FSoftObjectPath SoftObjectPath2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObject", meta = (AllowedClasses = "Blueprint Class"))
	FSoftObjectPath SoftObjectPath3;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObject", meta = (AllowedClasses = "MyActor")) //自定义类型 不推荐
	FSoftObjectPath SoftObjectPath4;

	/** Soft Object Reference - FSoftClassPath */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObjectClass")
	FSoftClassPath SoftClassPath;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObjectClass", meta = (MetaClass= "Pawn"))
	FSoftClassPath SoftClassPath_Pawn;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObjectClass", meta = (MetaClass = "MyActor"))
	FSoftClassPath SoftClassPath_MyActor;


	/** Soft Object Reference - TSoftObjectPtr<T> */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObjectPtr")
	TSoftObjectPtr<UObject> SoftObjectPtr1;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObjectPtr")
	TSoftObjectPtr<UObject> SoftObjectPtr2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftObjectPtr")
	TSoftObjectPtr<UTexture2D> SoftObjectPtr_Texture2D;

	UFUNCTION(BlueprintCallable)
	UTexture2D* GetLazyLoadTexture2D();

	/** Soft Object Reference - TSoftClassPtr<T> */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftClassPtr")
	TSoftClassPtr<AActor> SoftClassPtr_Actor;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftClassPtr")
	TSoftClassPtr<AMyActor> SoftClassPtr_Actor2;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "SoftClassPtr")
	TSoftClassPtr<UUserWidget> SoftClassPtr_UserWidget;
	
};
