// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "OnlineSessionSettings.h"

/**
 * General session settings for a OnlineRPG game
 */
class FOnlineRPGOnlineSessionSettings : public FOnlineSessionSettings
{
public:

	FOnlineRPGOnlineSessionSettings(bool bIsLAN = false, bool bIsPresence = false, int32 MaxNumPlayers = 4);
	virtual ~FOnlineRPGOnlineSessionSettings() {}
};

/**
 * General search setting for a OnlineRPG game
 */
class FOnlineRPGOnlineSearchSettings : public FOnlineSessionSearch
{
public:
	FOnlineRPGOnlineSearchSettings(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FOnlineRPGOnlineSearchSettings() {}
};

/**
 * Search settings for an empty dedicated server to host a match
 */
class FOnlineRPGOnlineSearchSettingsEmptyDedicated : public FOnlineRPGOnlineSearchSettings
{
public:
	FOnlineRPGOnlineSearchSettingsEmptyDedicated(bool bSearchingLAN = false, bool bSearchingPresence = false);

	virtual ~FOnlineRPGOnlineSearchSettingsEmptyDedicated() {}
};
