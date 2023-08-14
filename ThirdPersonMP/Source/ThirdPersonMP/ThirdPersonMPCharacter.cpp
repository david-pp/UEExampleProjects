// Copyright Epic Games, Inc. All Rights Reserved.

#include "ThirdPersonMPCharacter.h"

#include "CharacterBBComponent.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/Engine.h"
#include "ThirdPersonMPProjectile.h"
#include "UObject/ReflectedTypeAccessors.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"

//////////////////////////////////////////////////////////////////////////
// AThirdPersonMPCharacter

AThirdPersonMPCharacter::AThirdPersonMPCharacter(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;

	// Create a camera boom (pulls in towards the player if there is a collision)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	// Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)

	//Initialize the player's Health
	MaxHealth = 100.0f;
	CurrentHealth = MaxHealth;


	//Initialize projectile class
	ProjectileClass = AThirdPersonMPProjectile::StaticClass();
	//Initialize fire rate
	FireRate = 0.25f;
	bIsFiringWeapon = false;

	// BTComponent = CreateDefaultSubobject<UCharacterBTComponent>(TEXT("BTComponent"));
	// BTComponent->RegisterComponent();

	// Blackboard = CreateDefaultSubobject<UCharacterBBComponent>(TEXT("Blackboard"));
	// Blackboard->RegisterComponent();

	ExampleArray.OwnerCharacter = this;
	
	// Static Components
	MyStaticComponents.Reset();
	for (int i = 0; i < MyStaticComponentsNum; ++ i)
	{
		FString ComponentName = FString::Printf(TEXT("MyStaticComponent_%d"), i);
		UMyComponent* MyComponent = CreateDefaultSubobject<UMyComponent>(FName(ComponentName));
		if (MyComponent)
		{
			MyComponent->SetIsReplicated(true);

			MyStaticComponents.Add(MyComponent);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input

void AThirdPersonMPCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AThirdPersonMPCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AThirdPersonMPCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("TurnRate", this, &AThirdPersonMPCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AThirdPersonMPCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AThirdPersonMPCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AThirdPersonMPCharacter::TouchStopped);

	// VR headset functionality
	PlayerInputComponent->BindAction("ResetVR", IE_Pressed, this, &AThirdPersonMPCharacter::OnResetVR);

	// Handle firing projectiles
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &AThirdPersonMPCharacter::StartFire);
}


void AThirdPersonMPCharacter::OnResetVR()
{
	// If ThirdPersonMP is added to a project via 'Add Feature' in the Unreal Editor the dependency on HeadMountedDisplay in ThirdPersonMP.Build.cs is not automatically propagated
	// and a linker error will result.
	// You will need to either:
	//		Add "HeadMountedDisplay" to [YourProject].Build.cs PublicDependencyModuleNames in order to build successfully (appropriate if supporting VR).
	// or:
	//		Comment or delete the call to ResetOrientationAndPosition below (appropriate if not supporting VR)
	UHeadMountedDisplayFunctionLibrary::ResetOrientationAndPosition();
}

void AThirdPersonMPCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
	Jump();
}

void AThirdPersonMPCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
	StopJumping();
}

void AThirdPersonMPCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonMPCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}

void AThirdPersonMPCharacter::MoveForward(float Value)
{
	// UE_LOG(LogTemp, Log, TEXT("Move:%f"), Value);
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonMPCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}

void AThirdPersonMPCharacter::OnHealthUpdate()
{
	//Client-specific functionality
	if (IsLocallyControlled())
	{
		FString healthMessage = FString::Printf(TEXT("You now have %f health remaining."), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);

		if (CurrentHealth <= 0)
		{
			FString deathMessage = FString::Printf(TEXT("You have been killed."));
			GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, deathMessage);
		}
	}

	//Server-specific functionality
	if (GetLocalRole() == ROLE_Authority)
	{
		FString healthMessage = FString::Printf(TEXT("%s now has %f health remaining."), *GetFName().ToString(), CurrentHealth);
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, healthMessage);
	}

	//Functions that occur on all machines. 
	/*
		Any special functionality that should occur as a result of damage or death should be placed here.
	*/
}

