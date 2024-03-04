// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DSMasterTypes.generated.h"

UENUM()
enum class EDSMasterMode : int8
{
	None,
	Manager,
	Agent,
	/** Run as Manager and Agent */
	AllInOne,
};
