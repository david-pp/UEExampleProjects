#include "GameStorage.h"
#include "GameStorageEngine.h"
#include "GameStorageTestUser.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_Basic, "GameStorage.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)


bool FGameStorageTest_Basic::RunTest(const FString& Param)
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
			StorageEngine->SaveEntity(User, FGameEntityStorageKey(TEXT("user"), User->UserName));

			// 2
			User->UserName = TEXT("jessie");
			User->UserSex = EGameStorageTestUserSexType::Female;
			StorageEngine->SaveEntity(User, FGameEntityStorageKey(TEXT("user"), User->UserName));
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Load ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();
			StorageEngine->LoadEntity(User, FGameEntityStorageKey(TEXT("user"), TEXT("david")));

			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Name=%s"), *User->UserName);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Age=%d"), User->UserAge);
			UE_LOG(LogGameStorage, Log, TEXT("Basic - Load1 : User.Sex=%s"), LexToString(User->UserSex));
		}

		// Load and create
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Load and create ------ "));
		{
			auto User = StorageEngine->LoadAndCreateEntity<UGameStorageTestUser>(FGameEntityStorageKey(TEXT("user"), TEXT("jessie")));
			if (User)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load2 : User.Name=%s"), *User->UserName);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load2 : User.Age=%d"), User->UserAge);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load2 : User.Sex=%s"), LexToString(User->UserSex));
			}
		}

		// Load all
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- LoadAll ------ "));
		{
			TArray<UGameStorageTestUser*> Users;
			StorageEngine->LoadEntities<UGameStorageTestUser>(Users, TEXT("user"));

			for (auto User : Users)
			{
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load3 : User.Name=%s"), *User->UserName);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load3 : User.Age=%d"), User->UserAge);
				UE_LOG(LogGameStorage, Log, TEXT("Basic - Load3 : User.Sex=%s"), LexToString(User->UserSex));
			}
		}

		// Delete
		// UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Delete ------ "));
		// {
		// 	StorageEngine->DeleteEntity(FGameEntityStorageKey(TEXT("user"), TEXT("david")));
		// }

		// StorageEngine->AsyncSaveEntity(User, FNativeOnStorageEntitySaveDelegate::CreateLambda([](IGameStorageEntityPtr Entity, const FString& ErrorMsg)
		// {
		// }));
		// 	StorageEngine->AsyncLoadEntity(User, FNativeOnStorageEntityLoadDelegate::CreateLambda([](IGameStorageEntityPtr Entity, const FString& ErrorMsg)
		// 	{
		// 	}));
	}

	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGameStorageTest_EntityObjects, "GameStorage.EntityObjects", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGameStorageTest_EntityObjects::RunTest(const FString& Param)
{
	//
	// Warning !!!! will crash !!
	// 
	auto StorageEngine = IGameStorageEngine::GetDefault();
	if (StorageEngine)
	{
		// Save
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Save ------ "));
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

			FGameEntityStorageKey EntityKey(TEXT("player"), User->UserName);

			StorageEngine->SaveEntity(User, EntityKey);
			// StorageEngine->SaveEntity(User, FGameEntityFieldStorageKey(EntityKey, TEXT("profile")));
			// StorageEngine->SaveEntity(User->Items[0], FGameEntityFieldStorageKey(EntityKey, TEXT("item:1")));
			// StorageEngine->SaveEntity(User->Items[0], FGameEntityFieldStorageKey(EntityKey, TEXT("item:2")));
		}

		// Load
		UE_LOG(LogGameStorage, Log, TEXT("Basic --------- Load ------ "));
		{
			auto User = NewObject<UGameStorageTestUser>();


			FGameEntityStorageKey EntityKey(TEXT("player"), TEXT("david2"));

			StorageEngine->LoadEntity(User, EntityKey);
			// StorageEngine->LoadEntityObject(User->Profile, EntityKey, TEXT("profile"));
			// StorageEngine->LoadEntityObjects<UGameStorageTestUserItem>(User->Items, EntityKey, TEXT("item"));


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
