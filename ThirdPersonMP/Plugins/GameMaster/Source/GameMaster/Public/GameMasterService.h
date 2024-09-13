// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

// #include "CoreMinimal.h"
#include "TinyHttp.h"
// #include "UObject/Object.h"
// #include "GameMasterService.generated.h"

/**
 * 
 */
class FGameMasterService  : public FTinyHttpService
{
public:
	using FTinyHttpService::FTinyHttpService;
	virtual void RegisterRoutes() override;
};
