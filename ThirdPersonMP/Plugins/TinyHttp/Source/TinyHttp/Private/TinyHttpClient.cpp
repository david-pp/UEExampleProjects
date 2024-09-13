#include "TinyHttpClient.h"
#include "HttpModule.h"

FTinyHttpClient::FTinyHttpClient(const FString& InServiceURL) : ServiceURL(InServiceURL)
{
}

FTinyHttpClient::~FTinyHttpClient()
{
	Fini();
}

bool FTinyHttpClient::Init(const FString URL)
{
	ServiceURL = URL;
	return true;
}

void FTinyHttpClient::Fini()
{
	for (auto PendingRequest : PendingRequests)
	{
		if (PendingRequest->OnProcessRequestComplete().IsBound())
		{
			PendingRequest->OnProcessRequestComplete().Unbind();
		}
		PendingRequest->CancelRequest();
	}
	PendingRequests.Empty();
}

FHttpRequestRef FTinyHttpClient::CreateHttpRequest(FString RelativeURL, ETinyHttpRequestVerb Verb) const
{
	FHttpRequestRef HttpRequest = FHttpModule::Get().CreateRequest();
	FString URL = ServiceURL / RelativeURL;
	HttpRequest->SetURL(URL);
	HttpRequest->SetHeader("Content-Type", TEXT("application/json"));
	HttpRequest->SetVerb(LexToString(Verb));
	return HttpRequest;
}