void AThirdPersonMPCharacter::OnRep_CurrentHealth()
{
	OnHealthUpdate();
}


//////////////////////////////////////////////////////////////////////////
// Replicated Properties

void AThirdPersonMPCharacter::BeginPlay()
{
	Super::BeginPlay();

	AddDefaultExampleItems();

	if (GetLocalRole() == ROLE_Authority)
	{
		MyDynamicComponents.Reset();
		for (int i = 0; i < MyDynamicComponentsNum; ++ i)
		{
			FName ComponentName = FName(FString::Printf(TEXT("MyDynamicComponent_%d"), i));
			UMyComponent* MyComponent = NewObject<UMyComponent>(this, FName(ComponentName));
			if (MyComponent)
			{
				MyComponent->ComponentTags.Add(ComponentName);
				MyComponent->RegisterComponent();
				MyComponent->SetIsReplicated(true);
				MyDynamicComponents.Add(MyComponent);
			}
		}
	}
}

void AThirdPersonMPCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//Replicate current health.
	DOREPLIFETIME(AThirdPersonMPCharacter, CurrentHealth);

	//FAR
	DOREPLIFETIME_CONDITION(AThirdPersonMPCharacter, ExampleArray, COND_ReplayOrOwner);
	// DOREPLIFETIME(AThirdPersonMPCharacter, ExampleArray);

	DOREPLIFETIME(AThirdPersonMPCharacter, MyDynamicComponents);
}


void AThirdPersonMPCharacter::SetCurrentHealth(float healthValue)
{
	if (GetLocalRole() == ROLE_Authority)
	{
		CurrentHealth = FMath::Clamp(healthValue, 0.f, MaxHealth);
		OnHealthUpdate();
	}
}

float AThirdPersonMPCharacter::TakeDamage(float DamageTaken, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	float damageApplied = CurrentHealth - DamageTaken;
	SetCurrentHealth(damageApplied);
	return damageApplied;
}


void AThirdPersonMPCharacter::StartFire()
{
	if (!bIsFiringWeapon)
	{
		bIsFiringWeapon = true;
		UWorld* World = GetWorld();
		World->GetTimerManager().SetTimer(FiringTimer, this, &AThirdPersonMPCharacter::StopFire, FireRate, false);
		HandleFire();
	}
}

void AThirdPersonMPCharacter::StopFire()
{
	bIsFiringWeapon = false;
}

void AThirdPersonMPCharacter::HandleFire_Implementation()
{
	FVector spawnLocation = GetActorLocation() + (GetControlRotation().Vector() * 100.0f) + (GetActorUpVector() * 50.0f);
	FRotator spawnRotation = GetControlRotation();

	FActorSpawnParameters spawnParameters;
	spawnParameters.Instigator = GetInstigator();
	spawnParameters.Owner = this;

	AThirdPersonMPProjectile* spawnedProjectile = GetWorld()->SpawnActor<AThirdPersonMPProjectile>(spawnLocation, spawnRotation, spawnParameters);
}


void AThirdPersonMPCharacter::OnMontageAdvanced(UBehaviorTreeComponent& BTComp, float LastRatio, float CurrentRatio)
{
	TArray<FDelegateHandle> TriggeredHandles;

	for (auto& Item : OnAnimNotifyHandles)
	{
		if (FMath::IsWithin(Item.Value.Ratio, LastRatio, CurrentRatio))
		{
			Item.Value.bIsTriggered = true;
			Item.Value.BTDelegate.Broadcast(BTComp);
			// TriggeredHandles.Add(Item.Key);
		}
	}

	for (auto& Handle : TriggeredHandles)
	{
		OnAnimNotifyHandles.Remove(Handle);
	}
}

bool AThirdPersonMPCharacter::RunBehaviorTree(UBehaviorTree* BTAsset)
{
	return RunBehaviorTreeImpl(BTAsset, EBTExecutionMode::Looped);
}

