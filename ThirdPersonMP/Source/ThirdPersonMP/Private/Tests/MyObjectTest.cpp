#include "CoreMinimal.h"
#include "../MyAssetManager.h"

DEFINE_LOG_CATEGORY_STATIC(TestLog, Log, All);

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyObjectTest, "MyTest.ObjectTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyObjectSaveTest, "MyTest.SaveObjectTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyObjectLoadTest, "MyTest.LoadObjectTest", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyObjectSaveTest2, "MyTest.SaveObjectTest2", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyObjectLoadTest2, "MyTest.LoadObjectTest2", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


bool FMyObjectTest::RunTest(const FString& Param)
{
	UMyObject* Object = NewObject<UMyObject>();
	Object->MyName = TEXT("David");
	Object->MyAge = 18;
	Object->MyInfo = TEXT("A Game Programmer");
	Object->MyWeight = 60.0;
	UE_LOG(TestLog, Display, TEXT("Done ......"));
	return true;
}

bool FMyObjectSaveTest::RunTest(const FString& Param)
{
	FName ObjectName = TEXT("DavidObject");
	FString PackageName = TEXT("/Game/MyObject/DavidPackage");

	// Create an empty package
	UPackage* Package = CreatePackage(nullptr, *PackageName);
	Package->FullyLoad();

	// Creat an object and add to package
	UMyObject* Object = NewObject<UMyObject>(Package, ObjectName, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	Object->MyName = TEXT("David");
	Object->MyAge = 18;
	Object->MyInfo = TEXT("A Game Programmer");
	Object->MyWeight = 60.0;

	// Save Package
	Package->MarkPackageDirty();

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	bool bSaved = UPackage::SavePackage(Package, Object, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
	// const FSavePackageResultStruct Result = UPackage::Save(Package, Object, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
	// bool bSaved = (Result == ESavePackageResult::Success);

	UE_LOG(TestLog, Display, TEXT("Done ......"));
	return bSaved;
}

bool FMyObjectLoadTest::RunTest(const FString& Param)
{
	FName ObjectName = TEXT("DavidObject");
	FString PackageName = TEXT("/Game/MyObject/DavidPackage");
	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());

	if (FPaths::FileExists(PackageFileName))
	{
		UPackage* Package = LoadPackage(nullptr, *PackageName, LOAD_None);
		Package->FullyLoad();

		for (auto& ObjExport : Package->LinkerLoad->ExportMap)
		{
			if (ObjExport.Object)
			{
				UE_LOG(TestLog, Display, TEXT("Export : %s"), *ObjExport.Object->GetName());
			}

			UMyObject* MyObject = Cast<UMyObject>(ObjExport.Object);
			if (MyObject)
			{
				UE_LOG(TestLog, Display, TEXT("Export is MyObject : %s,%d, %s,%.2f"), *MyObject->MyName, MyObject->MyAge, *MyObject->MyInfo, MyObject->MyWeight);
			}
		}

		UE_LOG(TestLog, Display, TEXT("Done ......"));
		return true;
	}

	UE_LOG(TestLog, Warning, TEXT("Not Exist ......"));
	return false;
}


bool FMyObjectSaveTest2::RunTest(const FString& Param)
{
	// Packages
	UPackage* Package1 = CreatePackage(nullptr, TEXT("/Game/MyObject/Package1"));
	Package1->FullyLoad();
	UPackage* Package2 = CreatePackage(nullptr, TEXT("/Game/MyObject/Package2"));
	Package2->FullyLoad();
	UPackage* Package3 = CreatePackage(nullptr, TEXT("/Game/MyObject/Package3"));
	Package3->FullyLoad();

	// Objects
	UMyObject* David = NewObject<UMyObject>(Package1, TEXT("David"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	David->MyName = TEXT("David");

	UMyObject* ObjectA = NewObject<UMyObject>(Package1, TEXT("ObjectA"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	ObjectA->MyName = TEXT("ObjectA");
	UMyObject* ObjectB = NewObject<UMyObject>(Package2, TEXT("ObjectB"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	ObjectB->MyName = TEXT("ObjectB");
	UMyObject* ObjectC = NewObject<UMyObject>(Package3, TEXT("ObjectC"), EObjectFlags::RF_Public | EObjectFlags::RF_Standalone);
	ObjectC->MyName = TEXT("ObjectC");

	David->HardRef_A = ObjectA;
	David->HardRef_B = ObjectB;
	David->SoftRef_C = ObjectC;


	// Save Package
	Package1->MarkPackageDirty();
	Package2->MarkPackageDirty();
	Package3->MarkPackageDirty();

	FSavePackageResultStruct Result;

	// Package1
	{
		FString PackageFileName = FPackageName::LongPackageNameToFilename(TEXT("/Game/MyObject/Package1"), FPackageName::GetAssetPackageExtension());
		Result = UPackage::Save(Package1, David, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
		if (Result.Result == ESavePackageResult::Success)
		{
		}

		Result = UPackage::Save(Package1, ObjectA, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
		if (Result.Result == ESavePackageResult::Success)
		{
		}
	}

	{
		FString PackageFileName = FPackageName::LongPackageNameToFilename(TEXT("/Game/MyObject/Package2"), FPackageName::GetAssetPackageExtension());
		Result = UPackage::Save(Package2, ObjectB, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
		if (Result.Result == ESavePackageResult::Success)
		{
		}
	}

	{
		FString PackageFileName = FPackageName::LongPackageNameToFilename(TEXT("/Game/MyObject/Package3"), FPackageName::GetAssetPackageExtension());
		Result = UPackage::Save(Package3, ObjectC, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
		if (Result.Result == ESavePackageResult::Success)
		{
		}
	}

	UE_LOG(TestLog, Display, TEXT("Done ......"));
	return true;
}

bool FMyObjectLoadTest2::RunTest(const FString& Param)
{
	UPackage* Package1 = LoadPackage(nullptr, TEXT("/Game/MyObject/Package1"), LOAD_None);
	Package1->FullyLoad();
	UMyAssetUtility::DumpLinkerLoad(Package1);

	UPackage* Package2 = LoadPackage(nullptr, TEXT("/Game/MyObject/Package2"), LOAD_None);
	Package2->FullyLoad();
	UMyAssetUtility::DumpLinkerLoad(Package2);

	UPackage* Package3 = LoadPackage(nullptr, TEXT("/Game/MyObject/Package3"), LOAD_None);
	Package1->FullyLoad();
	UMyAssetUtility::DumpLinkerLoad(Package3);

	UMyObject* David = FindObject<UMyObject>(Package1, TEXT("David"));
	if (David)
	{
		UE_LOG(TestLog, Display, TEXT("My Object : %s"), *David->MyName);

		if (David->HardRef_A)
		{
			UE_LOG(TestLog, Display, TEXT(" - HardRef_A: %s"), *David->HardRef_A->MyName);
		}

		if (David->HardRef_B)
		{
			UE_LOG(TestLog, Display, TEXT(" - HardRef_B:  %s"), *David->HardRef_B->MyName);
		}

		UMyObject* ObjectC = David->SoftRef_C.LoadSynchronous();
		if (ObjectC)
		{
			UE_LOG(TestLog, Display, TEXT(" - SoftRef_C: %s"), *ObjectC->MyName);
		}
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMyObjectTest_StealPrivate, "MyTest.StealPrivate", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

//// --

template <const FString UMyObject::* MEMBER_INT_PTR>
struct GenerateThiefFunction
{
	friend FString StealPrivateName(const UMyObject& VictimObject)
	{
		return VictimObject.*MEMBER_INT_PTR;
	}
};

template struct GenerateThiefFunction<&UMyObject::PrivateName>;
FString StealPrivateName(const UMyObject& VictimObject);


//// --
template <typename Tag>
struct result
{
	/* export it ... */
	typedef typename Tag::type type;
	static type ptr;
};

template <typename Tag>
typename result<Tag>::type result<Tag>::ptr;

template <typename Tag, typename Tag::type p>
struct rob : result<Tag>
{
	/* fill it ... */
	struct filler
	{
		filler() { result<Tag>::ptr = p; }
	};

	static filler filler_obj;
};

template <typename Tag, typename Tag::type p>
typename rob<Tag, p>::filler rob<Tag, p>::filler_obj;

struct Af
{
	typedef void (UMyObject::*type)();
};

template struct rob<Af, &UMyObject::PrivateFunction>;

//// ---

// This is a rewrite and analysis of the technique in this article:
// http://bloglitb.blogspot.com/2010/07/access-to-private-members-thats-easy.html

// ------- Framework -------
// The little library required to work this magic

// Generate a static data member of type Tag::type in which to store
// the address of a private member.  It is crucial that Tag does not
// depend on the /value/ of the the stored address in any way so that
// we can access it from ordinary code without directly touching
// private data.
template <class Tag>
struct stowed
{
	static typename Tag::type value;
};

template <class Tag>
typename Tag::type stowed<Tag>::value;

// Generate a static data member whose constructor initializes
// stowed<Tag>::value.  This type will only be named in an explicit
// instantiation, where it is legal to pass the address of a private
// member.
template <class Tag, typename Tag::type x>
struct stow_private
{
	stow_private() { stowed<Tag>::value = x; }
	static stow_private instance;
};

template <class Tag, typename Tag::type x>
stow_private<Tag, x> stow_private<Tag, x>::instance;

/// ---------------

struct MyObject_PrivateName
{
	using type = FString UMyObject::*;
};

struct MyObject_PrivateFunction
{
	using type = void (UMyObject::*)();
};

template struct stow_private<MyObject_PrivateName, &UMyObject::PrivateName>;
template struct stow_private<MyObject_PrivateFunction, &UMyObject::PrivateFunction>;

// template <typename Tag>
// typename Tag::type saved_private_v;

// template <typename Tag, typename Tag::type x>
// bool save_private_v = (saved_private_v<Tag> = x);
//
// template bool save_private_v<MyObject_PrivateName, &UMyObject::PrivateName>;
// template bool save_private_v<MyObject_PrivateFunction, &UMyObject::PrivateFunction>;


bool FMyObjectTest_StealPrivate::RunTest(const FString& Param)
{
	UMyObject* Object = NewObject<UMyObject>();
	Object->MyName = TEXT("David");
	Object->MyAge = 18;
	Object->MyInfo = TEXT("A Game Programmer");
	Object->MyWeight = 60.0;

	// Method 1
	FString PrivateName = StealPrivateName(*Object);
	UE_LOG(TestLog, Log, TEXT("PrivateName:%s"), *PrivateName);

	// Method 2
	(Object->*result<Af>::ptr)();

	// Method 3
	PrivateName = Object->*stowed<MyObject_PrivateName>::value;
	UE_LOG(TestLog, Log, TEXT("PrivateName3:%s"), *PrivateName);

	(Object->*stowed<MyObject_PrivateFunction>::value)();

	return true;
}
