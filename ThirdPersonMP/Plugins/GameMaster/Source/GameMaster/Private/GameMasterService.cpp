// Fill out your copyright notice in the Description page of Project Settings.


#include "GameMasterService.h"

#include "GameMaster.h"

// @formatter:off
void FGameMasterService::RegisterRoutes()
{
	FTinyHttpService::RegisterRoutes();

	RegisterRoute({TEXT("test info"),
		FHttpPath(TEXT("/test")),
		EHttpServerRequestVerbs::VERB_GET,
		FHttpServiceHandler::CreateLambda([](const FHttpServerRequest& Request, const FHttpResultCallback& OnComplete)
		{
			FTinyHttp::DumpServerRequest(Request);
			
			auto Response = FHttpServerResponse::Ok();
			OnComplete(MoveTemp(Response));
			return true;
		})});
	
}
// @formatter:on
