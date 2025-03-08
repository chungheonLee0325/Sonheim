// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Base/BaseSkill.h"
#include "RecoverHPSkill.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API URecoverHPSkill : public UBaseSkill
{
	GENERATED_BODY()

	virtual void OnCastFire() override;
};
