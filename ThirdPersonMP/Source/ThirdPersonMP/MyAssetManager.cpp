// Fill out your copyright notice in the Description page of Project Settings.


#include "MyAssetManager.h"


void UMyAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
}

void UMyAssetManager::FinishInitialLoading()
{
	Super::FinishInitialLoading();
}

bool UMyObject::SaveAsAsset(FString AssetPath, FString PackageName, FString ObjectName)
{
	return true;
}

UObject* UMyAssetUtility::DebugLoadObject(FString ObjectPath)
{
	UObject* Object = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
	if (Object)
	{
		UE_LOG(LogTemp, Warning, TEXT("loaded:%s"), *ObjectPath);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid Path:%s"), *ObjectPath);
	}

	return Object;
}

// UTexture2D::CreateTransient
UTexture2D* UMyAssetUtility::GenerateDebugTexture(FString TextureName, FString PackageName, int32 TextureWidth, int32 TextureHeight, FColor Color)
{
	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UTexture2D* NewTexture = NewObject<UTexture2D>(Package, *TextureName, RF_Public | RF_Standalone | RF_MarkAsRootSet);
	NewTexture->AddToRoot(); // This line prevents garbage collection of the texture

	NewTexture->PlatformData = new FTexturePlatformData();
	NewTexture->PlatformData->SizeX = TextureWidth;
	NewTexture->PlatformData->SizeY = TextureHeight;
	NewTexture->PlatformData->SetNumSlices(1);
	NewTexture->PlatformData->PixelFormat = EPixelFormat::PF_B8G8R8A8;

	// Fill Pixel
	uint8* Pixels = new uint8[TextureWidth * TextureHeight * 4];
	for (int32 y = 0; y < TextureHeight; y++)
	{
		for (int32 x = 0; x < TextureWidth; x++)
		{
			int32 curPixelIndex = ((y * TextureWidth) + x);
			Pixels[4 * curPixelIndex] = Color.B;
			Pixels[4 * curPixelIndex + 1] = Color.G;
			Pixels[4 * curPixelIndex + 2] = Color.R;
			Pixels[4 * curPixelIndex + 3] = Color.A;
		}
	}

	// Allocate first mipmap.
	FTexture2DMipMap* Mip = new FTexture2DMipMap();
	NewTexture->PlatformData->Mips.Add(Mip);
	Mip->SizeX = TextureWidth;
	Mip->SizeY = TextureHeight;

	Mip->BulkData.Lock(LOCK_READ_WRITE);
	uint8* TextureData = (uint8*)Mip->BulkData.Realloc(TextureWidth * TextureHeight * 4);
	FMemory::Memcpy(TextureData, Pixels, sizeof(uint8) * TextureHeight * TextureWidth * 4);
	Mip->BulkData.Unlock();

#if WITH_EDITORONLY_DATA
	NewTexture->Source.Init(TextureWidth, TextureHeight, 1, 1, ETextureSourceFormat::TSF_BGRA8, Pixels);
	NewTexture->UpdateResource();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	FString PackageFileName = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	bool bSaved = UPackage::SavePackage(Package, NewTexture, EObjectFlags::RF_Public | EObjectFlags::RF_Standalone, *PackageFileName, GError, nullptr, true, true, SAVE_NoError);
#endif

	delete[] Pixels; // Don't forget to free the memory here

	return NewTexture;
}

void UMyAssetUtility::DumpLinkerLoad(UPackage* Package)
{
	if (!Package) return;

	UE_LOG(LogTemp, Display, TEXT("====== Package : %s ====="), *Package->GetName());

	UE_LOG(LogTemp, Display, TEXT("Import---------"));

	for (int Idx = 0; Idx < Package->LinkerLoad->ImportMap.Num(); Idx++)
	{
		FObjectImport& Import = Package->LinkerLoad->ImportMap[Idx];

		UE_LOG(LogTemp, Display, TEXT("%d - %s, %s, %s | %s"), Idx, *Import.ObjectName.ToString(), *Import.ClassName.ToString(), *Import.ClassPackage.ToString(), *GetNameSafe(Import.XObject));
	}

	UE_LOG(LogTemp, Display, TEXT("Export---------"));
	for (int Idx = 0; Idx < Package->LinkerLoad->ExportMap.Num(); Idx++)
	{
		FObjectExport& Export = Package->LinkerLoad->ExportMap[Idx];
		UE_LOG(LogTemp, Display, TEXT("%d - %s, %d | %s"), Idx, *Export.ObjectName.ToString(), Export.ClassIndex.ForDebugging(), *GetNameSafe(Export.Object));


		if (Idx < Package->LinkerLoad->DependsMap.Num())
		{
			for (auto PackageIdx : Package->LinkerLoad->DependsMap[Idx])
			{
				UE_LOG(LogTemp, Display, TEXT(" - Depends on: %d"), PackageIdx.ForDebugging());
			}
		}
	}
}

void UMyAssetUtility::DumpObjectDependencies(FString ObjectPath)
{
	IAssetRegistry& AssetRegistry = UMyAssetManager::Get().GetAssetRegistry();

	FAssetData AssetData = AssetRegistry.GetAssetByObjectPath(FName(ObjectPath));
	if (AssetData.IsValid())
	{
		FAssetIdentifier AssetIdentifier(AssetData.PackageName);
		TArray<FAssetIdentifier> OutDependencies;
		AssetRegistry.GetDependencies(AssetIdentifier, OutDependencies);

		TArray<FAssetIdentifier> OutReferencers;
		AssetRegistry.GetReferencers(AssetIdentifier, OutReferencers);

		for (auto AssetID : OutDependencies)
		{
			UE_LOG(LogTemp, Display, TEXT("%s - Depends on: %s"), *ObjectPath, *AssetID.ToString());

			TArray<FAssetData> AssetDatas;
			AssetRegistry.GetAssetsByPackageName(AssetID.PackageName, AssetDatas);
		}

		for (auto AssetID : OutReferencers)
		{
			UE_LOG(LogTemp, Display, TEXT("%s - Reference: %s"), *ObjectPath, *AssetID.ToString());

			TArray<FAssetData> AssetDatas;
			AssetRegistry.GetAssetsByPackageName(AssetID.PackageName, AssetDatas);
		}

		// Depends Details
		UE::AssetRegistry::EDependencyCategory Categories = UE::AssetRegistry::EDependencyCategory::All;
		UE::AssetRegistry::EDependencyQuery Flags = UE::AssetRegistry::EDependencyQuery::NoRequirements;
		bool bReferencers = false;
		TArray<FAssetDependency> LinksToAsset;
		{
			LinksToAsset.Reset();
			if (bReferencers)
			{
				AssetRegistry.GetReferencers(AssetIdentifier, LinksToAsset, Categories, Flags);
			}
			else
			{
				AssetRegistry.GetDependencies(AssetIdentifier, LinksToAsset, Categories, Flags);
			}

			for (FAssetDependency LinkToAsset : LinksToAsset)
			{
				FString RefType;
				if (EnumHasAnyFlags(LinkToAsset.Properties, UE::AssetRegistry::EDependencyProperty::Hard))
				{
					RefType = TEXT("HardRef");
				}
				else
				{
					RefType = TEXT("SoftRef");
				}
				UE_LOG(LogTemp, Display, TEXT("%s - Dependency: %s (%s)"), *ObjectPath, *LinkToAsset.AssetId.ToString(), *RefType);
			}
		}
	}
}
