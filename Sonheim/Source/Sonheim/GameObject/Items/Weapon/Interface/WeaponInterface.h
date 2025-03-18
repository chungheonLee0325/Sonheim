// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "WeaponInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class UWeaponInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SONHEIM_API IWeaponInterface
{
	GENERATED_BODY()

	void HandleLeftButtonPressed();
	void HandleRightButtonPressed();
public:
};
