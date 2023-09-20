// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

/**
 * 
 */
class THIRDPERSONMP_API SMainHUDWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMainHUDWidget)
		{
		}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);
};
