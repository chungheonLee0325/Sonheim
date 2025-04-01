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

	static bool CheckMoveEnable(const UObject* WorldContextObject, const class AAreaObject* Caster, const class AAreaObject* Target, const FVector& StartLoc, FVector& EndLoc);

	// 개행 문자 처리를 위한 헬퍼 메서드
	static FText ConvertEscapedNewlinesToFText(const FText& InputText);
};
