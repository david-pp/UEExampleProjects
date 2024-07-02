// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"

/**
 * Holds serialized message data.
 */
class FNatsSerializedMessage : public FMemoryWriter
{
public:
	/** Default constructor. */
	FNatsSerializedMessage()
		: FMemoryWriter(DataArray, true)
	{
	}

public:
	/**
	 * Creates an archive reader to the data.
	 *
	 * The caller is responsible for deleting the returned object.
	 *
	 * @return An archive reader.
	 */
	FArchive* CreateReader()
	{
		return new FMemoryReader(DataArray, true);
	}

	/**
	 * Get the serialized message data.
	 *
	 * @return Byte array of message data.
	 * @see GetState
	 */
	const TArray<uint8>& GetDataArray()
	{
		return DataArray;
	}

private:
	/** Holds the serialized data. */
	TArray<uint8> DataArray;
};


/** Type definition for shared pointers to instances of FNatsSerializedMessage. */
typedef TSharedPtr<FNatsSerializedMessage, ESPMode::ThreadSafe> FNatsSerializedMessagePtr;

/** Type definition for shared references to instances of FNatsSerializedMessage. */
typedef TSharedRef<FNatsSerializedMessage, ESPMode::ThreadSafe> FNatsSerializedMessageRef;
