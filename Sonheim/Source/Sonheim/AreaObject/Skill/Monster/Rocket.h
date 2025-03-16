// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Rocket.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API URocket : public UBaseSkill
{
	GENERATED_BODY()

public:
	URocket();
	
	virtual void OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void OnCastTick(float DeltaTime) override;

	virtual void OnCastFire() override;

	void Launch();
};
