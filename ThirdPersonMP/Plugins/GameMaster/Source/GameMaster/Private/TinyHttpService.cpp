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

FString FTinyHttp::RequestVerbToString(EHttpServerRequestVerbs Verb)
{
	switch (Verb)
	{
	case EHttpServerRequestVerbs::VERB_GET:
		return TEXT("GET");
	case EHttpServerRequestVerbs::VERB_POST:
		return TEXT("POST");
	case EHttpServerRequestVerbs::VERB_PUT:
		return TEXT("PUT");
	case EHttpServerRequestVerbs::VERB_PATCH:
		return TEXT("PATCH");
	case EHttpServerRequestVerbs::VERB_DELETE:
		return TEXT("DELETE");
	default:
		break;
	}
	return TEXT("Unkown");
}

FString FTinyHttp::RequestToDebugString(const FHttpServerRequest& Request, bool bShowBody)
{
	FString Result;

	FString QueryParams;
	for (auto It : Request.QueryParams)
	{
		if (!QueryParams.IsEmpty())
		{
			QueryParams.InsertAt(0, TEXT("&"));
		}

		QueryParams += FString::Printf(TEXT("%s=%s"), *It.Key, *It.Value);
	}

	// VERB URL?QueryParams
	Result += FString::Printf(TEXT("%s %s?%s\n"), *RequestVerbToString(Request.Verb), *Request.RelativePath.GetPath(), *QueryParams);

	// Header-Key : Value1,Value2
	for (auto Header : Request.Headers)
	{
		Result += FString::Printf(TEXT("%s : %s\n"), *Header.Key, *FString::Join(Header.Value, TEXT(",")));
	}

	// Body ?
	if (bShowBody)
	{
		FString ContentTypes = FString::Join(Request.Headers.FindRef(TEXT("Content-Type")), TEXT(","));

		if (ContentTypes.Find(TEXT("json")) != INDEX_NONE || ContentTypes.Find(TEXT("text")) != INDEX_NONE)
		{
			TArray<uint8> TCHARBody;
			ConvertToTCHAR(Request.Body, TCHARBody);

			FString BodyString(TCHARBody.Num(), (TCHAR*)TCHARBody.GetData());
			Result += FString::Printf(TEXT("\n%s\n"), *BodyString);
		}
		else
		{
			Result += FString::Printf(TEXT("\nHex, Length=%d\n"), Request.Body.Num());
		}
	}

	// Query Paths
	for (auto It : Request.QueryParams)
	{
		Result += FString::Printf(TEXT("[%s] = %s\n"), *It.Key, *It.Value);
	}

	return Result;
}

TUniquePtr<FHttpServerResponse> FTinyHttp::CreateResponse(EHttpServerResponseCodes InResponseCode)
{
	TUniquePtr<FHttpServerResponse> Response = MakeUnique<FHttpServerResponse>();
	Response->Code = InResponseCode;
	Response->Headers.FindOrAdd(TEXT("Content-Type")).Add(TEXT("application/json"));
	return Response;
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

	RegisterRoute({TEXT("Get help information about services"),
		FHttpPath(TEXT("/help/post-demo")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpServiceHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			UE_LOG(LogTinyHttp, Warning, TEXT("### Post Demo ----\n%s\n"), *FTinyHttp::RequestToDebugString(Request));
			OnComplete(FTinyHttp::CreateResponse(EHttpServerResponseCodes::Ok));
			return true;
		})});

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

const char* HelpHtmlTemplate = R"html(
<!DOCTYPE html>
<html>
<style>
table, th, td {
  border:1px solid black;
}
</style>
<body>
<h2>Tiny Http Server</h2>
<p>Routes :</p>

{{RouteTable}}

</body>
</html>
)html";

const char* HelpRouterTableHeader = R"html(
<table style="width:100%">
  <tr>
    <th>Method</th>
    <th>Path</th>
    <th>Description</th>
	<th>Handler</th>
  </tr>
)html";
const char* HelpRouteTableRowTemplate = R"html(
  <tr>
    <td>{{Method}}</td>
    <td>{{Path}}</td>
    <td>{{Description}}</td>
	<td>{{Handler}}</td>
  </tr>
)html";
const char* HelpRouterTableBottom = R"html(
</table>
)html";

bool FTinyHttpService::HandleHelpInfo(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
{
	UE_LOG(LogTinyHttp, Warning, TEXT("### Help  ----\n%s\n"), *FTinyHttp::RequestToDebugString(Request));

	FString RouteTable;
	RouteTable += HelpRouteTableRowTemplate;
	for (FHttpRequestRoute& Route : RegisteredHttpRoutes)
	{
		FString RouteRow = HelpRouteTableRowTemplate;
		RouteRow.ReplaceInline(TEXT("{{Method}}"), *FTinyHttp::RequestVerbToString(Route.Verb));
		RouteRow.ReplaceInline(TEXT("{{Path}}"), *Route.Path.GetPath());
		RouteRow.ReplaceInline(TEXT("{{Description}}"), *Route.RouteDescription);
		RouteRow.ReplaceInline(TEXT("{{Handler}}"), TEXT(""));

		RouteTable += RouteRow;
	}
	RouteTable += HelpRouterTableBottom;

	FString Body = HelpHtmlTemplate;
	Body.ReplaceInline(TEXT("{{RouteTable}}"), *RouteTable);
	OnComplete(FHttpServerResponse::Create(Body, TEXT("text/html")));
	return true;
}
