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
