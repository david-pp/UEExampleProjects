#include "GameStorage.h"
#include "GameStorageEngine.h"
#include "GameStoragePath.h"
#include "GameStorageTestUser.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_Path, "GameStorage.Path", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_Path::RunTest(const FString& Param)
{
	auto StorageEngine = IGameStorageEngine::GetDefault();
	if (!StorageEngine) return false;

	{
		FGameStoragePath EntityPath(TEXT("user:01"));

		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}
	{
		FGameStoragePath EntityPath(TEXT("user:01/profile"));

		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	{
		FGameStoragePath EntityPath(TEXT("user:01/character:001"));
		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	{
		FGameStoragePath EntityPath(TEXT("user:01/character:002"));
		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	{
		FGameStoragePath EntityPath(StorageEngine->GetNamespace());
		EntityPath.AppendPath(TEXT("config/game_title"));

		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_ObjectBasic, "GameStorage.ObjectBasic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_ObjectBasic::RunTest(const FString& Param)
{
	auto StorageEngine = IGameStorageEngine::GetDefault();
	if (StorageEngine)
	{
		// Save
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Save ------ "));
		{
			// 1
			auto User = NewObject<UGameStorageTestUser>();
			User->UserName = TEXT("david");
			User->UserAge = 100;
			User->UserSex = EGameStorageTestUserSexType::Male;
			StorageEngine->SaveObject(User, FString::Printf(TEXT("user:%s"), *User->UserName));

			// 2
			User->UserName = TEXT("jessie");
			User->UserSex = EGameStorageTestUserSexType::Female;
			StorageEngine->SaveObject(User, FString::Printf(TEXT("user:%s"), *User->UserName));

			// 3 (delete)
			User->UserName = TEXT("xxxx");
			StorageEngine->SaveObject(User, FString::Printf(TEXT("user:%s"), *User->UserName));
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Load ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			StorageEngine->LoadObject(User, TEXT("user:david"));

			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Name=%s"), *User->UserName);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Age=%d"), User->UserAge);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Sex=%s"), LexToString(User->UserSex));
		}

		// Load and create
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Load and create ------ "));
		{
			auto User = StorageEngine->LoadNewObject<UGameStorageTestUser>(TEXT("user:jessie"));
			if (User)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load2 : User.Name=%s"), *User->UserName);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load2 : User.Age=%d"), User->UserAge);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load2 : User.Sex=%s"), LexToString(User->UserSex));
			}
		}

		// Delete
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Delete  ------ "));
		{
			StorageEngine->DeleteObject(TEXT("user:xxxx"));
		}

		// Load all
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- LoadAll ------ "));
		{
			TArray<UGameStorageTestUser*> Users;
			StorageEngine->LoadObjects<UGameStorageTestUser>(Users, TEXT("user:*"));

			for (auto User : Users)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load3 : User.Name=%s"), *User->UserName);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load3 : User.Age=%d"), User->UserAge);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load3 : User.Sex=%s"), LexToString(User->UserSex));
			}
		}
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_ObjectAdvanced, "GameStorage.ObjectAdvanced", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_ObjectAdvanced::RunTest(const FString& Param)
{
	auto StorageEngine = IGameStorageEngine::GetDefault();
	if (StorageEngine)
	{
		// Save
		UE_LOG(LogGameStorage, Log, TEXT("--------- Save ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			User->UserName = TEXT("david2");
			User->UserAge = 100;
			User->UserSex = EGameStorageTestUserSexType::Male;

			// Profile
			User->Profile = NewObject<UGameStorageTestUserProfile>(User);
			User->Profile->Email = TEXT("david@gmail.com");
			User->Profile->Phone = TEXT("182222222");

			// Item1
			{
				auto Item = NewObject<UGameStorageTestUserItem>(User);
				Item->ItemName = TEXT("sword");
				Item->ItemCount = 1;
				User->Items.Add(Item);
			}
			{
				auto Item = NewObject<UGameStorageTestUserItem>(User);
				Item->ItemName = TEXT("potion");
				Item->ItemCount = 222;
				User->Items.Add(Item);
			}

			StorageEngine->SaveObject(User, FString::Printf(TEXT("player:%s"), *User->UserName));
			StorageEngine->SaveObject(User->Profile, FString::Printf(TEXT("player:%s/profile"), *User->UserName));
			for (auto Item : User->Items)
			{
				StorageEngine->SaveObject(Item, FString::Printf(TEXT("player:%s/item:%s"), *User->UserName, *Item->ItemName));
			}
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("--------- Load Profile & Items ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			User->Profile = StorageEngine->LoadNewObject<UGameStorageTestUserProfile>(TEXT("player:david2/profile"));
			StorageEngine->LoadObjects(User->Items, TEXT("player:david2/item:*"));
			if (User->Profile)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load : User.Profile=%s,%s"), *User->Profile->Email, *User->Profile->Phone);
			}

			for (auto Item : User->Items)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load : User.Item=%s,%d"), *Item->ItemName, Item->ItemCount);
			}
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("--------- Load Subobjects------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			StorageEngine->LoadObject(User, TEXT("player:david2"));

			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load : User.Name=%s"), *User->UserName);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load : User.Age=%d"), User->UserAge);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load : User.Sex=%s"), LexToString(User->UserSex));
			if (User->Profile)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load : User.Profile=%s,%s"), *User->Profile->Email, *User->Profile->Phone);
			}
			for (auto Item : User->Items)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load : User.Item=%s,%d"), *Item->ItemName, Item->ItemCount);
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_ObjectAsync, "GameStorage.ObjectAsync", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_ObjectAsync::RunTest(const FString& Param)
{
	FGameStorageEngineSettings Settings;
	Settings.Namespace = TEXT("async");
	Settings.SerializerType = EGameStorageSerializerType::Sav;

	Settings.bEnableRedisBackend = true;
	Settings.RedisAddress = TEXT("127.0.0.1");
	Settings.RedisPort = 6379;
	Settings.RedisPassword = TEXT("");
	Settings.bSaveEntityAsHash = false;
	auto StorageEngine = FGameStorageModule::Get()->CreateStorageEngine(Settings);

	if (StorageEngine)
	{
		// Save
		UE_LOG(LogGameStorage, Log, TEXT("Async --------- Save ------ "));
		{
			// 1
			auto User = NewObject<UGameStorageTestUser>();
			User->UserName = TEXT("david");
			User->UserAge = 100;
			User->UserSex = EGameStorageTestUserSexType::Male;
			StorageEngine->AsyncSaveObject(User, FString::Printf(TEXT("user:%s"), *User->UserName));

			// 2
			User->UserName = TEXT("jessie");
			User->UserSex = EGameStorageTestUserSexType::Female;
			StorageEngine->AsyncSaveObject(User, FString::Printf(TEXT("user:%s"), *User->UserName));

			// 3 (delete)
			User->UserName = TEXT("xxxx");
			StorageEngine->AsyncSaveObject(User, FString::Printf(TEXT("user:%s"), *User->UserName));


			FPlatformProcess::Sleep(0.1);
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("Async --------- Load ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			User->AddToRoot();
			StorageEngine->AsyncLoadObject(User, TEXT("user:david"), FOnStorageObjectLoadDelegate::CreateLambda([User](UObject* Object, const FString& ErrorMsg)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Async - Load : User.Name=%s"), *User->UserName);
				UE_LOG(LogGameStorage, Log, TEXT("Async - Load : User.Age=%d"), User->UserAge);
				UE_LOG(LogGameStorage, Log, TEXT("Async - Load : User.Sex=%s"), LexToString(User->UserSex));
				User->RemoveFromRoot();
			}));
		}

		// Load and create
		UE_LOG(LogGameStorage, Log, TEXT("Async --------- Load and create ------ "));
		{
			StorageEngine->AsyncLoadNewObject(TEXT("user:jessie"), UGameStorageTestUser::StaticClass(), GetTransientPackage(), FOnStorageObjectLoadDelegate::CreateLambda([](UObject* Object, const FString& ErrorMsg)
			{
				UGameStorageTestUser* User = Cast<UGameStorageTestUser>(Object);
				if (User)
				{
					UE_LOG(LogGameStorage, Log, TEXT("Async - LoadNew : User.Name=%s"), *User->UserName);
					UE_LOG(LogGameStorage, Log, TEXT("Async - LoadNew : User.Age=%d"), User->UserAge);
					UE_LOG(LogGameStorage, Log, TEXT("Async - LoadNew : User.Sex=%s"), LexToString(User->UserSex));
				}
			}));
		}

		// Delete
		UE_LOG(LogGameStorage, Log, TEXT("Async --------- Delete  ------ "));
		{
			StorageEngine->AsyncDeleteObject(TEXT("user:xxxx"));
		}

		// Load all
		UE_LOG(LogGameStorage, Log, TEXT("Async --------- LoadAll ------ "));
		{
			TArray<UGameStorageTestUser*> Users;
			FOnStorageObjectsLoadDelegate OnLoaded;
			OnLoaded.BindLambda([](const TArray<UObject*>& Objects, const FString& ErrorMsg)
			{
				for (auto Object : Objects)
				{
					UGameStorageTestUser* User = Cast<UGameStorageTestUser>(Object);
					if (User)
					{
						UE_LOG(LogGameStorage, Log, TEXT("Async - LoadAll : User.Name=%s"), *User->UserName);
						UE_LOG(LogGameStorage, Log, TEXT("Async - LoadAll : User.Age=%d"), User->UserAge);
						UE_LOG(LogGameStorage, Log, TEXT("Async - LoadAll : User.Sex=%s"), LexToString(User->UserSex));
					}
				}
			});

			StorageEngine->AsyncLoadObjects(UGameStorageTestUser::StaticClass(), TEXT("user:*"), GetTransientPackage(), OnLoaded);
		}

		FPlatformProcess::Sleep(3);
	}

	return true;
}
