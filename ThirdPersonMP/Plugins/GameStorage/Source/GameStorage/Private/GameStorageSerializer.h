#pragma once
#include "GameStorageTypes.h"
#include "Serialization/BufferArchive.h"

class IGameStorageSerializer;
typedef TSharedPtr<IGameStorageSerializer> IGameStorageSerializerPtr;

class IGameStorageSerializer
{
public:
	// factory method
	static IGameStorageSerializerPtr Create(EGameStorageSerializerType SerializerType);

public:
	virtual ~IGameStorageSerializer();

	virtual bool SaveObject(UObject* Object, TArray<uint8>& OutData) = 0;
	virtual bool LoadObject(UObject* Object, const TArray<uint8>& InData) = 0;
};

class FGameStorageSerializer_None : public IGameStorageSerializer
{
public:
	virtual bool SaveObject(UObject* Object, TArray<uint8>& OutData) override;
	virtual bool LoadObject(UObject* Object, const TArray<uint8>& InData) override;
};


class FGameStorageSerializer_Json : public IGameStorageSerializer
{
public:
	virtual bool SaveObject(UObject* Object, TArray<uint8>& OutData) override;
	virtual bool LoadObject(UObject* Object, const TArray<uint8>& InData) override;
};

class FGameStorageSerializer_SaveGame : public IGameStorageSerializer
{
public:
	virtual bool SaveObject(UObject* Object, TArray<uint8>& OutData) override;
	virtual bool LoadObject(UObject* Object, const TArray<uint8>& InData) override;
};

class FGameStorageSerializer
{
public:
	static bool SaveObjectToJson(UObject* Object, FBufferArchive& Archive);
	static bool LoadObjectFromJson(UObject* Object, const TArray<uint8>& JsonBytes);

	static bool SaveObjectToSav(UObject* Object, TArray<uint8>& OutSaveData);
	static bool LoadGameFromSav(UObject* Object, const TArray<uint8>& InSaveData);
};
