#include "GameStorage.h"
#include "GameStorageEngine.h"
#include "GameStorageTestUser.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_EntityPath, "GameStorage.EntityPath", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_EntityPath::RunTest(const FString& Param)
{
	auto StorageEngine = IGameStorageEngine::GetDefault();
	if (!StorageEngine) return false;

	{
		FGameEntityStoragePath EntityPath(TEXT("user:01"));

		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}
	{
		FGameEntityStoragePath EntityPath(TEXT("user:01/profile"));

		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	{
		FGameEntityStoragePath EntityPath(TEXT("user:01/character:001"));
		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	{
		FGameEntityStoragePath EntityPath(TEXT("user:01/character:002"));
		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	{
		FGameEntityStoragePath EntityPath(StorageEngine->GetNamespace());
		EntityPath.AppendPath(TEXT("config/game_title"));

		UE_LOG(LogGameStorage, Log, TEXT("Path:  %s"), *EntityPath.GetPath());
		UE_LOG(LogGameStorage, Log, TEXT("Redis: %s"), *EntityPath.ToRedisKey());
		UE_LOG(LogGameStorage, Log, TEXT("File:  %s"), *EntityPath.ToFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("File2: %s"), *EntityPath.ToFlatFilePath());
		UE_LOG(LogGameStorage, Log, TEXT("REST:  %s"), *EntityPath.ToRESTFulPath());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_EntityBasic, "GameStorage.EntityBasic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_EntityBasic::RunTest(const FString& Param)
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
			StorageEngine->SaveEntity(User, FString::Printf(TEXT("user:%s"), *User->UserName));

			// 2
			User->UserName = TEXT("jessie");
			User->UserSex = EGameStorageTestUserSexType::Female;
			StorageEngine->SaveEntity(User, FString::Printf(TEXT("user:%s"), *User->UserName));

			// 3 (delete)
			User->UserName = TEXT("xxxx");
			StorageEngine->SaveEntity(User, FString::Printf(TEXT("user:%s"), *User->UserName));
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Load ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			StorageEngine->LoadEntity(User, TEXT("user:david"));

			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Name=%s"), *User->UserName);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Age=%d"), User->UserAge);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Sex=%s"), LexToString(User->UserSex));
		}

		// Load and create
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Load and create ------ "));
		{
			auto User = StorageEngine->LoadAndCreateEntity<UGameStorageTestUser>(TEXT("user:jessie"));
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
			StorageEngine->DeleteEntity(TEXT("user:xxxx"));
		}

		// Load all
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- LoadAll ------ "));
		{
			TArray<UGameStorageTestUser*> Users;
			StorageEngine->LoadEntities<UGameStorageTestUser>(Users, TEXT("user:*"));

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


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_EntityObjects, "GameStorage.EntityObjects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_EntityObjects::RunTest(const FString& Param)
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

			StorageEngine->SaveEntity(User, FString::Printf(TEXT("player:%s"), *User->UserName));
			StorageEngine->SaveEntity(User->Profile, FString::Printf(TEXT("player:%s/profile"), *User->UserName));
			for (auto Item : User->Items)
			{
				StorageEngine->SaveEntity(Item, FString::Printf(TEXT("player:%s/item:%s"), *User->UserName, *Item->ItemName));
			}
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("--------- Load Profile & Items ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			User->Profile = StorageEngine->LoadAndCreateEntity<UGameStorageTestUserProfile>(TEXT("player:david2/profile"));
			StorageEngine->LoadEntities(User->Items, TEXT("player:david2/item:*"));
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
			StorageEngine->LoadEntity(User, TEXT("player:david2"));

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
