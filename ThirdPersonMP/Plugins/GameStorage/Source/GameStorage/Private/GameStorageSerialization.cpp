#include "GameStorageSerialization.h"

#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "Serialization/Formatters/JsonArchiveInputFormatter.h"
#include "Serialization/Formatters/JsonArchiveOutputFormatter.h"

bool FGameStorageSerialization::SaveObjectToJson(UObject* Object, FBufferArchive& Archive)
{
	if (!Object) return false;

	FJsonArchiveOutputFormatter Formatter(Archive);
	FStructuredArchive StructuredArchive(Formatter);
	FStructuredArchiveRecord RootRecord = StructuredArchive.Open().EnterRecord();
	Object->Serialize(RootRecord);
	StructuredArchive.Close();

	return true;
}

bool FGameStorageSerialization::LoadObjectFromJson(UObject* Object, const TArray<uint8>& JsonBytes)
{
	if (!Object) return false;

	FMemoryReader BinAr(JsonBytes, true);
	FJsonArchiveInputFormatter Formatter(BinAr);
	FStructuredArchive StructuredArchive(Formatter);
	Object->Serialize(StructuredArchive.Open().EnterRecord());
	StructuredArchive.Close();

	return true;
}


//////////////////////////////////////////////////////////////////////////
// FSaveObjectHeader

static const int UE4_SAVEGAME_FILE_TYPE_TAG = 0x53415647; // "SAVG"

struct FSaveGameFileVersion
{
	enum Type
	{
		InitialVersion = 1,
		// serializing custom versions into the savegame data to handle that type of versioning
		AddedCustomVersions = 2,

		// -----<new versions can be added above this line>-------------------------------------------------
		VersionPlusOne,
		LatestVersion = VersionPlusOne - 1
	};
};

struct FSaveObjectHeader
{
	FSaveObjectHeader();
	FSaveObjectHeader(TSubclassOf<UObject> ObjectType);

	void Empty();
	bool IsEmpty() const;

	void Read(FMemoryReader& MemoryReader);
	void Write(FMemoryWriter& MemoryWriter);

	int32 FileTypeTag;
	int32 SaveGameFileVersion;
	int32 PackageFileUE4Version;
	FEngineVersion SavedEngineVersion;
	int32 CustomVersionFormat;
	FCustomVersionContainer CustomVersions;
	FString SaveGameClassName;
};

FSaveObjectHeader::FSaveObjectHeader()
	: FileTypeTag(0), SaveGameFileVersion(0), PackageFileUE4Version(0), CustomVersionFormat(static_cast<int32>(ECustomVersionSerializationFormat::Unknown))
{
}

FSaveObjectHeader::FSaveObjectHeader(TSubclassOf<UObject> ObjectType)
	: FileTypeTag(UE4_SAVEGAME_FILE_TYPE_TAG), SaveGameFileVersion(FSaveGameFileVersion::LatestVersion), PackageFileUE4Version(GPackageFileUE4Version), SavedEngineVersion(FEngineVersion::Current()), CustomVersionFormat(static_cast<int32>(ECustomVersionSerializationFormat::Latest)), CustomVersions(FCurrentCustomVersions::GetAll()), SaveGameClassName(ObjectType->GetPathName())
{
}

void FSaveObjectHeader::Empty()
{
	FileTypeTag = 0;
	SaveGameFileVersion = 0;
	PackageFileUE4Version = 0;
	SavedEngineVersion.Empty();
	CustomVersionFormat = (int32)ECustomVersionSerializationFormat::Unknown;
	CustomVersions.Empty();
	SaveGameClassName.Empty();
}

bool FSaveObjectHeader::IsEmpty() const
{
	return (FileTypeTag == 0);
}

void FSaveObjectHeader::Read(FMemoryReader& MemoryReader)
{
	Empty();

	MemoryReader << FileTypeTag;

	if (FileTypeTag != UE4_SAVEGAME_FILE_TYPE_TAG)
	{
		// this is an old saved game, back up the file pointer to the beginning and assume version 1
		MemoryReader.Seek(0);
		SaveGameFileVersion = FSaveGameFileVersion::InitialVersion;

		// Note for 4.8 and beyond: if you get a crash loading a pre-4.8 version of your savegame file and 
		// you don't want to delete it, try uncommenting these lines and changing them to use the version 
		// information from your previous build. Then load and resave your savegame file.
		//MemoryReader.SetUE4Ver(MyPreviousUE4Version);				// @see GPackageFileUE4Version
		//MemoryReader.SetEngineVer(MyPreviousEngineVersion);		// @see FEngineVersion::Current()
	}
	else
	{
		// Read version for this file format
		MemoryReader << SaveGameFileVersion;

		// Read engine and UE4 version information
		MemoryReader << PackageFileUE4Version;

		MemoryReader << SavedEngineVersion;

		MemoryReader.SetUE4Ver(PackageFileUE4Version);
		MemoryReader.SetEngineVer(SavedEngineVersion);

		if (SaveGameFileVersion >= FSaveGameFileVersion::AddedCustomVersions)
		{
			MemoryReader << CustomVersionFormat;

			CustomVersions.Serialize(MemoryReader, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));
			MemoryReader.SetCustomVersions(CustomVersions);
		}
	}

	// Get the class name
	MemoryReader << SaveGameClassName;
}

void FSaveObjectHeader::Write(FMemoryWriter& MemoryWriter)
{
	// write file type tag. identifies this file type and indicates it's using proper versioning
	// since older UE4 versions did not version this data.
	MemoryWriter << FileTypeTag;

	// Write version for this file format
	MemoryWriter << SaveGameFileVersion;

	// Write out engine and UE4 version information
	MemoryWriter << PackageFileUE4Version;
	MemoryWriter << SavedEngineVersion;

	// Write out custom version data
	MemoryWriter << CustomVersionFormat;
	CustomVersions.Serialize(MemoryWriter, static_cast<ECustomVersionSerializationFormat::Type>(CustomVersionFormat));

	// Write the class name so we know what class to load to
	MemoryWriter << SaveGameClassName;
}

bool FGameStorageSerialization::SaveObjectToSav(UObject* SaveGameObject, TArray<uint8>& OutSaveData)
{
	if (SaveGameObject)
	{
		FMemoryWriter MemoryWriter(OutSaveData, true);

		FSaveObjectHeader SaveHeader(SaveGameObject->GetClass());
		SaveHeader.Write(MemoryWriter);

		// Then save the object state, replacing object refs and names with strings
		FObjectAndNameAsStringProxyArchive Ar(MemoryWriter, false);
		SaveGameObject->Serialize(Ar);
		return true; // Not sure if there's a failure case here.
	}

	return false;
}

bool FGameStorageSerialization::LoadGameFromSav(UObject* Object, const TArray<uint8>& InSaveData)
{
	if (InSaveData.Num() == 0)
	{
		// Empty buffer, return instead of causing a bad serialize that could crash
		return false;
	}

	FMemoryReader MemoryReader(InSaveData, true);
	FSaveObjectHeader SaveHeader;
	SaveHeader.Read(MemoryReader);

	FObjectAndNameAsStringProxyArchive Ar(MemoryReader, true);
	Object->Serialize(Ar);
	return true;
}
