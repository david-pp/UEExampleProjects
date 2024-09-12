#pragma once
#include "TinyHttpService.h"
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
struct FTestDeviceGetReply
{
	GENERATED_BODY()

	UPROPERTY()
	FTestDevice Device;
};

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

	bool HandleQueryDevices(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleCreateDevice(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleGetDevice(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleUpdateDevice(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);
	bool HandleDeleteDevice(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);


	// Advanced Mode
	int HandleGetDeviceEx(const FTestDeviceGetRequest& Request, FTestDeviceGetReply& Reply, FString& Error);

protected:
	TMap<FGuid, FTestDevice> Devices;
};
