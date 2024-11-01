#include "CoreMinimal.h"
#include "AsyncRedisTask.h"
#include "JsonObjectConverter.h"
#include "RedisTestObject.h"
#include "TinyRedis.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/Formatters/JsonArchiveInputFormatter.h"
#include "Serialization/Formatters/JsonArchiveOutputFormatter.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_Basic, "Redis.Basic", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_Basic::RunTest(const FString& Param)
{
	ITinyRedisPtr Redis = ITinyRedisModule::GetTinyRedis();
	if (!Redis) return false;

	// General Command
	{
		FRedisReply Reply = Redis->Command<FTinyRedisCommand>(TEXT("hmset user:basic2 name david sex male age 30"));
		UE_LOG(LogRedis, Warning, TEXT("Basic - hmset: %s"), *Reply.ToDebugString());

		Redis->AsyncCommand<FTinyRedisCommand>(TEXT("hgetall user:basic2")).Then([](TFuture<FRedisReply> Future)
		{
			UE_LOG(LogRedis, Warning, TEXT("Basic - Hgetall: %s"), *Future.Get().ToDebugString());
		});
	}

	// Simple Command
	{
		FRedisReply Reply = Redis->ExecCommand("hmset user:xxx name david sex male age 30");
		UE_LOG(LogRedis, Warning, TEXT("hmset - %s"), *Reply.ToDebugString());

		Redis->AsyncExecCommand("hgetall user:xxx").Then([](TFuture<FRedisReply> Future)
		{
			UE_LOG(LogRedis, Warning, TEXT("Baic - hgetall: %s"), *Future.Get().ToDebugString());
		});
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_API, "Redis.API", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FRedisTest_API::RunTest(const FString& Param)
{
	ITinyRedisPtr Redis = ITinyRedisModule::GetTinyRedis();
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
	ITinyRedisPtr Redis = ITinyRedisModule::GetTinyRedis();
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

bool FRedisTest_PipelineAPI::RunTest(const FString& Param)
{
	ITinyRedisPtr Redis = ITinyRedisModule::GetTinyRedis();
	if (!Redis) return false;

	// Sync Pipeline
	{
		// Save
		{
			auto Pipeline = Redis->CreatePipeline();
			Pipeline->Start();
			Pipeline->Command<TinyRedis::HashSet>(TEXT("user:test1"), TEXT("name"), TEXT("dddd"));
			Pipeline->Command<TinyRedis::HashSet>(TEXT("user:test1"), TEXT("age"), TEXT("100"));
			FRedisPipelineReply PipelineReply = Pipeline->Commit();

			UE_LOG(LogRedis, Log, TEXT("Pipeline - Sync Sets Results : %s"), *PipelineReply.ToDebugString());
		}

		// Load
		{
			auto Pipeline = Redis->CreatePipeline();
			Pipeline->Start();
			Pipeline->Command<TinyRedis::HashGet>(TEXT("user:test1"), TEXT("name"))->OnReply.BindLambda([](const FRedisReply& Reply)
			{
				UE_LOG(LogRedis, Log, TEXT("Pipeline - Sync Get : Name=%s"), *Reply.String);
			});
			Pipeline->Command<TinyRedis::HashGet>(TEXT("user:test1"), TEXT("age"))->OnReply.BindLambda([](const FRedisReply& Reply)
			{
				UE_LOG(LogRedis, Log, TEXT("Pipeline - Sync Get : Age=%s"), *Reply.String);
			});

			FRedisPipelineReply PipelineReply = Pipeline->Commit();
			UE_LOG(LogRedis, Log, TEXT("Pipeline - Sync Gets Results : %s"), *PipelineReply.ToDebugString());
		}
	}

	// Async Pipeline
	{
		// Save
		{
			auto Pipeline = Redis->CreatePipeline();
			Pipeline->Start();
			Pipeline->Command<TinyRedis::HashSet>(TEXT("user:test2"), TEXT("name"), TEXT("eeeee"));
			Pipeline->Command<TinyRedis::HashSet>(TEXT("user:test2"), TEXT("age"), TEXT("200"));

			Pipeline->AsyncCommit().Then([](TFuture<FRedisPipelineReply> Future)
			{
				UE_LOG(LogRedis, Log, TEXT("Pipeline - Async Sets Results :\n%s"), *Future.Get().ToDebugString());
			});
		}

		FPlatformProcess::Sleep(0.2); // wait for async api is done

		// Load
		{
			auto Pipeline = Redis->CreatePipeline();
			Pipeline->Start();
			Pipeline->Command<TinyRedis::HashGet>(TEXT("user:test2"), TEXT("name"))->OnReply.BindLambda([](const FRedisReply& Reply)
			{
				UE_LOG(LogRedis, Log, TEXT("Pipeline - ASync Get : Name=%s"), *Reply.String);
			});
			Pipeline->Command<TinyRedis::HashGet>(TEXT("user:test2"), TEXT("age"))->OnReply.BindLambda([](const FRedisReply& Reply)
			{
				UE_LOG(LogRedis, Log, TEXT("Pipeline - ASync Get : Age=%s"), *Reply.String);
			});

			Pipeline->AsyncCommit().Then([](TFuture<FRedisPipelineReply> Future)
			{
				UE_LOG(LogRedis, Log, TEXT("Pipeline - Async Gets Results :\n%s"), *Future.Get().ToDebugString());
			});
		}

		FPlatformProcess::Sleep(0.2); // wait for async api is done
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRedisTest_ObjectMappingAPI, "Redis.ObjectMapping", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

typedef TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> FCondensedJsonStringWriterFactory;
typedef TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>> FCondensedJsonStringWriter;

typedef TJsonWriterFactory<TCHAR, TPrettyJsonPrintPolicy<TCHAR>> FPrettyJsonStringWriterFactory;
typedef TJsonWriter<TCHAR, TPrettyJsonPrintPolicy<TCHAR>> FPrettyJsonStringWriter;

bool FRedisTest_ObjectMappingAPI::RunTest(const FString& Param)
{
	ITinyRedisPtr Redis = ITinyRedisModule::GetTinyRedis();
	if (!Redis) return false;

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

			// Property -> Json String
			FString JsonString;
			{
				TSharedRef<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
				TSharedPtr<FJsonValue> JsonValue = FJsonObjectConverter::UPropertyToJsonValue(Property, PropertyValue);
				JsonObject->SetField(Property->GetName(), JsonValue);
				FJsonSerializer::Serialize(JsonObject, FCondensedJsonStringWriterFactory::Create(&JsonString));
				// FString JsonIdentifier = TEXT("");// Property->GetName();
				// FJsonSerializer::Serialize(JsonValue, JsonIdentifier, FCondensedJsonStringWriterFactory::Create(&JsonString));
			}

			UE_LOG(LogRedis, Log, TEXT("Mapping - URedisTestObject.%s -> Json:%s"), *Property->GetName(), *JsonString);

			// Write it to Redis
			Redis->HashSet(TEXT("user:david"), Property->GetName(), JsonString);
		}
	}

	// Object Load by Multiple HGET
	{
		URedisTestObject* Obj = NewObject<URedisTestObject>();

		for (TFieldIterator<FProperty> It(URedisTestObject::StaticClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Obj);

			// Read from Redis
			FString JsonString = Redis->HashGet<FString>(TEXT("user:david"), Property->GetName());
			UE_LOG(LogRedis, Log, TEXT("Mapping - Load Json: %s"), *JsonString);

			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
			TSharedPtr<FJsonObject> JsonObject;
			FJsonSerializer::Deserialize(JsonReader, JsonObject);
			if (JsonObject)
			{
				TSharedPtr<FJsonValue> JsonValue = JsonObject->TryGetField(Property->GetName());
				if (JsonValue)
				{
					FJsonObjectConverter::JsonValueToUProperty(JsonValue, Property, PropertyValue);
				}
			}
		}

		UE_LOG(LogRedis, Log, TEXT("Mapping - Object.Health=%.f"), Obj->Health);
		UE_LOG(LogRedis, Log, TEXT("Mapping - Object.Ammo=%d"), Obj->Ammo);
		UE_LOG(LogRedis, Log, TEXT("Mapping - Object.Location=%s"), *Obj->Location.ToString());
	}

	// Object Load by Pipeline
	{
		URedisTestObject* Obj = NewObject<URedisTestObject>();
		auto Pipeline = Redis->CreatePipeline();

		// Pipeline start
		Pipeline->Start();

		// pipeline the HGET command
		for (TFieldIterator<FProperty> It(URedisTestObject::StaticClass()); It; ++It)
		{
			FProperty* Property = *It;
			void* PropertyValue = Property->ContainerPtrToValuePtr<void>(Obj);

			Pipeline->Command<TinyRedis::HashGet>(TEXT("user:david"), Property->GetName())->OnReply.BindLambda([Property, PropertyValue](const FRedisReply& Reply)
			{
				FString JsonString = Reply.String;
				UE_LOG(LogRedis, Log, TEXT("Mapping - Load by Pipeline Json: %s"), *JsonString);
				TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
				TSharedPtr<FJsonObject> JsonObject;
				FJsonSerializer::Deserialize(JsonReader, JsonObject);
				if (JsonObject)
				{
					TSharedPtr<FJsonValue> JsonValue = JsonObject->TryGetField(Property->GetName());
					if (JsonValue)
					{
						FJsonObjectConverter::JsonValueToUProperty(JsonValue, Property, PropertyValue);
					}
				}
			});
		}

		// sync commit
		Pipeline->Commit();

		UE_LOG(LogRedis, Log, TEXT("Mapping - Object2.Health=%.f"), Obj->Health);
		UE_LOG(LogRedis, Log, TEXT("Mapping - Object2.Ammo=%d"), Obj->Ammo);
		UE_LOG(LogRedis, Log, TEXT("Mapping - Object2.Location=%s"), *Obj->Location.ToString());
	}
	return true;
}
