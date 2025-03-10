// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Sonheim/ResourceManager/SonheimGameType.h"
#include "SonheimUtility.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USonheimUtility : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static float CalculateDamageMultiplier(EElementalAttribute DefenseAttribute, EElementalAttribute AttackAttribute);
};
