﻿#include "TinyHttpService.h"
#include "HttpServerModule.h"
#include "JsonObjectConverter.h"
#include "TinyHttp.h"


// Json Protocol
namespace
{
	TSharedPtr<FJsonObject> MakeOkJsonObject(const TSharedPtr<FJsonObject>& Data = nullptr)
	{
		TSharedPtr<FJsonObject> JsonBody = MakeShared<FJsonObject>();
		JsonBody->SetStringField(TEXT("Code"), FString::Printf(TEXT("%d"), EHttpServerResponseCodes::Ok));
		JsonBody->SetStringField(TEXT("Status"), TEXT("OK"));
		if (Data)
		{
			JsonBody->SetObjectField(TEXT("Payload"), Data);
		}

		return JsonBody;
	}

	FString MakeOkJsonString(const TSharedPtr<FJsonObject>& Data = nullptr)
	{
		FString JsonString;
		TSharedPtr<FJsonObject> JsonBody = MakeOkJsonObject(Data);
		if (JsonBody)
		{
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
			FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);
		}
		return MoveTemp(JsonString);
	}

	TSharedPtr<FJsonObject> MakeErrorJsonObject(const EHttpServerResponseCodes& HttpCode, const int ErrorCode, const FString& ErrorMessage, const TSharedPtr<FJsonObject>& ErrorDetails = nullptr)
	{
		TSharedPtr<FJsonObject> JsonBody = MakeShared<FJsonObject>();
		JsonBody->SetStringField(TEXT("Code"), FString::Printf(TEXT("%d"), EHttpServerResponseCodes::BadRequest));
		JsonBody->SetStringField(TEXT("Status"), TEXT("BadRequest"));

		if (ErrorCode > 0 || ErrorMessage.Len() > 0)
		{
			JsonBody->SetStringField(TEXT("ErrorCode"), LexToString(ErrorCode));
			JsonBody->SetStringField(TEXT("ErrorMessage"), ErrorMessage.ReplaceCharWithEscapedChar());
		}

		if (ErrorDetails)
		{
			JsonBody->SetObjectField(TEXT("ErrorDetails"), ErrorDetails);
		}

		return JsonBody;
	}

	FString MakeErrorJsonString(const EHttpServerResponseCodes& HttpCode, const int ErrorCode, const FString& ErrorMessage, const TSharedPtr<FJsonObject>& ErrorDetails = nullptr)
	{
		FString JsonString;
		TSharedPtr<FJsonObject> JsonBody = MakeErrorJsonObject(HttpCode, ErrorCode, ErrorMessage, ErrorDetails);
		if (JsonBody)
		{
			TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
			FJsonSerializer::Serialize(JsonBody.ToSharedRef(), Writer);
		}
		return MoveTemp(JsonString);
	}
}

TUniquePtr<FHttpServerResponse> FTinyHttp::ServiceResponseInternal(const FServiceResponsePtr& ServiceResponse, const UStruct* ServiceResponseStruct)
{
	EHttpServerResponseCodes HttpCode = EHttpServerResponseCodes::Ok;
	TSharedRef<FJsonObject> JsonResponse = MakeShared<FJsonObject>();

	if (ServiceResponse && ServiceResponseStruct)
	{
		if (ServiceResponse->HasError())
		{
			HttpCode = EHttpServerResponseCodes::BadRequest;
			JsonResponse->SetStringField(TEXT("Code"), FString::Printf(TEXT("%d"), EHttpServerResponseCodes::BadRequest));
			JsonResponse->SetStringField(TEXT("Status"), TEXT("BadRequest"));

			JsonResponse->SetStringField(TEXT("ErrorCode"), LexToString(ServiceResponse->ErrorCode));
			JsonResponse->SetStringField(TEXT("ErrorMessage"), ServiceResponse->ErrorMessage.ReplaceCharWithEscapedChar());
		}
		else // OK
		{
			HttpCode = EHttpServerResponseCodes::Ok;
			JsonResponse->SetStringField(TEXT("Code"), FString::Printf(TEXT("%d"), EHttpServerResponseCodes::Ok));
			JsonResponse->SetStringField(TEXT("Status"), TEXT("OK"));

			TSharedRef<FJsonObject> JsonPayload = MakeShared<FJsonObject>();
			if (FJsonObjectConverter::UStructToJsonObject(ServiceResponseStruct, ServiceResponse.Get(), JsonPayload))
			{
				JsonResponse->SetObjectField(TEXT("Payload"), JsonPayload);
			}
			else
			{
				UE_LOG(LogTinyHttp, Warning, TEXT("ServiceResponseInternal - Struct to JsonObject Failed: %s"), *ServiceResponseStruct->GetName());
			}
		}
	}
	else // No response payload
	{
		HttpCode = EHttpServerResponseCodes::Ok;
		JsonResponse->SetStringField(TEXT("Code"), FString::Printf(TEXT("%d"), EHttpServerResponseCodes::Ok));
		JsonResponse->SetStringField(TEXT("Status"), TEXT("OK"));
	}

	FString JsonString;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&JsonString);
	if (!FJsonSerializer::Serialize(JsonResponse, Writer))
	{
		return FHttpServerResponse::Error(EHttpServerResponseCodes::ServerError, TEXT(""), TEXT("serialize response failed"));
	}

	auto Response = FHttpServerResponse::Create(JsonString, TEXT("application/json"));
	Response->Code = HttpCode;
	return Response;
}

