// Fill out your copyright notice in the Description page of Project Settings.


#include "OnlineSessionHelper.h"

#include "OnlineSubsystemRPGTypes.h"
#include "SocketSubsystem.h"

void FOnlineSessionHelper::SetupHttpSessionDetails(FGameSessionDetails& SessionDetails, FNamedOnlineSession* OnlineSession)
{
	// FOnlineSessionSettings -> Settings
	SessionDetails.Settings.NumPublicConnections = OnlineSession->SessionSettings.NumPublicConnections;
	SessionDetails.Settings.PermissionLevel = 0;
	SessionDetails.Settings.bInvitesAllowed = OnlineSession->SessionSettings.bAllowInvites;

	// Current server process Info
	FParse::Value(FCommandLine::Get(), TEXT("ServerGuid="), SessionDetails.ServerGuid);
	FParse::Value(FCommandLine::Get(), TEXT("BucketId="), SessionDetails.Settings.BucketId);
	SessionDetails.ServerPID = FPlatformProcess::GetCurrentProcessId();

	// Session State
	SessionDetails.SetSessionState(OnlineSession->SessionState);

	// FSessionSettings -> Attribute
	SessionDetails.Attributes.Empty();
	for (FSessionSettings::TConstIterator It(OnlineSession->SessionSettings.Settings); It; ++It)
	{
		FName Key = It.Key();
		const FOnlineSessionSetting& Setting = It.Value();

		FGameSessionAttribute Attribute;
		Attribute.Key = Key.ToString();
		Attribute.ValueType = Setting.Data.GetTypeString();
		Attribute.Value = Setting.Data.ToString();

		SessionDetails.Attributes.Add(Attribute);
	}

	TSharedPtr<FOnlineSessionInfoRPG> SessionInfo = StaticCastSharedPtr<FOnlineSessionInfoRPG>(OnlineSession->SessionInfo);
	if (SessionInfo)
	{
		SessionDetails.HostAddress = SessionInfo->HostAddr->ToString(true);
	}
}

void FOnlineSessionHelper::SetupOnlineSession(FOnlineSession* OnlineSession, const FGameSessionDetails& SessionDetails)
{
	FOnlineSessionSettings& OnlineSettings = OnlineSession->SessionSettings;

	// Settings -> FOnlineSessionSettings
	OnlineSettings.NumPublicConnections = SessionDetails.Settings.NumPublicConnections;
	OnlineSettings.bAllowInvites = SessionDetails.Settings.bInvitesAllowed;

	// Attributes -> FOnlineSessionSettings.Settings
	for (auto& Attribute : SessionDetails.Attributes)
	{
		FOnlineSessionSetting AttributeSetting;

		const FString& NewValue = Attribute.Value;
		EOnlineKeyValuePairDataType::Type ValueType = EOnlineKeyValuePairDataType::FromString(Attribute.ValueType);

		switch (ValueType)
		{
		case EOnlineKeyValuePairDataType::Float:
			{
				// Convert the string to a float
				float FloatVal = FCString::Atof(*NewValue);
				AttributeSetting.Data.SetValue(FloatVal);
				break;
			}
		case EOnlineKeyValuePairDataType::Int32:
			{
				// Convert the string to a int
				int32 IntVal = FCString::Atoi(*NewValue);
				AttributeSetting.Data.SetValue(IntVal);
				break;
			}
		case EOnlineKeyValuePairDataType::UInt32:
			{
				// Convert the string to a int
				uint64 IntVal = FCString::Strtoui64(*NewValue, nullptr, 10);
				AttributeSetting.Data.SetValue(static_cast<uint32>(IntVal));
				break;
			}
		case EOnlineKeyValuePairDataType::Double:
			{
				// Convert the string to a double
				double Val = FCString::Atod(*NewValue);
				AttributeSetting.Data.SetValue(Val);
				break;
			}
		case EOnlineKeyValuePairDataType::Int64:
			{
				int64 Val = FCString::Atoi64(*NewValue);
				AttributeSetting.Data.SetValue(Val);
				break;
			}
		case EOnlineKeyValuePairDataType::UInt64:
			{
				uint64 Val = FCString::Strtoui64(*NewValue, nullptr, 10);
				AttributeSetting.Data.SetValue(Val);
				break;
			}
		case EOnlineKeyValuePairDataType::String:
			{
				// Copy the string
				AttributeSetting.Data.SetValue(NewValue);
				break;
			}
		case EOnlineKeyValuePairDataType::Bool:
			{
				bool Val = NewValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) ? true : false;
				AttributeSetting.Data.SetValue(Val);
				break;
			}
		case EOnlineKeyValuePairDataType::Blob:
		case EOnlineKeyValuePairDataType::Empty: break;
		}

		OnlineSettings.Set(FName(Attribute.Key), AttributeSetting);
	}

	// Create a New Session Info & Update It
	TSharedPtr<FOnlineSessionInfoRPG> SessionInfoRPG = MakeShared<FOnlineSessionInfoRPG>();
	if (SessionInfoRPG)
	{
		SessionInfoRPG->SessionId = FUniqueNetIdRPG(SessionDetails.SessionId);

		SessionInfoRPG->HostAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		bool IsValid = false;
		SessionInfoRPG->HostAddr->SetIp(*SessionDetails.HostAddress, IsValid);

		OnlineSession->SessionInfo = SessionInfoRPG;
	}
}

FString FOnlineSessionHelper::SessionSearchToQueryParams(const FOnlineSessionSearch* SearchSettings)
{
	FString Params;
	if (SearchSettings)
	{
		if (SearchSettings->MaxSearchResults > 0)
		{
			Params += FString::Printf(TEXT("MaxSearchResults=%d&"), SearchSettings->MaxSearchResults);
		}

		if (SearchSettings->QuerySettings.SearchParams.Num() > 0)
		{
			for (FSearchParams::TConstIterator It(SearchSettings->QuerySettings.SearchParams); It; ++It)
			{
				const FName Key = It.Key();
				const FOnlineSessionSearchParam& SearchParam = It.Value();
				Params += FString::Printf(TEXT("%s=%s&"), *Key.ToString(), *SearchParam.Data.ToString());
			}
		}
	}

	return Params;
}
