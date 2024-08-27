#pragma once
#include "Serialization/BufferArchive.h"

class FGameStorageSerialization
{
public:
	static bool SaveObjectToJson(UObject* Object, FBufferArchive& Archive);
	static bool LoadObjectFromJson(UObject* Object, const TArray<uint8>& JsonBytes);

	static bool SaveObjectToSav(UObject* Object, TArray<uint8>& OutSaveData);
	static bool LoadGameFromSav(UObject* Object, const TArray<uint8>& InSaveData);
};
