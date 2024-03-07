// Fill out your copyright notice in the Description page of Project Settings.

#include "Session/GameSessionTypes.h"

void FRPGGameSessionDetails::SetupSettingsByOnlineSettings(const FOnlineSessionSettings& OnlineSettings)
{
	Settings.NumPublicConnections = OnlineSettings.NumPublicConnections;
	Settings.PermissionLevel = 0;
	Settings.bInvitesAllowed = OnlineSettings.bAllowInvites;
}

void FRPGGameSessionDetails::SetupAttributesByOnlineSettings(const FOnlineSessionSettings& OnlineSettings)
{
	Attributes.Empty();
	for (FSessionSettings::TConstIterator It(OnlineSettings.Settings); It; ++It)
	{
		FName Key = It.Key();
		const FOnlineSessionSetting& Setting = It.Value();

		FRPGGameSessionAttribute Attribute;
		Attribute.Key = Key.ToString();
		Attribute.ValueType = Setting.Data.GetTypeString();
		Attribute.Value = Setting.Data.ToString();

		Attributes.Add(Attribute);
	}
}

void FRPGGameSessionDetails::SetupOnlineSessionSettings(FOnlineSessionSettings& OnlineSettings) const
{
	// Settings -> FOnlineSessionSettings
	OnlineSettings.NumPublicConnections = Settings.NumPublicConnections;
	OnlineSettings.bAllowInvites = Settings.bInvitesAllowed;

	// Attributes -> FOnlineSessionSettings.Settings
	for (auto& Attribute : Attributes)
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
}

void FActiveRPGGameSession::SetupFromNamedOnlineSession(FNamedOnlineSession* OnlineSession)
{
	if (!OnlineSession) return;

	SessionDetails.SetupSettingsByOnlineSettings(OnlineSession->SessionSettings);
	SessionDetails.SetupAttributesByOnlineSettings(OnlineSession->SessionSettings);
}

void FActiveRPGGameSession::SetupToNamedOnlineSession(FNamedOnlineSession* OnlineSession)
{
	if (!OnlineSession) return;

	SessionDetails.SetupOnlineSessionSettings(OnlineSession->SessionSettings);
}

void FActiveRPGGameSession::SetupByOnlineSettings(const FOnlineSessionSettings& Settings)
{
	SessionDetails.SetupSettingsByOnlineSettings(Settings);
	SessionDetails.SetupAttributesByOnlineSettings(Settings);
}

TSharedPtr<FOnlineSessionSettings> FActiveRPGGameSession::CreateOnlineSessionSettings() const
{
	TSharedPtr<class FOnlineSessionSettings> Settings = MakeShareable(new FOnlineSessionSettings);
	// ShooterHostSettings->Set(SETTING_MATCHING_HOPPER, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::DontAdvertise);
	// ShooterHostSettings->Set(SETTING_MATCHING_TIMEOUT, 120.0f, EOnlineDataAdvertisementType::ViaOnlineService);
	// ShooterHostSettings->Set(SETTING_SESSION_TEMPLATE_NAME, FString("GameSession"), EOnlineDataAdvertisementType::DontAdvertise);
	// ShooterHostSettings->Set(SETTING_GAMEMODE, FString("TeamDeathmatch"), EOnlineDataAdvertisementType::ViaOnlineService);
	// ShooterHostSettings->Set(SETTING_MAPNAME, GetWorld()->GetMapName(), EOnlineDataAdvertisementType::ViaOnlineService);
	// ShooterHostSettings->bAllowInvites = true;
	// ShooterHostSettings->bIsDedicated = true;

	SessionDetails.SetupOnlineSessionSettings(*Settings);
	return Settings;
}