bool AThirdPersonMPCharacter::RunBehaviorTreeSingleRun(UBehaviorTree* BTAsset)
{
	return RunBehaviorTreeImpl(BTAsset, EBTExecutionMode::SingleRun);
}

void AThirdPersonMPCharacter::StopBehaviorTree()
{
	if (BTComponent)
	{
		BTComponent->StopTree();
	}
}

bool AThirdPersonMPCharacter::RunActionBehaviorTree(FName Action, UBehaviorTree* BTAsset)
{
	if (BTAsset == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("RunBehaviorTree: Unable to run NULL behavior tree"));
		return false;
	}

	bool bSuccess = false;
	// see if need a blackboard component at all
	UBlackboardComponent* BlackboardComp = Blackboard;
	if (BTAsset->BlackboardAsset && (Blackboard == nullptr || Blackboard->IsCompatibleWith(BTAsset->BlackboardAsset) == false))
	{
		bSuccess = UseBlackboard(BTAsset->BlackboardAsset, BlackboardComp);
		UE_LOG(LogTemp, Warning, TEXT("RunBehaviorTree: BlackboardAsset is not Compatible with Character's blackboard"));
		// return false;
	}

	UCharacterBTComponent* ActionBTC = nullptr;
	UCharacterBTComponent** ActionBTCPtr = ActionBTComponents.Find(Action);
	if (ActionBTCPtr)
	{
		ActionBTC = *ActionBTCPtr;
	}

	if (ActionBTC == NULL)
	{
		UE_LOG(LogTemp, Log, TEXT("RunBehaviorTree: spawning Action BehaviorTreeComponent.."));

		FName BTCName(FString::Printf(TEXT("%sBTC"), *Action.ToString()));

		ActionBTC = NewObject<UCharacterBTComponent>(this, BTCName);
		ActionBTC->RegisterComponent();
		ActionBTComponents.Add(Action, ActionBTC);
	}

	check(ActionBTC != NULL);
	ActionBTC->StartTree(*BTAsset, EBTExecutionMode::Looped);

	return true;
}

void AThirdPersonMPCharacter::StopActionBehaviorTree(FName Action)
{
	UCharacterBTComponent* ActionBTC = nullptr;
	UCharacterBTComponent** ActionBTCPtr = ActionBTComponents.Find(Action);
	if (ActionBTCPtr)
	{
		ActionBTC = *ActionBTCPtr;
		if (ActionBTC)
		{
			ActionBTC->StopTree();
		}
	}
}

void AThirdPersonMPCharacter::RunActionBehaviorTreeOnServer_Implementation(FName Action, UBehaviorTree* BTAsset)
{
	RunActionBehaviorTree(Action, BTAsset);
}

void AThirdPersonMPCharacter::RunBehaviorTreeEx(UBehaviorTree* BTAsset, bool Autonoumous, bool Authority, bool Simulated)
{
	if (IsLocallyControlled())
	{
		if (Autonoumous)
		{
			RunBehaviorTree(BTAsset);
		}

		if (Authority || Simulated)
		{
			RunBehaviorTreeOnServer(BTAsset, Autonoumous, Authority, Simulated);
		}
	}
}

void AThirdPersonMPCharacter::RunBehaviorTreeOnServer_Implementation(UBehaviorTree* BTAsset, bool Autonoumous, bool Authority, bool Simulated)
{
	if (Authority)
	{
		RunBehaviorTree(BTAsset);
	}

	if (Simulated)
	{
		RunBehaviorTreeOnClients(BTAsset, Autonoumous, Authority, Simulated);
	}
}

void AThirdPersonMPCharacter::RunBehaviorTreeOnClients_Implementation(UBehaviorTree* BTAsset, bool Autonoumous, bool Authority, bool Simulated)
{
	if (Simulated && GetLocalRole() == ENetRole::ROLE_SimulatedProxy)
	{
		RunBehaviorTree(BTAsset);
	}
}

