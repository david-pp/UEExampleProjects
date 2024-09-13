#include "TinyHttpRestTest.h"

#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "TinyHttp.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"


/**
* GET   /device-management/devices     : Get all devices
* POST  /device-management/devices     : Create a new device
* GET   /device-management/devices/{id} : Get the device information identified by "id"
* PUT   /device-management/devices/{id} : Update the device information identified by "id"
* DELETE /device-management/devices/{id} : Delete device by "id"
 */

// @formatter:off
void FTinyHttpRestTest::RegisterRoutes()
{
	FTinyHttpService::RegisterRoutes();
	
	RegisterRoute({
		TEXT("Get all devices"),
		FHttpPath(TEXT("/devices")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpServiceHandler::CreateRaw(this, &FTinyHttpRestTest::HandleQueryDevices),
		true
	});

	// Create
	RegisterRoute({
		TEXT("Create a new device"),
		FHttpPath(TEXT("/devices")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpServiceHandler::CreateRaw(this, &FTinyHttpRestTest::HandleCreateDevice),
		true
	});

	// Get
	RegisterRoute({
		TEXT("Get the device information"),
		FHttpPath(TEXT("/devices/:device")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpServiceHandler::CreateLambda([this](const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
		{
			FTestDeviceGetRequest Request;
			Request.DeviceID = HttpRequest.PathParams.FindRef(TEXT("device"));
			FServiceResponsePtr Response = HandleGetDeviceEx(Request);
			OnComplete(FTinyHttp::ServiceResponse(Response, FTestDeviceGetResponse::StaticStruct()));
			return true;
		}),
		true
	});
	// Get Ex
	// RegisterRoute<FTestDeviceGetRequest, FTestDeviceGetResponse> (
	// 	TEXT("Get the device information"),
	// 	FHttpPath(TEXT("/devices/:device")),
	// 	EHttpServerRequestVerbs::VERB_GET,
	// 	[this](const FTestDeviceGetRequest& Request) 
	// 	{
	// 		return this->HandleGetDeviceEx(Request);
	// 	});

	// Update
	RegisterRoute({
		TEXT("Update the device information"),
		FHttpPath(TEXT("/devices/:device")),
		EHttpServerRequestVerbs::VERB_PUT,
		FHttpServiceHandler::CreateRaw(this, &FTinyHttpRestTest::HandleUpdateDevice),
		true
	});

	// Delete
	RegisterRoute({
		TEXT("Delete the device"),
		FHttpPath(TEXT("/devices/:device")),
		EHttpServerRequestVerbs::VERB_DELETE,
		FHttpServiceHandler::CreateRaw(this, &FTinyHttpRestTest::HandleDeleteDevice),
		true
	});
}
// @formatter:on

bool FTinyHttpRestTest::HandleQueryDevices(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	FString DeviceType = Request.QueryParams.FindRef(TEXT("type"));

	FTestDeviceCollection DeviceCollection;
	for (TTuple<FGuid, FTestDevice>& It : Devices)
	{
		// filter by ?type=XXX
		if (DeviceType.IsEmpty() || DeviceType == It.Value.DeviceType)
		{
			DeviceCollection.Devices.Add(It.Value);
		}
	}

	OnComplete(FTinyHttp::ServiceOK(DeviceCollection));
	return true;
}

bool FTinyHttpRestTest::HandleCreateDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
{
	UE_LOG(LogTinyHttp, Warning, TEXT("HandleCreateDevice : \n%s"), *FTinyHttp::RequestToDebugString(HttpRequest));

	FTestDeviceCreateRequest CreateRequest;
	if (FTinyHttp::DeserializeRequest(HttpRequest, CreateRequest))
	{
		CreateRequest.Device.DeviceId = FGuid::NewGuid();
		Devices.Add(CreateRequest.Device.DeviceId, CreateRequest.Device);

		OnComplete(FTinyHttp::ServiceOK());
	}
	else
	{
		OnComplete(FTinyHttp::ServiceError(101, TEXT("Invalid request")));
	}

	return true;
}

bool FTinyHttpRestTest::HandleGetDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
{
	FTestDeviceGetRequest Request;
	if (!FTinyHttp::DeserializeRequest(HttpRequest, Request))
	{
		OnComplete(FTinyHttp::ServiceError(100,TEXT("Invalid Request")));
		return true;
	}

	FServiceResponsePtr Response = HandleGetDeviceEx(Request);
	if (Response)
	{
		OnComplete(FTinyHttp::ServiceResponse(Response, FTestDeviceGetResponse::StaticStruct()));
	}

	return true;
}

FServiceResponsePtr FTinyHttpRestTest::HandleGetDeviceEx(const FTestDeviceGetRequest& Request)
{
	FGuid DeviceID;
	FGuid::Parse(Request.DeviceID, DeviceID);

	FTestDevice* Device = Devices.Find(DeviceID);
	if (!Device)
	{
		return MakeShared<FTestDeviceGetResponse>(100, TEXT("Can't find device"));
	}

	auto Response = MakeShared<FTestDeviceGetResponse>();
	Response->Device = *Device;
	return Response;
}

bool FTinyHttpRestTest::HandleUpdateDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
{
	FGuid DeviceID;
	FString DeviceIDString = HttpRequest.PathParams.FindRef(TEXT("device"));
	FGuid::Parse(DeviceIDString, DeviceID);

	FTestDeviceUpdateRequest UpdateRequest;
	if (!FTinyHttp::DeserializeRequest(HttpRequest, UpdateRequest))
	{
		OnComplete(FTinyHttp::ServiceError(100, TEXT("Invalid request")));
		return true;
	}

	FTestDevice& Device = Devices.FindOrAdd(DeviceID);
	Device.DeviceId = DeviceID;
	Device.DeviceName = UpdateRequest.DeviceName;
	Device.DeviceType = UpdateRequest.DeviceType;
	Device.DeviceUsers = UpdateRequest.DeviceUsers;

	OnComplete(FTinyHttp::ServiceOK());
	return true;
}

bool FTinyHttpRestTest::HandleDeleteDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
{
	FGuid DeviceID;
	FString DeviceIDString = HttpRequest.PathParams.FindRef(TEXT("device"));
	FGuid::Parse(DeviceIDString, DeviceID);

	FTestDevice* Device = Devices.Find(DeviceID);
	if (!Device)
	{
		OnComplete(FTinyHttp::ServiceError(101, TEXT("Can't find device")));
		return true;
	}

	Devices.Remove(DeviceID);
	OnComplete(FTinyHttp::ServiceOK());
	return true;
}


// ------------------------- Test -----------------------------------

BEGIN_DEFINE_SPEC(FTinyHttpServiceSpec, "TinyHttp.TestService", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
	TSharedPtr<FTinyHttpRestTest> Service;
	uint32 ServicePort = 8090;
	FString ServiceURL = TEXT("http://127.0.0.1:8090/");

	void GetDeviceTest();
END_DEFINE_SPEC(FTinyHttpServiceSpec)

void FTinyHttpServiceSpec::Define()
{
	Service = MakeShared<FTinyHttpRestTest>(ServicePort, TEXT("TestService"));
	Service->RegisterRoutes();
	Service->Start();

	Describe("Spec for Http Service", [this]
	{
		BeforeEach([this]
		{
			TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
			Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
			{
				TestTrue(TEXT("connect should success"), bSucceeded);
				UE_LOG(LogTinyHttp, Log, TEXT("Put a device :%s\n"), *HttpResponse->GetContentAsString());
			});

			FTestDeviceUpdateRequest DeviceRequest;
			DeviceRequest.DeviceName = TEXT("David's PC");
			DeviceRequest.DeviceType = TEXT("PC");
			DeviceRequest.DeviceUsers.Append({TEXT("David1"), TEXT("中文名字")});

			TArray<uint8> OutUTF8Payload;
			FTinyHttp::UStructToJsonPayload(OutUTF8Payload, FTestDeviceUpdateRequest::StaticStruct(), &DeviceRequest);

			Request->SetURL(ServiceURL / TEXT("devices/BA9A2E6844BF65F3BC84D0A2230AE870"));
			Request->SetContent(OutUTF8Payload);
			Request->SetVerb(TEXT("PUT"));
			Request->ProcessRequest();
		});

		It("Get Device", [this]
		{
			GetDeviceTest();
		});

		AfterEach([this]
		{
		});
	});

	Describe("Get device", [this]
	{
		It("Get Device", [this]
		{
			GetDeviceTest();
		});
	});


	// Service->Stop();
}

void FTinyHttpServiceSpec::GetDeviceTest()
{
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
	Request->OnProcessRequestComplete().BindLambda([this](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
	{
		TestTrue(TEXT("connect should success"), bSucceeded);
		UE_LOG(LogTinyHttp, Log, TEXT("Get a device :%s\n"), *HttpResponse->GetContentAsString());

		if (HttpResponse->GetResponseCode() == (int32)EHttpServerResponseCodes::Ok)
		{
			TSharedPtr<FJsonObject> JsonResponse = FTinyHttp::JsonPayloadToObject(HttpResponse->GetContent());
			if (JsonResponse)
			{
				const TSharedPtr<FJsonObject>& JsonPayload = JsonResponse->GetObjectField(TEXT("Payload"));

				FTestDeviceGetResponse GetResponse;
				if (FJsonObjectConverter::JsonObjectToUStruct(JsonPayload.ToSharedRef(), FTestDeviceGetResponse::StaticStruct(), &GetResponse))
				{
					FTestDevice Device = GetResponse.Device;
					TestEqual(TEXT("Device Name"), Device.DeviceName, TEXT("David's PC"));
					TestEqual(TEXT("Device Type"), Device.DeviceType, TEXT("PC"));

					TestEqual(TEXT("Device Userx2"), Device.DeviceUsers.Num(), 2);
					if (Device.DeviceUsers.Num() == 2)
					{
						TestEqual(TEXT("Device User[0]"), Device.DeviceUsers[0], TEXT("David1"));
						TestEqual(TEXT("Device User[2]"), Device.DeviceUsers[1], TEXT("中文名字"));
					}

					UE_LOG(LogTinyHttp, Log, TEXT("Get Device Result :%s,%s"), *Device.DeviceId.ToString(), *Device.DeviceName);
				}
			}
		}
		else if (HttpResponse->GetResponseCode() == (int32)EHttpServerResponseCodes::BadRequest)
		{
			TSharedPtr<FJsonObject> JsonResponse = FTinyHttp::JsonPayloadToObject(HttpResponse->GetContent());
			if (JsonResponse)
			{
				FString ErrorCode = JsonResponse->GetStringField(TEXT("ErrorCode"));
				FString ErrorMessage = JsonResponse->GetStringField(TEXT("ErrorMessage"));

				UE_LOG(LogTinyHttp, Log, TEXT("Get Device Error :%s,%s"), *ErrorCode, *ErrorMessage.ReplaceEscapedCharWithChar());
			}
		}
	});

	Request->SetURL(ServiceURL / TEXT("devices/BA9A2E6844BF65F3BC84D0A2230AE870"));
	Request->SetVerb(TEXT("GET"));
	Request->ProcessRequest();
}
