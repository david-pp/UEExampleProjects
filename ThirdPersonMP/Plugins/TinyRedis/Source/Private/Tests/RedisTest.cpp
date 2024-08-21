#include "CoreMinimal.h"
#include "AsyncRedisCommand.h"
#include "JsonObjectConverter.h"
#include "RedisTestObject.h"
#include "TinyRedis.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/Formatters/JsonArchiveInputFormatter.h"
#include "Serialization/Formatters/JsonArchiveOutputFormatter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_Async, "Redis.Async", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_Async::RunTest(const FString& Param)
{
	IRedisInterfacePtr Redis = ITinyRedisModule::GetTinyRedis();
	if (Redis)
	{
		FRedisReply Reply = Redis->ExecCommand("hmset user:xxx name david sex male age 30");
		UE_LOG(LogRedis, Warning, TEXT("hmset - %s"), *Reply.ToDebugString());

		Redis->AsyncExecCommand("hgetall user:xxx").Then([](TFuture<FRedisReply> Future)
		{
			UE_LOG(LogRedis, Warning, TEXT("hgetall -  %s"), *Future.Get().ToDebugString());
		});
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_API, "Redis.API", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_API::RunTest(const FString& Param)
{
	IRedisInterfacePtr Redis = ITinyRedisModule::GetTinyRedis();
	if (Redis)
	{
		// Set/Get General
		{
			Redis->Set(TEXT("redis:test1:k1"), 1024);
			Redis->Set(TEXT("redis:test1:k2"), TEXT("a-simple-text"));
			Redis->Set(TEXT("redis:test1:k3"), 3.1415926);

			int32 V1 = Redis->Get<int32>(TEXT("redis:test1:k1"));
			FString V2 = Redis->Get<FString>(TEXT("redis:test1:k2"));
			float V3 = Redis->Get<float>(TEXT("redis:test1:k3"));

			UE_LOG(LogRedis, Log, TEXT("Test1 - Key1=%d"), V1);
			UE_LOG(LogRedis, Log, TEXT("Test1 - Key2=%s"), *V2);
			UE_LOG(LogRedis, Log, TEXT("Test1 - Key3=%f"), V3);
		}

		// Set/Get UTF8 String
		{
			Redis->SetStr(TEXT("redis:test2:key1"), TEXT("a simple long text"));
			Redis->SetStr(TEXT("redis:test2:key2"), TEXT("我是一段中文  间隔  再来一段"));

			FString V1 = Redis->GetStr(TEXT("redis:test2:key1")).String;
			FString V2 = Redis->GetStr(TEXT("redis:test2:key2")).String;

			UE_LOG(LogRedis, Log, TEXT("Test2 - Key1=%s"), *V1);
			UE_LOG(LogRedis, Log, TEXT("Test2 - Key2=%s"), *V2);
		}

		// Set/Get Binary
		{
			// binary array
			Redis->SetBin(TEXT("redis:test3:k1"), {0x01, 0x02, 0x03, 0x04});

			// UObject
			{
				URedisTestObject* Obj = NewObject<URedisTestObject>();
				Obj->Health = 120.0f;
				Obj->Ammo = 100;
				Obj->Location = FVector(20, 30, 40);

				FBufferArchive BinAr;
				// BinAr << *Obj;         // Customize
				Obj->Serialize(BinAr); // UObject 
				Redis->SetBin(TEXT("redis:test3:k2"), BinAr);
			}

			// get binary array
			{
				TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:test3:k1")).BinArray;
				for (auto& Bin : BinArray)
				{
					UE_LOG(LogRedis, Log, TEXT("Test3 - K1:%x"), Bin);
				}
			}

			// get TestObject
			{
				TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:test3:k2")).BinArray;
				FMemoryReader BinAr(BinArray, true);

				URedisTestObject* Obj = NewObject<URedisTestObject>();
				BinAr.Seek(0);
				// BinAr << *Obj;            // Customize
				Obj->Serialize(BinAr); // UObject 

				UE_LOG(LogRedis, Log, TEXT("Test3 - K2.Health=%.f"), Obj->Health);
				UE_LOG(LogRedis, Log, TEXT("Test3 - K2.Ammo=%d"), Obj->Ammo);
				UE_LOG(LogRedis, Log, TEXT("Test3 - K2.Location=%s"), *Obj->Location.ToString());
			}
		}

		// Set/Get UObject
		{
			// Save
			{
				URedisTestObject* Obj = NewObject<URedisTestObject>();
				Obj->Health = 120.0f;
				Obj->Ammo = 100;
				Obj->Location = FVector(20, 30, 40);

				// Object
				{
					FBufferArchive BinAr;
					Obj->Serialize(BinAr);
					Redis->SetBin(TEXT("redis:obj:k1"), BinAr);
				}

				// Archive
				{
					FBufferArchive BinAr;
					BinAr << *Obj;
					Redis->SetBin(TEXT("redis:obj:k2"), BinAr);
				}

				// Json - default
				{
					FBufferArchive BinAr;
					FJsonArchiveOutputFormatter Formatter(BinAr);
					FStructuredArchive StructuredArchive(Formatter);
					FStructuredArchiveRecord RootRecord = StructuredArchive.Open().EnterRecord();
					Obj->Serialize(RootRecord);
					StructuredArchive.Close();

					Redis->SetBin(TEXT("redis:obj:k3"), BinAr);
				}

				// Json - customize
				{
					FBufferArchive BinAr;
					FJsonArchiveOutputFormatter Formatter(BinAr);
					FStructuredArchive StructuredArchive(Formatter);
					Obj->SerializeJson(StructuredArchive);

					Redis->SetBin(TEXT("redis:obj:k4"), BinAr);
				}
			}
			// Load
			{
				// Object
				{
					TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:obj:k1")).BinArray;
					FMemoryReader BinAr(BinArray, true);

					URedisTestObject* Obj = NewObject<URedisTestObject>();
					BinAr.Seek(0);
					Obj->Serialize(BinAr);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Object.Health=%.f"), Obj->Health);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Object.Ammo=%d"), Obj->Ammo);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Object.Location=%s"), *Obj->Location.ToString());
				}

				// Archive
				{
					TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:obj:k2")).BinArray;
					FMemoryReader BinAr(BinArray, true);

					URedisTestObject* Obj = NewObject<URedisTestObject>();
					BinAr.Seek(0);
					BinAr << *Obj;

					UE_LOG(LogRedis, Log, TEXT("Test4 - Archive.Health=%.f"), Obj->Health);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Archive.Ammo=%d"), Obj->Ammo);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Archive.Location=%s"), *Obj->Location.ToString());
				}

				// Json - default
				{
					TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:obj:k3")).BinArray;
					FMemoryReader BinAr(BinArray, true);

					URedisTestObject* Obj = NewObject<URedisTestObject>();
					FJsonArchiveInputFormatter Formatter(BinAr);
					FStructuredArchive StructuredArchive(Formatter);
					Obj->Serialize(StructuredArchive.Open().EnterRecord());
					StructuredArchive.Close();


					FString JsonBody = FString(BinArray.Num(), ANSI_TO_TCHAR((char*)BinArray.GetData()));

					UE_LOG(LogRedis, Log, TEXT("Test4 - Json : %s"), *JsonBody);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Json.Health=%.f"), Obj->Health);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Json.Ammo=%d"), Obj->Ammo);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Json.Location=%s"), *Obj->Location.ToString());
				}

				// Json - customize
				{
					TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:obj:k4")).BinArray;
					FMemoryReader BinAr(BinArray, true);

					URedisTestObject* Obj = NewObject<URedisTestObject>();
					FJsonArchiveInputFormatter Formatter(BinAr);
					FStructuredArchive StructuredArchive(Formatter);
					Obj->SerializeJson(StructuredArchive);

					FString JsonBody = FString(BinArray.Num(), ANSI_TO_TCHAR((char*)BinArray.GetData()));

					UE_LOG(LogRedis, Log, TEXT("Test4 - Json : %s"), *JsonBody);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Json.Health=%.f"), Obj->Health);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Json.Ammo=%d"), Obj->Ammo);
					UE_LOG(LogRedis, Log, TEXT("Test4 - Json.Location=%s"), *Obj->Location.ToString());
				}

				// Object Modified
				{
					// Object
					{
						TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:obj:k1")).BinArray;
						FMemoryReader BinAr(BinArray, true);
						{
							BinAr.Seek(0);

							URedisTestObject_ModifyAdd* Obj = NewObject<URedisTestObject_ModifyAdd>();
							Obj->Serialize(BinAr);
							UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Health=%.f"), Obj->Health);
							UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Ammo=%d"), Obj->Ammo);
							UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Mana=%d"), Obj->Mana);
							UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Location=%s"), *Obj->Location.ToString());
						}
						{
							BinAr.Seek(0);
							URedisTestObject_ModifyRemove* Obj = NewObject<URedisTestObject_ModifyRemove>();
							Obj->Serialize(BinAr);
							UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Remove.Health=%.f"), Obj->Health);
							UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Remove.Location=%s"), *Obj->Location.ToString());
						}
					}
				}

				// Json
				{
					TArray<uint8> BinArray = Redis->GetBin(TEXT("redis:obj:k3")).BinArray;
					FMemoryReader BinAr(BinArray, true);
					FString JsonBody = FString(BinArray.Num(), ANSI_TO_TCHAR((char*)BinArray.GetData()));
					{
						BinAr.Seek(0);
						URedisTestObject_ModifyAdd* Obj = NewObject<URedisTestObject_ModifyAdd>();
						FJsonArchiveInputFormatter Formatter(BinAr);
						FStructuredArchive StructuredArchive(Formatter);
						Obj->Serialize(StructuredArchive.Open().EnterRecord());
						StructuredArchive.Close();

						UE_LOG(LogRedis, Log, TEXT("Test4 - Json : %s"), *JsonBody);
						UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Health=%.f"), Obj->Health);
						UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Ammo=%d"), Obj->Ammo);
						UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Mana=%d"), Obj->Mana);
						UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Add.Location=%s"), *Obj->Location.ToString());
					}
					{
						BinAr.Seek(0);
						URedisTestObject_ModifyRemove* Obj = NewObject<URedisTestObject_ModifyRemove>();
						FJsonArchiveInputFormatter Formatter(BinAr);
						FStructuredArchive StructuredArchive(Formatter);
						Obj->Serialize(StructuredArchive.Open().EnterRecord());
						StructuredArchive.Close();

						UE_LOG(LogRedis, Log, TEXT("Test4 - Json : %s"), *JsonBody);
						UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Remove.Health=%.f"), Obj->Health);
						UE_LOG(LogRedis, Log, TEXT("Test4 - Object_Remove.Location=%s"), *Obj->Location.ToString());
					}
				}
			}
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_AsyncAPI, "Redis.AsyncAPI", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_AsyncAPI::RunTest(const FString& Param)
{
	IRedisInterfacePtr Redis = ITinyRedisModule::GetTinyRedis();
	if (Redis)
	{
		Redis->AsyncSet(TEXT("game:title"), TEXT("tinyredis"));
		Redis->AsyncSet(TEXT("game:price"), 200);
		Redis->AsyncSetStr(TEXT("game:tinyredis:desc"), TEXT("a simple async redis client wrapper"));
		Redis->AsyncSetBin(TEXT("game:tinyredis:binary"), {0x01, 0x02, 0x03, 0x04});

		FPlatformProcess::Sleep(0.2); // wait for async api is done

		Redis->AsyncGet(TEXT("game:title")).Then([Redis](TFuture<FRedisReply> Future)
		{
			FString Value = Future.Get().String;
			Redis->AsyncGetStr(FString::Printf(TEXT("game:%s:desc"), *Value)).Then([](TFuture<FRedisReply> Future)
			{
				UE_LOG(LogRedis, Warning, TEXT("desc : %s"), *Future.Get().String);
			});
		});

		auto Future = Redis->AsyncGet(TEXT("game:price")).Then([](TFuture<FRedisReply> Future)
		{
			UE_LOG(LogRedis, Warning, TEXT("prices : %d"), Future.Get().GetStringAs<int32>());
		});

		Redis->AsyncGetBin(TEXT("game:tinyredis:binary")).Then([](TFuture<FRedisReply> Future)
		{
			TArray<uint8> BinArray = Future.Get().BinArray;
			for (auto& Bin : BinArray)
			{
				UE_LOG(LogRedis, Log, TEXT("binary : %x"), Bin);
			}
		});

		FPlatformProcess::Sleep(0.2); // wait for async api is done
	}
	return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_PipelineAPI, "Redis.Pipeline", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

typedef TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> FCondensedJsonStringWriterFactory;
typedef TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> FCondensedJsonStringWriter;

typedef TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>> FPrettyJsonStringWriterFactory;
typedef TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>> FPrettyJsonStringWriter;

bool FRedisTest_PipelineAPI::RunTest(const FString& Param)
{
	IRedisInterfacePtr Redis = ITinyRedisModule::GetTinyRedis();
	if (!Redis) return false;

	auto Pipeline = Redis->CreatePipeline();

	// Sync Pipeline
	if (Pipeline)
	{
		Pipeline->Start();
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		FRedisPipelineReply PipelineReply = Pipeline->Commit();

		UE_LOG(LogRedis, Log, TEXT("Pipeline - Sync : %s"), *PipelineReply.ToDebugString());
	}

	// Async Pipeline
	if (Pipeline)
	{
		Pipeline->Start();
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AppendCommand(new FTinyRedisCommand_HSet());
		Pipeline->AsyncCommit().Then([](TFuture<FRedisPipelineReply> Future)
		{
			UE_LOG(LogRedis, Log, TEXT("Pipeline - Async : %s"), *Future.Get().ToDebugString());
		});
	}

	// Object Save
	{
		URedisTestObject* Obj = NewObject<URedisTestObject>();
		Obj->TypeName = TEXT("david");
		Obj->Health = 120.0f;
		Obj->Ammo = 100;
		Obj->Location = FVector(20, 30, 40);

		for (TFieldIterator<FProperty> It(URedisTestObject::StaticClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Obj);

			TSharedPtr<FJsonValue> JsonValue = FJsonObjectConverter::UPropertyToJsonValue(Property, PropertyValue);


			TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
			JsonObject->SetField(Property->GetName(), JsonValue);

			FString JsonString;
			FJsonSerializer::Serialize(JsonObject, FCondensedJsonStringWriterFactory::Create(&JsonString));
			// FString JsonIdentifier = TEXT("");// Property->GetName();
			// FJsonSerializer::Serialize(JsonValue, JsonIdentifier, FCondensedJsonStringWriterFactory::Create(&JsonString));

			UE_LOG(LogRedis, Log, TEXT("URedisTestObject.%s -> Json:%s"), *Property->GetName(), *JsonString);

			// Write it to Redis
			Redis->HashSet(TEXT("user:david"), Property->GetName(), JsonString);
		}
	}

	// Object Load
	{
		URedisTestObject* Obj = NewObject<URedisTestObject>();

		for (TFieldIterator<FProperty> It(URedisTestObject::StaticClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Obj);

			// Read from Redis
			FString JsonString = Redis->HashGet<FString>(TEXT("user:david"), Property->GetName());

			UE_LOG(LogRedis, Log, TEXT("Pipeline - Load Json: %s"), *JsonString);

			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
			TSharedPtr<FJsonObject> JsonObject;
			FJsonSerializer::Deserialize(JsonReader, JsonObject);

			if (JsonObject)
			{
				TSharedPtr<FJsonValue> JsonValue = JsonObject->TryGetField(Property->GetName());
				if (JsonValue) {
					FJsonObjectConverter::JsonValueToUProperty(JsonValue, Property, PropertyValue);
				}
			}
		}

		UE_LOG(LogRedis, Log, TEXT("Pipeline - Object.Health=%.f"), Obj->Health);
		UE_LOG(LogRedis, Log, TEXT("Pipeline - Object.Ammo=%d"), Obj->Ammo);
		UE_LOG(LogRedis, Log, TEXT("Pipeline - Object.Location=%s"), *Obj->Location.ToString());
	}

	return true;
}
