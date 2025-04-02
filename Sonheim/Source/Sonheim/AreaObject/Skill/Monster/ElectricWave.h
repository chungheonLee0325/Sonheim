// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "ElectricWave.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UElectricWave : public UBaseSkill
{
	GENERATED_BODY()

public:
	UElectricWave();
	
	virtual void OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void OnCastTick(float DeltaTime) override;

	virtual void OnCastFire() override;

	void ShockWave();

	float Range{1500.f};
	int32 AttackCount{3};

	float DelayTime = 2.0f;
	float CurrentTime = 0.0f;
};