bool AThirdPersonMPCharacter::RunBehaviorTreeImpl(UBehaviorTree* BTAsset, EBTExecutionMode::Type ExecuteMode/*= EBTExecutionMode::Looped*/)
{
	if (BTAsset == NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("RunBehaviorTree: Unable to run NULL behavior tree"));
		return false;
	}

	bool bSuccess = true;

	// see if need a blackboard component at all
	UBlackboardComponent* BlackboardComp = Blackboard;
	if (BTAsset->BlackboardAsset && (Blackboard == nullptr || Blackboard->IsCompatibleWith(BTAsset->BlackboardAsset) == false))
	{
		bSuccess = UseBlackboard(BTAsset->BlackboardAsset, BlackboardComp);
	}

	if (bSuccess)
	{
		if (BTComponent == NULL)
		{
			UE_LOG(LogTemp, Log, TEXT("RunBehaviorTree: spawning BehaviorTreeComponent.."));

			BTComponent = NewObject<UCharacterBTComponent>(this, TEXT("BTComponent"));
			BTComponent->RegisterComponent();
		}

		check(BTComponent != NULL);
		BTComponent->StartTree(*BTAsset, ExecuteMode);
	}

	return bSuccess;
}

bool AThirdPersonMPCharacter::UseBlackboard(UBlackboardData* BlackboardAsset, UBlackboardComponent*& BlackboardComponent)
{
	if (BlackboardAsset == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("UseBlackboard: trying to use NULL Blackboard asset. Ignoring"));
		return false;
	}

	bool bSuccess = true;
	Blackboard = FindComponentByClass<UCharacterBBComponent>();

	if (Blackboard == nullptr)
	{
		Blackboard = NewObject<UCharacterBBComponent>(this, TEXT("BlackboardComponent"));
		if (Blackboard != nullptr)
		{
			InitializeBlackboard(*Blackboard, *BlackboardAsset);
			Blackboard->RegisterComponent();
		}
	}
	else if (Blackboard->GetBlackboardAsset() == nullptr)
	{
		InitializeBlackboard(*Blackboard, *BlackboardAsset);
	}
	else if (Blackboard->GetBlackboardAsset() != BlackboardAsset)
	{
		UE_LOG(LogTemp, Log, TEXT("UseBlackboard: requested blackboard %s while already has %s instantiated. Forcing new BB."), *GetNameSafe(BlackboardAsset), *GetNameSafe(Blackboard->GetBlackboardAsset()));
		InitializeBlackboard(*Blackboard, *BlackboardAsset);
	}

	BlackboardComponent = Blackboard;

	return bSuccess;
}

bool AThirdPersonMPCharacter::InitializeBlackboard(UBlackboardComponent& BlackboardComp, UBlackboardData& BlackboardAsset)
{
	check(BlackboardComp.GetOwner() == this);

	if (BlackboardComp.InitializeBlackboard(BlackboardAsset))
	{
		// find the "self" key and set it to our pawn
		const FBlackboard::FKey SelfKey = BlackboardAsset.GetKeyID(FBlackboard::KeySelf);
		if (SelfKey != FBlackboard::InvalidKey)
		{
			BlackboardComp.SetValue<UBlackboardKeyType_Object>(SelfKey, this);
		}

		OnUsingBlackBoard(&BlackboardComp, &BlackboardAsset);
		return true;
	}
	return false;
}

void AThirdPersonMPCharacter::OnReq_ExampleArray()
{
	DumpExampleArray(TEXT("OnReq_ExampleArray"));
}

void AThirdPersonMPCharacter::AddDefaultExampleItems()
{
	if (GetLocalRole() == ROLE_Authority)
	{
		for (int i = 1; i < 10; ++i)
		{
			FExampleItemEntry& AddItem = ExampleArray.Items.AddDefaulted_GetRef();
			AddItem.ExampleIntProperty = i * 100;
			AddItem.ExampleFloatProperty = 3.14;
			AddItem.ExampleString = TEXT("David") + FString::FromInt(i);
			ExampleArray.MarkItemDirty(AddItem);
		}
	}
}

