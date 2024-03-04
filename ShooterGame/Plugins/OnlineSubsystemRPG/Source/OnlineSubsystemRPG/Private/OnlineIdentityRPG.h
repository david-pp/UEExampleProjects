// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/CoreOnline.h"
#include "OnlineSubsystemTypes.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "OnlineSubsystemRPGTypes.h"

class FOnlineSubsystemRPG;

/**
 * Info associated with an user account generated by this online service
 */
class FUserOnlineAccountRPG : 
	public FUserOnlineAccount
{

public:

	// FOnlineUser
	
	virtual FUniqueNetIdRef GetUserId() const override { return UserIdPtr; }
	virtual FString GetRealName() const override { return TEXT("DummyRealName"); }
	virtual FString GetDisplayName(const FString& Platform = FString()) const override  { return TEXT("DummyDisplayName"); }
	virtual bool GetUserAttribute(const FString& AttrName, FString& OutAttrValue) const override;
	virtual bool SetUserAttribute(const FString& AttrName, const FString& AttrValue) override;

	// FUserOnlineAccount

	virtual FString GetAccessToken() const override { return TEXT("DummyAuthTicket"); }
	virtual bool GetAuthAttribute(const FString& AttrName, FString& OutAttrValue) const override;

	// FUserOnlineAccountRPG

	FUserOnlineAccountRPG(const FString& InUserId=TEXT("")) 
		: UserIdPtr(new FUniqueNetIdRPG(InUserId))
	{ }

	virtual ~FUserOnlineAccountRPG()
	{
	}

	/** User Id represented as a FUniqueNetId */
	FUniqueNetIdRef UserIdPtr;

        /** Additional key/value pair data related to auth */
	TMap<FString, FString> AdditionalAuthData;
        /** Additional key/value pair data related to user attribution */
	TMap<FString, FString> UserAttributes;
};

/**
 * RPG service implementation of the online identity interface
 */
class FOnlineIdentityRPG : public IOnlineIdentity
{
public:

	// IOnlineIdentity

	virtual bool Login(int32 LocalUserNum, const FOnlineAccountCredentials& AccountCredentials) override;
	virtual bool Logout(int32 LocalUserNum) override;
	virtual bool AutoLogin(int32 LocalUserNum) override;
	virtual TSharedPtr<FUserOnlineAccount> GetUserAccount(const FUniqueNetId& UserId) const override;
	virtual TArray<TSharedPtr<FUserOnlineAccount> > GetAllUserAccounts() const override;
	virtual FUniqueNetIdPtr GetUniquePlayerId(int32 LocalUserNum) const override;
	virtual FUniqueNetIdPtr CreateUniquePlayerId(uint8* Bytes, int32 Size) override;
	virtual FUniqueNetIdPtr CreateUniquePlayerId(const FString& Str) override;
	virtual ELoginStatus::Type GetLoginStatus(int32 LocalUserNum) const override;
	virtual ELoginStatus::Type GetLoginStatus(const FUniqueNetId& UserId) const override;
	virtual FString GetPlayerNickname(int32 LocalUserNum) const override;
	virtual FString GetPlayerNickname(const FUniqueNetId& UserId) const override;
	virtual FString GetAuthToken(int32 LocalUserNum) const override;
	virtual void RevokeAuthToken(const FUniqueNetId& UserId, const FOnRevokeAuthTokenCompleteDelegate& Delegate) override;
	virtual void GetUserPrivilege(const FUniqueNetId& UserId, EUserPrivileges::Type Privilege, const FOnGetUserPrivilegeCompleteDelegate& Delegate) override;
	virtual FPlatformUserId GetPlatformUserIdFromUniqueNetId(const FUniqueNetId& UniqueNetId) const override;
	virtual FString GetAuthType() const override;

	// FOnlineIdentityRPG

	/**
	 * Constructor
	 *
	 * @param InSubsystem online subsystem being used
	 */
	FOnlineIdentityRPG(FOnlineSubsystemRPG* InSubsystem);

	/**
	 * Destructor
	 */
	virtual ~FOnlineIdentityRPG();

private:

	/**
	 * Should use the initialization constructor instead
	 */
	FOnlineIdentityRPG() = delete;

	/** Cached pointer to owning subsystem */
	FOnlineSubsystemRPG* RPGSubsystem;

	/** Ids mapped to locally registered users */
	TMap<int32, FUniqueNetIdPtr> UserIds;

	/** Ids mapped to locally registered users */
	TMap<FUniqueNetIdRPG, TSharedRef<FUserOnlineAccountRPG>> UserAccounts;
};

typedef TSharedPtr<FOnlineIdentityRPG, ESPMode::ThreadSafe> FOnlineIdentityRPGPtr;
