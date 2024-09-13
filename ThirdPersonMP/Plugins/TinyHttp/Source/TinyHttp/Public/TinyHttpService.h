#pragma once

#include "HttpPath.h"
#include "HttpRouteHandle.h"
#include "HttpServerRequest.h"
#include "HttpServerResponse.h"
#include "IHttpRouter.h"
#include "TinyHttpTypes.h"
#include "TinyHttpUtils.h"

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


/**
 * HTTP Request Route Rules
 */
struct FHttpRequestRoute
{
	FHttpRequestRoute(FString InRouteDescription, FHttpPath InPath, EHttpServerRequestVerbs InVerb, FHttpServiceHandler InHandler, bool bInDebugRequest = false)
		: RouteDescription(MoveTemp(InRouteDescription)), Path(MoveTemp(InPath)), Verb(InVerb), Handler(MoveTemp(InHandler)), bDebugRequest(bInDebugRequest)
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
	/** Dump http request to log ? */
	bool bDebugRequest;

	friend uint32 GetTypeHash(const FHttpRequestRoute& Route) { return HashCombine(GetTypeHash(Route.Path), GetTypeHash(Route.Verb)); }
	friend bool operator==(const FHttpRequestRoute& LHS, const FHttpRequestRoute& RHS) { return LHS.Path == RHS.Path && LHS.Verb == RHS.Verb; }
};

/**
 * A Tiny Http Service
 */
class TINYHTTP_API FTinyHttpService
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
	template <typename RequestType, typename ResponseType>
	void RegisterRoute(FString InRouteDescription, FHttpPath InPath, EHttpServerRequestVerbs InVerb,
		const TFunction<FServiceResponsePtr (const RequestType&)>& ServiceHandler)
	{
		FHttpRequestRoute Route(InRouteDescription, InPath, InVerb, FHttpServiceHandler::CreateLambda(
			[ServiceHandler](const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
			{
				RequestType Request;
				if (FTinyHttp::DeserializeRequest(HttpRequest, Request))
				{
					FServiceResponsePtr Response = ServiceHandler(Request);
					if (Response)
					{
						OnComplete(FTinyHttp::ServiceResponse(Response, ResponseType::StaticStruct()));
					}
					else
					{
						// need ? 
						OnComplete(FTinyHttp::ServiceError(SERVICE_NO_REPLY, TEXT("Service has no reply")));
					}
				}
				else
				{
					OnComplete(FTinyHttp::ServiceError(SERVICE_INVALID_REQUEST,TEXT("Invalid Request")));
				}
				return true;
			}));
		
		RegisterRoute(Route);
	}

	template <typename RequestType, typename ResponseType>
	void OnGet(const FString& InDescription, const FString& InPath, const TFunction<FServiceResponsePtr()>& ServiceHandler)
	{
		FHttpRequestRoute Route(InDescription, FHttpPath(InPath), EHttpServerRequestVerbs::VERB_GET, FHttpServiceHandler::CreateLambda(
			[ServiceHandler](const FHttpServerRequest& HttpRequest, const FHttpResultCallback& OnComplete)
		{
			FServiceResponsePtr Response = ServiceHandler();
			if (Response)
			{
				OnComplete(FTinyHttp::ServiceResponse(Response, ResponseType::StaticStruct()));
			}
			else
			{
				OnComplete(FTinyHttp::ServiceError(SERVICE_NO_REPLY, TEXT("Service has no reply")));
			}
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
