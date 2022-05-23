// Fill out your copyright notice in the Description page of Project Settings.


#include "MyActor.h"


// Sets default values
AMyActor::AMyActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	TotalDamage = 200;
	DamageTimeInSeconds = 1.0f;

	// Create StaticMesh Compoment
	BodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(RootComponent);

	// Construction Time Reference - FObjectFinder加载非蓝图类资源
	static ConstructorHelpers::FObjectFinder<UStaticMesh> BodySM(
		TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube'"));
	if (BodySM.Succeeded())
	{
		BodyMesh->SetStaticMesh(BodySM.Object);
	}

	// Construction Time Reference - FClassFinder加载蓝图资源
	static ConstructorHelpers::FClassFinder<AActor> MyActorClass(TEXT("Blueprint'/Game/HelloUE/BP_Axis'"));
	if (MyActorClass.Succeeded())
	{
		ClassRef = MyActorClass.Class;
		ActorClassRef = MyActorClass.Class;
	}
}

// Called when the game starts or when spawned
void AMyActor::BeginPlay()
{
	Super::BeginPlay();

	CalculateDPS();
	UE_LOG(LogTemp, Display, TEXT("创建一个AMyActor, TotalDamge=%d, DPS=%f"), TotalDamage, DamagePerSecond);
	UE_LOG(LogTemp, Display, TEXT("创建一个AMyActor, Name=%s"), *MyName.ToString());

	if (ActorClassRef)
	{
		UE_LOG(LogTemp, Display, TEXT("创建一个AMyActor, ActorClassRef=%s"), *ActorClassRef->GetName());
	}
}

void AMyActor::CalculateDPS()
{
	DamagePerSecond = TotalDamage / DamageTimeInSeconds;
}

UTexture2D* AMyActor::GetLazyLoadTexture2D()
{
	if (SoftObjectPtr_Texture2D.IsPending())
	{
		return SoftObjectPtr_Texture2D.LoadSynchronous();
	}

	return SoftObjectPtr_Texture2D.Get();
}

void AMyActor::PostInitProperties()
{
	Super::PostInitProperties();
	CalculateDPS();
}

void AMyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	CalculateDPS();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AMyActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UE_LOG(LogTemp, Display, TEXT("删除一个AMyActor, TotalDamge=%d, DPS=%f"), TotalDamage, DamagePerSecond);
}

// Called every frame
void AMyActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