FString AThirdPersonMPCharacter::GetLocalRoleString() const
{
	return StaticEnum<ENetRole>()->GetNameByValue((int64)GetLocalRole()).ToString();
}

void AThirdPersonMPCharacter::DumpExampleArray(const FString& Title)
{
	UE_LOG(LogTemp, Warning, TEXT("---%s-- Items(%d)@%s - %s--------"), *Title, ExampleArray.Items.Num(), *GetLocalRoleString(), *GetName());
	for (auto& Item : ExampleArray.Items)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item: %s, %d, %.2f"), *Item.ExampleString, Item.ExampleIntProperty, Item.ExampleFloatProperty);
	}
}

void AThirdPersonMPCharacter::OnRep_MyDynamicComponents()
{
}

void AThirdPersonMPCharacter::ModifyMyDynamicComponents_Implementation()
{
	for (auto MyComponent : MyDynamicComponents)
	{
		MyComponent->MyName = TEXT("Dynamic-David");
	}
}

void AThirdPersonMPCharacter::RequestDumpMyComponents_Implementation()
{
	DoDumpMyComponents();
}

void AThirdPersonMPCharacter::DoDumpMyComponents_Implementation()
{
#if 0
	TArray<UMyComponent*> MyComponents;
	GetComponents<UMyComponent>(MyComponents);
	for (auto MyComponent : MyComponents)
	{
		UE_LOG(LogTemp, Error, TEXT("MyComponent@%s : %s"), *GetLocalRoleString(), *MyComponent->MyName);
	}
#else
	for (auto MyComponent : MyStaticComponents)
	{
		UE_LOG(LogTemp, Error, TEXT("MyComponent@%s : %s"), *GetLocalRoleString(), *MyComponent->MyName);
	}

	for (auto MyComponent : MyDynamicComponents)
	{
		UE_LOG(LogTemp, Error, TEXT("MyComponent@%s : %s"), *GetLocalRoleString(), *MyComponent->MyName);
	}
#endif
}

void AThirdPersonMPCharacter::ModifyMyStaticComponents_Implementation()
{
	// Method1
	for (auto MyComponent : MyStaticComponents)
	{
		MyComponent->MyName = TEXT("Static-David");
	}

	// Method2
	// TArray<UMyComponent*> MyComponents;
	// GetComponents<UMyComponent>(MyComponents);
	// for (auto MyComponent : MyComponents)
	// {
	// 	MyComponent->MyName = TEXT("David");
	// }
}

void AThirdPersonMPCharacter::DoDumpExampleArray_Implementation()
{
	DumpExampleArray();
}

void AThirdPersonMPCharacter::RequestDumpExampleArray_Implementation()
{
	DoDumpExampleArray();
}

void AThirdPersonMPCharacter::SetItemString_Implementation(int ItemInt, const FString& ItemString)
{
	FExampleItemEntry* Item = ExampleArray.Items.FindByPredicate([ItemInt](FExampleItemEntry& ItemEntry)-> bool
	{
		return ItemEntry.ExampleIntProperty == ItemInt;
	});

	if (Item)
	{
		Item->ExampleString = ItemString;
		ExampleArray.MarkItemDirty(*Item);
	}
}

void AThirdPersonMPCharacter::RemoveItem_Implementation(int ItemInt)
{
	ExampleArray.Items.RemoveAll([ItemInt](FExampleItemEntry& ItemEntry)
	{
		return ItemEntry.ExampleIntProperty == ItemInt;
	});

	ExampleArray.MarkArrayDirty();
}

void AThirdPersonMPCharacter::AddItem_Implementation(const FExampleItemEntry& Item)
{
	FExampleItemEntry& AddItem = ExampleArray.Items.Add_GetRef(Item);
	ExampleArray.MarkItemDirty(AddItem);
}
