// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMaster.h"
#include "GameSessionTypes.h"
#include "IHttpRequest.h"
#include "IHttpResponse.h"
#include "JsonObjectConverter.h"
// #include "DSMasterServiceClient.generated.h"

// Create Session
typedef TFunction<void(FGameSessionDetails& Reply, bool bSucceeded)> FCreateSessionCallback;
// Find Session
typedef TFunction<void(FGameSessionSearchResult& Reply, bool bSucceeded)> FFindSessionCallback;
// Update Session
typedef TFunction<void(FGameSessionUpdateReply& Reply, bool bSucceeded)> FUpdateSessionCallback;

enum class DSMASTER_API EHttpRequestVerbs : uint16
{
	VERB_NONE = 0,
	VERB_GET = 1 << 0,
	VERB_POST = 1 << 1,
	VERB_PUT = 1 << 2,
	VERB_PATCH = 1 << 3,
	VERB_DELETE = 1 << 4,
	VERB_OPTIONS = 1 << 5
};

class DSMASTER_API FDSMasterHttpClient
{
public:
	FDSMasterHttpClient()
	{
		ServiceURL = TEXT("http://127.0.0.1:30000");
	}

	bool Init(const FString URL)
	{
		ServiceURL = URL;
		return true;
	}

	bool RequestCreateSession(const FGameSessionDetails& InRequest, const FCreateSessionCallback& Callback)
	{
		return Request<FGameSessionDetails, FGameSessionDetails>(TEXT("/session"), EHttpRequestVerbs::VERB_POST, InRequest, Callback);
	}

	bool RequestFindSessions(const FString& QueryParams, const FFindSessionCallback& Callback)
	{
		FString URL = "/sessions";
		if (!QueryParams.IsEmpty())
		{
			URL += FString::Printf(TEXT("?"));
			URL += QueryParams;
		}
		return Request<FGameSessionSearchResult>(URL, EHttpRequestVerbs::VERB_GET, Callback);
	}

	bool RequestUpdateSessionState(const FGameSessionUpdateStateRequest& InRequest, const FUpdateSessionCallback& Callback)
	{
		if (InRequest.SessionId.IsEmpty()) return false;

		FString URL = FString::Printf(TEXT("/session/%s/state"), *InRequest.SessionId);
		return Request<FGameSessionUpdateStateRequest, FGameSessionUpdateReply>(URL, EHttpRequestVerbs::VERB_PUT, InRequest, Callback);
	}

	bool RequestUpdateSessionDetails(const FGameSessionDetails& InRequest, const FUpdateSessionCallback& Callback)
	{
		if (InRequest.SessionId.IsEmpty()) return false;
		FString URL = FString::Printf(TEXT("/session/%s"), *InRequest.SessionId);
		return Request<FGameSessionDetails, FGameSessionUpdateReply>(URL, EHttpRequestVerbs::VERB_PUT, InRequest, Callback);
	}

	bool RequestOneGameSession(const FString& QueryParams, const FFindSessionCallback& Callback)
	{
		FString URL = "/gamesession/request";
		if (!QueryParams.IsEmpty())
		{
			URL += FString::Printf(TEXT("?"));
			URL += QueryParams;
		}
		return Request<FGameSessionSearchResult>(URL, EHttpRequestVerbs::VERB_GET, Callback);
	}

	bool RequestGameSessionFind(const FString& QueryParams, const FFindSessionCallback& Callback)
	{
		FString URL = "/gamesession/find";
		if (!QueryParams.IsEmpty())
		{
			URL += FString::Printf(TEXT("?"));
			URL += QueryParams;
		}
		return Request<FGameSessionSearchResult>(URL, EHttpRequestVerbs::VERB_GET, Callback);
	}

	/**
	 * Request is a Struct
	 */
	template <typename RequestStructType, typename ReplyStructType>
	bool Request(FString RelativeURL, EHttpRequestVerbs Verb, const RequestStructType& InRequest, const TFunction<void(ReplyStructType&, bool)>& Callback)
	{
		FString RequestJsonString;
		if (!FJsonObjectConverter::UStructToJsonObjectString(InRequest, RequestJsonString))
		{
			UE_LOG(LogDSMaster, Warning, TEXT("HttpClient - Request, invalid json : %s"), *RequestJsonString);
			return false;
		}

		auto HttpRequest = CreateHttpRequest(RelativeURL, Verb);
		HttpRequest->SetContentAsString(RequestJsonString);
		HttpRequest->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
		{
			ReplyStructType OutReply;
			if (bSucceeded && FJsonObjectConverter::JsonObjectStringToUStruct(HttpResponse->GetContentAsString(), &OutReply))
			{
				Callback(OutReply, true);
			}
			else
			{
				Callback(OutReply, false);
			}
		});

		return HttpRequest->ProcessRequest();
	}

	/**
	 *	Request is a Verb
	 */
	template <typename ReplyStructType>
	bool Request(FString RelativeURL, EHttpRequestVerbs Verb, const TFunction<void(ReplyStructType&, bool)>& Callback)
	{
		auto HttpRequest = CreateHttpRequest(RelativeURL, Verb);
		HttpRequest->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr HttpRequest, FHttpResponsePtr HttpResponse, bool bSucceeded)
		{
			ReplyStructType OutReply;
			if (bSucceeded && FJsonObjectConverter::JsonObjectStringToUStruct(HttpResponse->GetContentAsString(), &OutReply))
			{
				Callback(OutReply, true);
			}
			else
			{
				Callback(OutReply, false);
			}
		});

		return HttpRequest->ProcessRequest();
	}

	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateHttpRequest(FString RelativeURL, EHttpRequestVerbs Verb = EHttpRequestVerbs::VERB_GET) const;

protected:
	/** Session Services Base URL */
	FString ServiceURL;
};
