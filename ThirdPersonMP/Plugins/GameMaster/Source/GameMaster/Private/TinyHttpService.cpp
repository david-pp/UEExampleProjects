#include "TinyHttpService.h"
#include "HttpServerModule.h"

DEFINE_LOG_CATEGORY(LogTinyHttp);

void FTinyHttp::ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload)
{
	int32 StartIndex = OutTCHARPayload.Num();
	OutTCHARPayload.AddUninitialized(FUTF8ToTCHAR_Convert::ConvertedLength((ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR)) * sizeof(TCHAR));
	FUTF8ToTCHAR_Convert::Convert((TCHAR*)(OutTCHARPayload.GetData() + StartIndex), (OutTCHARPayload.Num() - StartIndex) / sizeof(TCHAR), (ANSICHAR*)InUTF8Payload.GetData(), InUTF8Payload.Num() / sizeof(ANSICHAR));
}

void FTinyHttp::ConvertToUTF8(TConstArrayView<uint8> InTCHARPayload, TArray<uint8>& OutUTF8Payload)
{
	int32 StartIndex = OutUTF8Payload.Num();
	OutUTF8Payload.AddUninitialized(FTCHARToUTF8_Convert::ConvertedLength((TCHAR*)InTCHARPayload.GetData(), InTCHARPayload.Num() / sizeof(TCHAR)) * sizeof(ANSICHAR));
	FTCHARToUTF8_Convert::Convert((ANSICHAR*)(OutUTF8Payload.GetData() + StartIndex), (OutUTF8Payload.Num() - StartIndex) / sizeof(ANSICHAR), (TCHAR*)InTCHARPayload.GetData(), InTCHARPayload.Num() / sizeof(TCHAR));
}

void FTinyHttp::ConvertToUTF8(const FString& InString, TArray<uint8>& OutUTF8Payload)
{
	int32 StartIndex = OutUTF8Payload.Num();
	OutUTF8Payload.AddUninitialized(FTCHARToUTF8_Convert::ConvertedLength(*InString, InString.Len()) * sizeof(ANSICHAR));
	FTCHARToUTF8_Convert::Convert((ANSICHAR*)(OutUTF8Payload.GetData() + StartIndex), (OutUTF8Payload.Num() - StartIndex) / sizeof(ANSICHAR), *InString, InString.Len());
}

bool FTinyHttp::JsonPayloadToUStruct(const TArray<uint8>& InUTF8Payload, const UStruct* StructDefinition, void* OutStruct)
{
	TArray<uint8> TCHARBody;
	ConvertToTCHAR(InUTF8Payload, TCHARBody);

	FMemoryReaderView BodyReader(TCHARBody);
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(&BodyReader);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogTinyHttp, Warning, TEXT("JsonPayloadToUStruct - Unable to deserialize"));
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(JsonObject.ToSharedRef(), StructDefinition, OutStruct))
	{
		UE_LOG(LogTinyHttp, Warning, TEXT("JsonPayloadToUStruct - Unable to convert to struct"));
		return false;
	}

	return true;
}

bool FTinyHttp::UStructToJsonPayload(TArray<uint8>& OutUTF8Payload, const UStruct* StructDefinition, const void* Struct)
{
	FString JsonString;
	if (!FJsonObjectConverter::UStructToJsonObjectString(StructDefinition, Struct, JsonString))
	{
		return false;
	}

	ConvertToUTF8(JsonString, OutUTF8Payload);
	return true;
}

void FTinyHttp::DumpServerRequest(const FHttpServerRequest& Request, const FString& LogTitle)
{
	UE_LOG(LogTinyHttp, Log, TEXT("--- %s ServerRequest : %s"), *LogTitle, *Request.RelativePath.GetPath());

	for (auto Header : Request.Headers)
	{
		UE_LOG(LogTinyHttp, Log, TEXT("- Header : %s"), *Header.Key);
		for (auto Value : Header.Value)
		{
			UE_LOG(LogTinyHttp, Log, TEXT("-- Value : %s"), *Value);
		}
	}

	for (auto QueryParam : Request.QueryParams)
	{
		UE_LOG(LogTinyHttp, Log, TEXT("- QueryParam : %s -> %s"), *QueryParam.Key, *QueryParam.Value);
	}

	for (auto PathParam : Request.PathParams)
	{
		UE_LOG(LogTinyHttp, Log, TEXT("- PathParam : %s -> %s"), *PathParam.Key, *PathParam.Value);
	}
}

FTinyHttpService::FTinyHttpService(uint32 InServicePort, const FString& InServiceName) : ServicePort(InServicePort), ServiceName(InServiceName)
{
}

FTinyHttpService::~FTinyHttpService()
{
}

