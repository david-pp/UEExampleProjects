// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HttpModule.h"
#include "JsonObjectConverter.h"
#include "TinyHttp.h"
#include "TinyHttpTypes.h"
#include "Interfaces/IHttpRequest.h"

/**
 * A Http Request Builder
 */
class TINYHTTP_API FHttpRequestBuilder
{
public:
	FHttpRequestBuilder(const FString& InRootURL = TEXT("")) : RootURL(InRootURL)
	{
		Verb = ETinyHttpRequestVerb::VERB_GET;
	}

	~FHttpRequestBuilder()
	{
		Build();
	}

	//
	// Verb & URL
	// 
	FHttpRequestBuilder& Get(const FString& URL)
	{
		Verb = ETinyHttpRequestVerb::VERB_GET;
		ServiceURL = URL;
		return *this;
	}

	FHttpRequestBuilder& Post(const FString& URL)
	{
		Verb = ETinyHttpRequestVerb::VERB_POST;
		ServiceURL = URL;
		return *this;
	}

	FHttpRequestBuilder& Put(const FString& URL)
	{
		Verb = ETinyHttpRequestVerb::VERB_PUT;
		ServiceURL = URL;
		return *this;
	}

	FHttpRequestBuilder& Delete(const FString& URL)
	{
		Verb = ETinyHttpRequestVerb::VERB_DELETE;
		ServiceURL = URL;
		return *this;
	}

	//
	// Request Payload
	//
	template <typename RequestType>
	FHttpRequestBuilder& WithPayload(const RequestType& InRequest)
	{
		if (!FJsonObjectConverter::UStructToJsonObjectString(InRequest, ContentString))
		{
			UE_LOG(LogTinyHttp, Warning, TEXT("WithPayload to json failed"));
		}
		return *this;
	}

	FHttpRequestBuilder& WithStringPayload(const FString& InRequest)
	{
		ContentString = InRequest;
		return *this;
	}

	FHttpRequestBuilder& WithBytesPayload(TArray<uint8>&& InRequest)
	{
		ContentPayload = MoveTemp(InRequest);
		return *this;
	}

	//
	// Callbacks
	//
	template <typename ResponsePayloadType>
	FHttpRequestBuilder& Handling(const TFunction<void(ResponsePayloadType&, const FString& Error)>& Callback)
	{
		CompleteDelegate.BindLambda([Callback](FHttpRequestPtr, FHttpResponsePtr HttpResponse, bool bSucceeded)
		{
			FServiceResponse Response;
			ResponsePayloadType Payload;
			if (bSucceeded)
			{
				TSharedPtr<FJsonObject> JsonPayload = FTinyHttp::DeserializeServiceResponse(HttpResponse, Response);
				if (!Response.HasError() && JsonPayload)
				{
					if (!FJsonObjectConverter::JsonObjectToUStruct(JsonPayload.ToSharedRef(), &Payload))
					{
						Response.ErrorMessage = FString::Printf(TEXT("Json to struct failed"));
					}
				}
			}
			else
			{
				Response.ErrorMessage = TEXT("Network error");
			}

			Callback(Payload, Response.ErrorMessage);
		});
		return *this;
	}

	FHttpRequestBuilder& Handling(const TFunction<void(const FString& Error)>& Callback)
	{
		CompleteDelegate.BindLambda([Callback](FHttpRequestPtr, FHttpResponsePtr HttpResponse, bool bSucceeded)
		{
			FServiceResponse Response;
			if (bSucceeded)
			{
				FTinyHttp::DeserializeServiceResponse(HttpResponse, Response);
			}
			else
			{
				Response.ErrorMessage = TEXT("Network error");
			}

			Callback(Response.ErrorMessage);
		});
		return *this;
	}

	/**
	 * Build & Send the request
	 */
	void Build() const
	{
		FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
		if (RootURL.IsEmpty())
		{
			HttpRequest->SetURL(ServiceURL);
		}
		else
		{
			HttpRequest->SetURL(RootURL / ServiceURL);
		}

		HttpRequest->SetHeader("Content-Type", TEXT("application/json"));
		HttpRequest->SetVerb(LexToString(Verb));
		if (ContentString.Len() > 0)
		{
			HttpRequest->SetContentAsString(ContentString);
		}
		else if (ContentPayload.Num() > 0)
		{
			HttpRequest->SetContent(ContentPayload);
		}
		if (CompleteDelegate.IsBound())
		{
			HttpRequest->OnProcessRequestComplete() = CompleteDelegate;
		}
		HttpRequest->ProcessRequest();
	}

protected:
	ETinyHttpRequestVerb Verb;
	FString RootURL;
	FString ServiceURL;

	TArray<uint8> ContentPayload;
	FString ContentString;

	FHttpRequestCompleteDelegate CompleteDelegate;
};

/**
 * A tiny http client
 */
class TINYHTTP_API FTinyHttpClient
{
public:
	FTinyHttpClient(const FString& InServiceURL);
	virtual ~FTinyHttpClient();

	bool Init(const FString URL);
	void Fini();


	FHttpRequestRef CreateHttpRequest(FString RelativeURL, ETinyHttpRequestVerb Verb = ETinyHttpRequestVerb::VERB_GET) const;

protected:
	/** Services Base URL */
	FString ServiceURL;
	/** Pending requests */
	TSet<FHttpRequestRef> PendingRequests;
};

typedef TSharedPtr<FTinyHttpClient, ESPMode::ThreadSafe> FTinyHttpClientPtr;
