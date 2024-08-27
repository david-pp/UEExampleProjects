#include "GameStorageSerialization.h"

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
