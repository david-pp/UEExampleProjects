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
	case 6:
		Snippets_Container();
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

void Snippet_Array()
{
	// Creating and Filling an Array
	TArray<FString> StrArr;
	{
		TArray<int32> IntArray;

		IntArray.Init(10, 5);
		// IntArray == [10,10,10,10,10]

		StrArr.Add(TEXT("Hello"));
		StrArr.Emplace(TEXT("World"));
		// StrArr == ["Hello","World"]

		FString Arr[] = {TEXT("of"), TEXT("Tomorrow")};
		StrArr.Append(Arr, UE_ARRAY_COUNT(Arr));
		// StrArr == ["Hello","World","of","Tomorrow"]

		StrArr.AddUnique(TEXT("!"));
		// StrArr == ["Hello","World","of","Tomorrow","!"]

		StrArr.AddUnique(TEXT("!"));
		// StrArr is unchanged as "!" is already an element

		StrArr.Insert(TEXT("Brave"), 1);
		// StrArr == ["Hello","Brave","World","of","Tomorrow","!"]

		StrArr.SetNum(8);
		// StrArr == ["Hello","Brave","World","of","Tomorrow","!","",""]

		StrArr.SetNum(6);
		// StrArr == ["Hello","Brave","World","of","Tomorrow","!"]
	}

	// Iteration
	{
		FString JoinedStr;
		// Example 1
		for (auto& Str : StrArr)
		{
			JoinedStr += Str;
			JoinedStr += " ";
		}
		UE_LOG(LogTemp, Warning, TEXT("Snippet_TArray:%s"), *JoinedStr);

		// Example 2
		JoinedStr = "";
		for (int32 Idx = 0; Idx < StrArr.Num(); ++Idx)
		{
			JoinedStr += StrArr[Idx];
			JoinedStr += " ";
		}
		UE_LOG(LogTemp, Warning, TEXT("Snippet_TArray:%s"), *JoinedStr);


		// Example 3
		JoinedStr = "";
		for (auto It = StrArr.CreateConstIterator(); It; ++It)
		{
			JoinedStr += *It;
			JoinedStr += " ";
		}
		UE_LOG(LogTemp, Warning, TEXT("Snippet_TArray:%s"), *JoinedStr);

		// Example 4
		JoinedStr = "";
		for (TArray<FString>::TConstIterator It(StrArr); It; ++It)
		{
			JoinedStr += *It;
			JoinedStr += " ";
		}
		UE_LOG(LogTemp, Warning, TEXT("Snippet_TArray:%s"), *JoinedStr);
	}

	// Sorting
	{
		// by quick sort
		StrArr.Sort();
		// StrArr == ["!","Brave","Hello","of","Tomorrow","World"]

		StrArr.Sort([](const FString& A, const FString& B)
		{
			return A.Len() < B.Len();
		});
		// StrArr == ["!","of","Hello","Brave","World","Tomorrow"]

		// by heap sort
		StrArr.HeapSort([](const FString& A, const FString& B)
		{
			return A.Len() < B.Len();
		});
		// StrArr == ["!","of","Hello","Brave","World","Tomorrow"]

		// by merge sort
		StrArr.StableSort([](const FString& A, const FString& B)
		{
			return A.Len() < B.Len();
		});
		// StrArr == ["!","of","Brave","Hello","World","Tomorrow"]
	}
}

