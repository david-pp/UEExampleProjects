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
			UE_LOG(LogTinyHttp, Warning, TEXT("test : \n%s"), *FTinyHttp::RequestToDebugString(Request));
			
			auto Response = FHttpServerResponse::Ok();
			OnComplete(MoveTemp(Response));
			return true;
		})});
	
}
// @formatter:on
