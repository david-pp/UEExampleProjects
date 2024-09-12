#pragma once

#include "HttpPath.h"
#include "HttpRouteHandle.h"
#include "HttpServerResponse.h"
#include "HttpServerRequest.h"
#include "IHttpRouter.h"

/**
 * FHttpServiceHandler 
 *
 *  NOTE - Returning true implies that the delegate will eventually invoke OnComplete
 *  NOTE - Returning false implies that the delegate will never invoke OnComplete
 * 
 * @param Request The incoming http request to be handled
 * @param OnComplete The callback to invoke to write an http response
 * @return True if the request has been handled, false otherwise
 */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FHttpServiceHandler, const FHttpServerRequest&, const FHttpResultCallback&);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnHttpServiceStarted, uint32 /*Port*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHttpServiceStoped, uint32 /*Port*/);

DECLARE_LOG_CATEGORY_EXTERN(LogTinyHttp, Log, All);

#define SERVICE_OK 0

struct FTinyHttp
{
	/**
	 * Convert a UTF-8 payload to a TCHAR payload.
	 * @param InUTF8Payload The UTF-8 payload in binary format.
	 * @param OutTCHARPayload The converted TCHAR output in binary format.
	 */
	static void ConvertToTCHAR(TConstArrayView<uint8> InUTF8Payload, TArray<uint8>& OutTCHARPayload);
	static void ConvertToString(TConstArrayView<uint8> InUTF8Payload, FString& OutString);

	/**
	 * Convert a TCHAR payload to UTF-8.
	 * @param InTCHARPayload The TCHAR payload in binary format.
	 * @param OutUTF8Payload The converted UTF-8 output in binary format.
	 */
	static void ConvertToUTF8(TConstArrayView<uint8> InTCHARPayload, TArray<uint8>& OutUTF8Payload);

	/**
	 * Convert a FString to UTF-8.
	 * @param InString The string to be converted.
	 * @param OutUTF8Payload the converted UTF-8 output in binary format.
	 */
	static void ConvertToUTF8(const FString& InString, TArray<uint8>& OutUTF8Payload);

	/** Json UTF8 <-> UStruct */
	static bool JsonPayloadToUStruct(const TArray<uint8>& InUTF8Payload, const UStruct* StructDefinition, void* OutStruct);
	static bool UStructToJsonPayload(TArray<uint8>& OutUTF8Payload, const UStruct* StructDefinition, const void* Struct);

	/**
	 * Deserialize a request into a UStruct.
	 * @param InRequest The incoming http request.
	 * @param OutDeserializedRequest The structure to serialize using the request's content.
	 * @return Whether the deserialization was successful. 
	 * 
	 * @note InCompleteCallback will be called with an appropriate http response if the deserialization fails.
	 */
	template <typename RequestType>
	static bool DeserializeRequest(const FHttpServerRequest& InRequest, RequestType& OutDeserializedRequest)
	{
		return JsonPayloadToUStruct(InRequest.Body, RequestType::StaticStruct(), &OutDeserializedRequest);
	}

	/**
	 * Serialize a response object into a UTF-8 Payload.
	 * @param InResponseObject the object to serialize.
	 * @param OutResponsePayload the resulting UTF-8 payload.
	 */
	template <typename ResponseType>
	static bool SerializeResponse(const ResponseType& InResponseObject, TArray<uint8>& OutResponsePayload)
	{
		return UStructToJsonPayload(OutResponsePayload, ResponseType::StaticStruct(), &InResponseObject);
	}

	static FString RequestVerbToString(EHttpServerRequestVerbs Verb);
	static FString RequestBodyToString(const FHttpServerRequest& Request);
	static FString RequestToDebugString(const FHttpServerRequest& Request, bool bShowBody = true);


	/**
	 * Service Response
	 */
	static TUniquePtr<FHttpServerResponse> ServiceOK()
	{
		return ServiceOKInternal(nullptr, nullptr);
	}

	template <typename ResponseDataStruct>
	static TUniquePtr<FHttpServerResponse> ServiceOK(const ResponseDataStruct& Data)
	{
		return ServiceOKInternal(ResponseDataStruct::StaticStruct(), &Data);
	}

	static TUniquePtr<FHttpServerResponse> ServiceError(const int ErrorCode, const FString& ErrorMessage)
	{
		return ServiceErrorInternal(ErrorCode, ErrorMessage, nullptr, nullptr);
	}

	template <typename ErrorDetailStruct>
	static TUniquePtr<FHttpServerResponse> ServiceError(const int ErrorCode, const FString& ErrorMessage, const ErrorDetailStruct& ErrorDetail)
	{
		return ServiceErrorInternal(ErrorCode, ErrorMessage, ErrorDetailStruct::StaticStruct(), &ErrorDetail);
	}

protected:
	static TUniquePtr<FHttpServerResponse> ServiceOKInternal(const UStruct* StructDefinition, const void* Struct);
	static TUniquePtr<FHttpServerResponse> ServiceErrorInternal(const int ErrorCode, const FString& ErrorMessage, const UStruct* StructDefinition, const void* Struct);
};

