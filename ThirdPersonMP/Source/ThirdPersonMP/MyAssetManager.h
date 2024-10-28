// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "MyAssetManager.generated.h"

/**
 * 
 */
UCLASS(BlueprintType)
class THIRDPERSONMP_API UMyAssetManager : public UAssetManager
{
	GENERATED_BODY()

public:
	/** Starts initial load, gets called from InitializeObjectReferences */
	virtual void StartInitialLoading() override;

	/** Finishes initial loading, gets called from end of Engine::Init() */
	virtual void FinishInitialLoading() override;

};

UCLASS()
class UMyObject : public UObject
{
	GENERATED_BODY()

public:
	bool SaveAsAsset(FString AssetPath, FString PackageName, FString ObjectName);

private:
	FString PrivateName = TEXT("Nothing is Impossible!!");
	void PrivateFunction();
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	FString MyName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	int32 MyAge;

	// will not serialized
	FString MyInfo;
	float MyWeight = 65.0;

	// Hard Ref
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	UMyObject* HardRef_A;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	UMyObject* HardRef_B;
	
	// Soft Ref
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Debug)
	TSoftObjectPtr<UMyObject> SoftRef_C;
};

UCLASS()
class UMyAssetUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category=Debug)
	static UObject* DebugLoadObject(FString ObjectPath);

	UFUNCTION(BlueprintCallable, Category=Debug)
	static UTexture2D* GenerateDebugTexture(FString TextureName, FString PackageName, int32 TextureWidth, int32 TextureHeight, FColor Color);


	static void DumpLinkerLoad(UPackage* Package);

	UFUNCTION(BlueprintCallable, Category=Debug)
	static void DumpObjectDependencies(FString ObjectPath);
};
