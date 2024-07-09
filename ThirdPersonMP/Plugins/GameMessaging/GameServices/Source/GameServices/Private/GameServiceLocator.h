// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"

class FTypeContainer;
class IGameServiceLocator;

class FGameServiceLocatorFactory
{
public:
	static TSharedRef<IGameServiceLocator> Create(const TSharedRef<FTypeContainer>& ServiceDependencies);
};
