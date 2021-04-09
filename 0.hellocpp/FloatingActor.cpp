// Fill out your copyright notice in the Description page of Project Settings.


#include "FloatingActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"


UMyGCType::UMyGCType()
{
	UE_LOG(LogTemp, Warning, TEXT("%s, %x"), TEXT(__FUNCTION__), this);
}

UMyGCType::~UMyGCType()
{
	UE_LOG(LogTemp, Warning, TEXT("%s, %x"), TEXT(__FUNCTION__), this);
}
////////////////////////////////////////////////////

// Sets default values
AFloatingActor::AFloatingActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Set Cube Static Mesh
	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	VisualMesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeVisualAsset(
		TEXT("/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube"));
	if (CubeVisualAsset.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeVisualAsset.Object);
		VisualMesh->SetRelativeLocation(FVector(0, 0, 0));
	}

	// Default Values
	TotalDamage = 200;
	DamageTimeInSeconds = 1.0f;
}

void AFloatingActor::PostInitProperties()
{
	Super::PostInitProperties();
	CalculateValues();
}

void AFloatingActor::CalculateValues()
{
	DamagePerSecond = TotalDamage / DamageTimeInSeconds;
}

// Called when the game starts or when spawned
void AFloatingActor::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void AFloatingActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	FVector NewLocation = GetActorLocation();
	FRotator NewRotation = GetActorRotation();

	float RunningTime = GetGameTimeSinceCreation();
	float DeltaHeight = FMath::Sin(RunningTime + DeltaTime) - FMath::Sin(RunningTime);
	NewLocation.Z += DeltaHeight * FloatSpeed;
	NewRotation.Yaw += DeltaTime * RotationSpeed;
	SetActorLocationAndRotation(NewLocation, NewRotation);
}

#ifdef WITH_EDITOR
void AFloatingActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	CalculateValues();

	Super::PostEditChangeProperty(PropertyChangedEvent);
}

#endif


void AFloatingActor::CalledByBP(int Op)
{
	UE_LOG(LogTemp, Warning, TEXT("%s - %d"), TEXT(__FUNCTION__), Op);

	switch (Op)
	{
	case 0: // dump all properties
		DumpAllProperties();
		break;
	case 1: // BlueprintImplementableEvent
		CalledFromCppBPImpl();
		break;
	case 2: // BlueprintNativeEvent
		CalledFromCppNative();
		break;
	case 3: // With return and params
		UE_LOG(LogTemp, Warning, TEXT("CalledFromCppWithReturn: %d"), CalledFromCppWithReturn("TestString"));
		break;
	case 4:
		DumpActors();
		break;
	case 5:
		CreateDoomedObject();
		break;
		
	default:
		break;
	}
}

void AFloatingActor::DumpActors()
{
	UWorld* World = this->GetWorld();
	if (World)
	{
		for (TActorIterator<AFloatingActor> It(World); It; ++It)
		{
			UE_LOG(LogTemp, Warning, TEXT("Actors: %s"), *It->GetActorLabel());
		}
	}
}


void AFloatingActor::DumpAllProperties()
{
	UClass* Class = AFloatingActor::GetClass();

	// Traverse Properties
	for (TFieldIterator<UProperty> It(Class, EFieldIteratorFlags::ExcludeSuper); It; ++It)
	{
		UProperty* Prop = *It;
		UE_LOG(LogTemp, Warning, TEXT("%s"), *Prop->GetName());
	}


	// Get Property defined in BluePrint
	static FName PropName("VolumnSize");
	UProperty* VolumnSizeProp = Class->FindPropertyByName(PropName);
	if (VolumnSizeProp)
	{
		float* Value = VolumnSizeProp->ContainerPtrToValuePtr<float>(this);
		UE_LOG(LogTemp, Warning, TEXT("VolumnSizeProp: %s = %f"), *VolumnSizeProp->GetName(), *Value);
	}
}

void AFloatingActor::CalledFromCppNative_Implementation()
{
	UE_LOG(LogTemp, Warning, TEXT("%s"), TEXT(__FUNCTION__));
}

void AFloatingActor::CreateDoomedObject()
{
	SafeObject = NewObject<UMyGCType>();
	DoomedObject = NewObject<UMyGCType>();
}