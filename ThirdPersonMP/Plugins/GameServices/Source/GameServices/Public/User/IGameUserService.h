// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Async/AsyncResult.h"
#include "IGameService.h"
#include "IGameUserService.generated.h"

/** 
 * The basic data for the last or currently signed-in user.
 */
USTRUCT()
struct FGameUserDetails
{
	GENERATED_USTRUCT_BODY()

	/** The users epic games account display name */
	UPROPERTY(EditAnywhere, Category = "User")
	FText DisplayName;

	/** The users epic games account email address */
	UPROPERTY(EditAnywhere, Category = "User")
	FText Email;

	/** The users real name attached to their epic games account */
	UPROPERTY(EditAnywhere, Category = "User")
	FText RealName;

	/** Whether this user is currently signed-in to the game */
	UPROPERTY(EditAnywhere, Category = "User")
	bool IsSignedIn;

	FGameUserDetails() : DisplayName(), Email(), RealName(), IsSignedIn(false)
	{
	}
};


/**
 * Interface for the Game application's user services.
 * Specializes in readonly requests for information about the last or currently signed-in user.
 */
class IGameUserService : public IGameService
{
public:
	/**
	 * Requests the details of the last or currently signed in user
	 */
	virtual TAsyncResult<FGameUserDetails> GetUserDetails() = 0;

public:
	virtual ~IGameUserService()
	{
	}
};

Expose_TNameOf(IGameUserService);
