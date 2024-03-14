// Fill out your copyright notice in the Description page of Project Settings.

#include "DSMasterServiceClient.h"
#include "HttpModule.h"

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> FDSMasterHttpClient::CreateHttpRequest(FString RelativeURL, EHttpRequestVerbs Verb) const
{
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = FHttpModule::Get().CreateRequest();

	FString URL = ServiceURL / RelativeURL;
	HttpRequest->SetURL(URL);
	HttpRequest->SetHeader("Content-Type", TEXT("application/json"));

	switch (Verb)
	{
	case EHttpRequestVerbs::VERB_GET:
		{
			HttpRequest->SetVerb(TEXT("GET"));
		}
		break;
	case EHttpRequestVerbs::VERB_POST:
		{
			HttpRequest->SetVerb(TEXT("POST"));
		}
		break;
	case EHttpRequestVerbs::VERB_PUT:
		{
			HttpRequest->SetVerb(TEXT("PUT"));
		}
		break;
	case EHttpRequestVerbs::VERB_DELETE:
		{
			HttpRequest->SetVerb(TEXT("DELETE"));
		}
		break;
	default:
		{
			HttpRequest->SetVerb(TEXT("GET"));
		}
		break;
	}

	return HttpRequest;
}