/**
 * HTTP Request Route Rules
 */
struct FHttpRequestRoute
{
	FHttpRequestRoute(FString InRouteDescription, FHttpPath InPath, EHttpServerRequestVerbs InVerb, FHttpServiceHandler InHandler)
		: RouteDescription(MoveTemp(InRouteDescription)), Path(MoveTemp(InPath)), Verb(InVerb), Handler(MoveTemp(InHandler))
	{
	}

	/** A description of how the route should be used. */
	FString RouteDescription;
	/** Relative path (ie. /remote/object) */
	FHttpPath Path;
	/** The desired HTTP verb (ie. GET, PUT..) */
	EHttpServerRequestVerbs Verb = EHttpServerRequestVerbs::VERB_GET;
	/** The handler called when the route is accessed. */
	FHttpServiceHandler Handler;

	friend uint32 GetTypeHash(const FHttpRequestRoute& Route) { return HashCombine(GetTypeHash(Route.Path), GetTypeHash(Route.Verb)); }
	friend bool operator==(const FHttpRequestRoute& LHS, const FHttpRequestRoute& RHS) { return LHS.Path == RHS.Path && LHS.Verb == RHS.Verb; }
};

/**
 * A Tiny Http Service
 */
class FTinyHttpService
{
public:
	FTinyHttpService(uint32 InServicePort, const FString& InServiceName = TEXT("HttpService"));
	virtual ~FTinyHttpService();

	/**
	 * Start the web service
	 */
	void Start();

	/**
	 * Start the web service.
	 */
	void Stop();

	/**
	 * Register/Unregister a route to the API.
	 * @param Route The route to register.
	 */
	void RegisterRoute(const FHttpRequestRoute& Route);
	void UnregisterRoute(const FHttpRequestRoute& Route);

	// @formatter:off
	template <typename RequestType, typename ReplyType>
	void RegisterRoute(FString InRouteDescription, FHttpPath InPath, EHttpServerRequestVerbs InVerb,
		TUniqueFunction<int (const RequestType&, ReplyType&, FString& ErrorMsg)> ServiceWork)
	{
	
		FHttpRequestRoute Route(InRouteDescription, InPath, InVerb, FHttpServiceHandler::CreateLambda(
			[ServiceWork](const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
			{
				RequestType Request;
				if (!FTinyHttp::DeserializeRequest(HttpRequest, Request))
				{
					OnComplete(FTinyHttp::ServiceError(100,TEXT("Invalid Request")));
				}

				ReplyType Reply;
				FString ErrorMsg;
				int ErrorCode = ServiceWork(Request, Reply, ErrorMsg);
				if(0 == ErrorCode)
				{
					OnComplete(FTinyHttp::ServiceOK(Reply));
				}
				else
				{
					OnComplete(FTinyHttp::ServiceError(ErrorCode, ErrorMsg));
				}
				
				return true;
			}));
		
		RegisterRoute(Route);
	}
	// @formatter:on

	/**
	 * Register/Unregister request preprocessor
	 */
	virtual FDelegateHandle RegisterRequestPreprocessor(FHttpRequestHandler RequestPreprocessor);
	virtual void UnregisterRequestPreprocessor(const FDelegateHandle& RequestPreprocessorHandle);

	/** Register HTTP routes (to override) */
	virtual void RegisterRoutes();

	/**
	 * Callbacks for service start/stop 
	 */
	virtual FOnHttpServiceStarted& OnServiceStarted() { return OnServiceStartedDelegate; }
	virtual FOnHttpServiceStoped& OnServiceStopped() { return OnServiceStoppedDelegate; }

protected:
	/** Bind the route in the http router and add it to the list of active routes. */
	void StartRoute(const FHttpRequestRoute& Route);
	bool HandleHelpInfo(const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete);

	/** Port of the http server. */
	uint32 ServicePort;
	/** Service display name */
	FString ServiceName;

	/** Http router handle */
	TSharedPtr<IHttpRouter> HttpRouter;

	/** Mapping of routes to delegate handles */
	TMap<uint32, FHttpRouteHandle> ActiveRouteHandles;
	/** Set of routes that will be activated on http server start. */
	TSet<FHttpRequestRoute> RegisteredHttpRoutes;

	/** List of preprocessor delegates that need to be registered when the server is started. */
	TMap<FDelegateHandle, FHttpRequestHandler> PreprocessorsToRegister;
	/** Mappings of preprocessors delegate handles generated from the other module to the ones generated from the Http Module. */
	TMap<FDelegateHandle, FDelegateHandle> PreprocessorsHandleMappings;

	//~ Server started stopped delegates.
	FOnHttpServiceStarted OnServiceStartedDelegate;
	FOnHttpServiceStoped OnServiceStoppedDelegate;
};

typedef TSharedPtr<FTinyHttpService> FTinyHttpServicePtr;
