#pragma once
#include "TinyHttpService.h"
#include "TinyHttpTypes.h"
#include "TinyHttpRestTest.generated.h"

USTRUCT()
struct FTestDevice
{
	GENERATED_BODY()

	UPROPERTY()
	FGuid DeviceId;

	UPROPERTY()
	FString DeviceName;

	UPROPERTY()
	FString DeviceType;

	UPROPERTY()
	TArray<FString> DeviceUsers;
};

USTRUCT()
struct FTestDeviceCollection
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FTestDevice> Devices;
};

USTRUCT()
struct FTestDeviceCreateRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FTestDevice Device;
};


// Update
USTRUCT()
struct FTestDeviceUpdateRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FString DeviceName;
	UPROPERTY()
	FString DeviceType;
	UPROPERTY()
	TArray<FString> DeviceUsers;
};

// Get Device Details
USTRUCT()
struct FTestDeviceGetRequest
{
	GENERATED_BODY()

	UPROPERTY()
	FString DeviceID;
};

USTRUCT()
struct FTestDeviceGetResponse : public FServiceResponse
{
	GENERATED_BODY()

	using FServiceResponse::FServiceResponse;
	
	UPROPERTY()
	FTestDevice Device;
};

typedef TFunction<FServiceResponsePtr (const FTestDeviceGetRequest&)> FTestDeviceGetHandler;

/**
* GET   /device-management/devices     : Get all devices (filter by params)
* POST  /device-management/devices     : Create a new device
* GET   /device-management/devices/{id} : Get the device information identified by "id"
* PUT   /device-management/devices/{id} : Update the device information identified by "id"
* DELETE /device-management/devices/{id} : Delete device by "id"
*
* GET  /device-management/devices/{id}/name : get device name
*/
class FTinyHttpRestTest : public FTinyHttpService
{
public:
	using FTinyHttpService::FTinyHttpService;

	virtual void RegisterRoutes() override;

	bool HandleQueryDevices(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete);
	bool HandleCreateDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete);
	bool HandleGetDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete);
	bool HandleUpdateDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete);
	bool HandleDeleteDevice(const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete);


	// Advanced Mode
	FServiceResponsePtr HandleGetDeviceEx(const FTestDeviceGetRequest& Request);

protected:
	TMap<FGuid, FTestDevice> Devices;
};