void FTinyHttpService::Start()
{
	if (!HttpRouter)
	{
		HttpRouter = FHttpServerModule::Get().GetHttpRouter(ServicePort);
		if (!HttpRouter)
		{
			UE_LOG(LogTinyHttp, Error, TEXT("%s couldn't be started on port %d"), *ServiceName, ServicePort);
			return;
		}

		for (FHttpRequestRoute& Route : RegisteredHttpRoutes)
		{
			StartRoute(Route);
		}

		// Go through externally registered request pre-processors and register them with the http router.
		for (const TPair<FDelegateHandle, FHttpRequestHandler>& Handler : PreprocessorsToRegister)
		{
			// Find the pre-processors HTTP-handle from the one we generated.
			FDelegateHandle& Handle = PreprocessorsHandleMappings.FindChecked(Handler.Key);
			if (Handle.IsValid())
			{
				HttpRouter->UnregisterRequestPreprocessor(Handle);
			}

			// Update the preprocessor handle mapping.
			Handle = HttpRouter->RegisterRequestPreprocessor(Handler.Value);
		}

		FHttpServerModule::Get().StartAllListeners();
		OnServiceStartedDelegate.Broadcast(ServicePort);
	}
}

void FTinyHttpService::Stop()
{
	if (FHttpServerModule::IsAvailable())
	{
		FHttpServerModule::Get().StopAllListeners();
	}

	if (HttpRouter)
	{
		for (const TPair<uint32, FHttpRouteHandle>& Tuple : ActiveRouteHandles)
		{
			if (Tuple.Key)
			{
				HttpRouter->UnbindRoute(Tuple.Value);
			}
		}

		ActiveRouteHandles.Reset();
	}

	HttpRouter.Reset();
	OnServiceStoppedDelegate.Broadcast(ServicePort);
}

void FTinyHttpService::RegisterRoute(const FHttpRequestRoute& Route)
{
	RegisteredHttpRoutes.Add(Route);

	// If the route is registered after the server is already started.
	if (HttpRouter)
	{
		StartRoute(Route);
	}
}

void FTinyHttpService::UnregisterRoute(const FHttpRequestRoute& Route)
{
	RegisteredHttpRoutes.Remove(Route);
	const uint32 RouteHash = GetTypeHash(Route);
	if (FHttpRouteHandle* Handle = ActiveRouteHandles.Find(RouteHash))
	{
		if (HttpRouter)
		{
			HttpRouter->UnbindRoute(*Handle);
		}
		ActiveRouteHandles.Remove(RouteHash);
	}
}

void FTinyHttpService::StartRoute(const FHttpRequestRoute& Route)
{
	// @formatter:off
	
	// The handler is wrapped in a lambda since HttpRouter::BindRoute only accepts TFunctions
	ActiveRouteHandles.Add(GetTypeHash(Route),
				HttpRouter->BindRoute(Route.Path, Route.Verb,
				[this, Handler = Route.Handler](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
						{
							return Handler.Execute(Request, OnComplete);
						}));
	// @formatter:on
}

void FTinyHttpService::RegisterRoutes()
{
	// @formatter:off
	
	RegisterRoute({TEXT("Get help information about services"),
		FHttpPath(TEXT("/help")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpServiceHandler::CreateRaw(this, &FTinyHttpService::HandleHelpInfo)});

	// @formatter:on
}

FDelegateHandle FTinyHttpService::RegisterRequestPreprocessor(FHttpRequestHandler RequestPreprocessor)
{
	FDelegateHandle WebHandle{FDelegateHandle::GenerateNewHandle};

	PreprocessorsToRegister.Add(WebHandle, RequestPreprocessor);

	FDelegateHandle HttpRouterHandle;
	if (HttpRouter)
	{
		HttpRouterHandle = HttpRouter->RegisterRequestPreprocessor(MoveTemp(RequestPreprocessor));
	}

	PreprocessorsHandleMappings.Add(WebHandle, HttpRouterHandle);
	return WebHandle;
}

void FTinyHttpService::UnregisterRequestPreprocessor(const FDelegateHandle& RequestPreprocessorHandle)
{
	PreprocessorsToRegister.Remove(RequestPreprocessorHandle);
	if (FDelegateHandle* HttpRouterHandle = PreprocessorsHandleMappings.Find(RequestPreprocessorHandle))
	{
		if (HttpRouterHandle->IsValid())
		{
			HttpRouter->UnregisterRequestPreprocessor(*HttpRouterHandle);
		}

		PreprocessorsHandleMappings.Remove(RequestPreprocessorHandle);
	}
}

bool FTinyHttpService::HandleHelpInfo(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	return true;
}