TUniquePtr<FHttpServerResponse> FTinyHttp::ServiceOKInternal(const UStruct* StructDefinition, const void* Struct)
{
	FString JsonString;
	if (StructDefinition && Struct)
	{
		TSharedRef<FJsonObject> DataJsonObject = MakeShared<FJsonObject>();
		if (FJsonObjectConverter::UStructToJsonObject(StructDefinition, Struct, DataJsonObject))
		{
			JsonString = MakeOkJsonString(DataJsonObject);
		}
		else
		{
			JsonString = MakeOkJsonString();
			UE_LOG(LogTinyHttp, Warning, TEXT("ServiceOKInternal - Struct to JsonObject Failed: %s"), *StructDefinition->GetName());
		}
	}
	else
	{
		JsonString = MakeOkJsonString();
	}

	auto Response = FHttpServerResponse::Create(JsonString, TEXT("application/json"));
	Response->Code = EHttpServerResponseCodes::Ok;
	return Response;
}

TUniquePtr<FHttpServerResponse> FTinyHttp::ServiceErrorInternal(const int ErrorCode, const FString& ErrorMessage, const UStruct* StructDefinition, const void* Struct)
{
	FString JsonString;
	if (StructDefinition && Struct)
	{
		TSharedRef<FJsonObject> ErrorDetail = MakeShared<FJsonObject>();
		if (FJsonObjectConverter::UStructToJsonObject(StructDefinition, Struct, ErrorDetail))
		{
			JsonString = MakeErrorJsonString(EHttpServerResponseCodes::BadRequest, ErrorCode, ErrorMessage, ErrorDetail);
		}
		else
		{
			JsonString = MakeErrorJsonString(EHttpServerResponseCodes::BadRequest, ErrorCode, ErrorMessage);
			UE_LOG(LogTinyHttp, Warning, TEXT("ServiceErrorInternal - Struct to JsonObject Failed: %s"), *StructDefinition->GetName());
		}
	}
	else
	{
		JsonString = MakeErrorJsonString(EHttpServerResponseCodes::BadRequest, ErrorCode, ErrorMessage);
	}

	auto Response = FHttpServerResponse::Create(JsonString, TEXT("application/json"));
	Response->Code = EHttpServerResponseCodes::BadRequest;
	return Response;
}

// -------------------------------------

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
	[this, Route](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
			{
				if (Route.bDebugRequest)
				{
					UE_LOG(LogTinyHttp, Warning, TEXT("### %s : %s\n%s\n"),
						*Route.Path.GetPath(), *Route.RouteDescription,  *FTinyHttp::RequestToDebugString(Request));
				}
				return Route.Handler.Execute(Request, OnComplete);
			}));
	// @formatter:on
}

void FTinyHttpService::RegisterRoutes()
{
	// @formatter:off
	
	RegisterRoute({TEXT("Get help information about services"),
		FHttpPath(TEXT("/help")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpServiceHandler::CreateRaw(this, &FTinyHttpService::HandleHelpInfo)
	});


	RegisterRoute({TEXT("error response"),
		FHttpPath(TEXT("/help/error")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpServiceHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			OnComplete(FTinyHttp::ServiceError(0,TEXT("InvalidRequest")));
			return true;
		}),
		true
	});
	
	RegisterRoute({TEXT("ok response"),
		FHttpPath(TEXT("/help/ok")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpServiceHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			OnComplete(FTinyHttp::ServiceOK());
			return true;
		}),
		true
	});

	RegisterRoute({TEXT("Post request demo"),
		FHttpPath(TEXT("/help/post-demo")),
		EHttpServerRequestVerbs::VERB_POST,
		FHttpServiceHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			UE_LOG(LogTinyHttp, Warning, TEXT("### Post Demo ----\n%s\n"), *FTinyHttp::RequestToDebugString(Request));
			OnComplete(FTinyHttp::ServiceOK());
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
<h2>Tiny Http Server : {{ServiceName}} </h2>
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
	RouteTable += HelpRouterTableHeader;
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
	Body.ReplaceInline(TEXT("{{ServiceName}}"), *ServiceName);
	Body.ReplaceInline(TEXT("{{RouteTable}}"), *RouteTable);
	OnComplete(FHttpServerResponse::Create(Body, TEXT("text/html")));
	return true;
}