void Snippet_Map()
{
	TMap<int32, FString> FruitMap;

	// Creating and Filling a Map
	{
		FruitMap.Add(5, TEXT("Banana"));
		FruitMap.Add(2, TEXT("Grapefruit"));
		FruitMap.Add(7, TEXT("Pineapple"));
	}

	// Iteration
	{
		for (auto& Elem : FruitMap)
		{
			FPlatformMisc::LocalPrint(*FString::Printf(TEXT("%d - %s"), Elem.Key, *Elem.Value));
		}
	}

	// Query
	{
		int32 Count = FruitMap.Num();
		// Count == 6

		bool bHas7 = FruitMap.Contains(7);
		bool bHas8 = FruitMap.Contains(8);
		// bHas7 == true
		// bHas8 == false
		//

		FString* Ptr7 = FruitMap.Find(7);
		FString* Ptr8 = FruitMap.Find(8);
		// *Ptr7 == "Pineapple"
		//  Ptr8 == nullptr
		//

		const int32* KeyMangoPtr = FruitMap.FindKey(TEXT("Mango"));
		const int32* KeyKumquatPtr = FruitMap.FindKey(TEXT("Kumquat"));
		// *KeyMangoPtr   == 5
		//  KeyKumquatPtr == nullptr
	}

	// Sorting
	{
		FruitMap.KeySort([](int32 A, int32 B)
		{
			return A > B; // sort keys in reverse
		});
		for (auto It = FruitMap.CreateConstIterator(); It; ++It)
		{
			UE_LOG(LogTemp, Warning, TEXT("%d - %s"), It->Key, *It->Value);
		}
	}
}

void Snippet_String()
{
	// FString
	{
		FString TestHUDString = TEXT("This is my test FString.");

		// Debugging message
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Blue, TestHUDString);

		// Logging
		UE_LOG(LogClass, Log, TEXT("This is a testing statement. %s"), *TestHUDString);

		CHAR* StringANSI = TCHAR_TO_ANSI(TEXT("hello"));
		TCHAR* StringTCHAR = ANSI_TO_TCHAR("hello");
		
	}
	
	// FName
	{
		FName TestHUDName = FName(TEXT("ThisIsMyTestFName"));

		// FName -> FString/FText
		FString TestHUDString = TestHUDName.ToString();
		FText TestHUDText = FText::FromName(TestHUDName);

		// Compare
		if (TestHUDName == "ThisIsMyTestFName" || TestHUDName.Compare(FName("ThisIsMyTestFName")))
		{
		}

		if (FName(TEXT("ThisIsMyTestFName"), FNAME_Find) != NAME_None)
		{
			// Do something
		}
	}

	// FText
	{
#define LOCTEXT_NAMESPACE "HelloNameSpace"

		FText HelloWorldText = LOCTEXT("HelloWorldText", "Hello World !");
		FText GoodbyeWorldText = NSLOCTEXT("OtherNamespace", "GoodbyteWorldText", "Goodbyte World !");

		uint32 CurrentHealth = 666;
		FText ShowMe = FText::Format(LOCTEXT("ExampleFText", "You currently have {0} health left."), CurrentHealth);
		UE_LOG(LogTemp, Warning, TEXT("%s"), *ShowMe.ToString());
		
#undef LOCTEXT_NAMESPACE 
	}
}

class FMyObjectType //: public TSharedFromThis<FMyObjectType, ESPMode::ThreadSafe>
{
public:
	FMyObjectType()
	{
		UE_LOG(LogTemp, Warning, TEXT("FMyObjectType-Contruct:%p"), this);
	}

	~FMyObjectType()
	{
		UE_LOG(LogTemp, Warning, TEXT("FMyObjectType-Destroyed:%p"), this);
	}
};

void Snippet_SmartPointer()
{
	// SharedPTr
	{
		// Create an empty shared pointer
		TSharedPtr<FMyObjectType> EmptyPointer;

		// Create a shared pointer to a new object
		TSharedPtr<FMyObjectType> NewPointer(new FMyObjectType());

		// Create a Shared Pointer from a Shared Reference
		TSharedRef<FMyObjectType> NewReference(new FMyObjectType());
		TSharedPtr<FMyObjectType> PointerFromReference = NewReference;

		// Create a Thread-safe Shared Pointer
		TSharedPtr<FMyObjectType, ESPMode::ThreadSafe> NewThreadsafePointer = MakeShared<FMyObjectType, ESPMode::ThreadSafe>();
	}
}

void AFloatingActor::Snippets_Container()
{
	Snippet_Array();
	Snippet_Map();
	Snippet_String();
	Snippet_SmartPointer();
}
