// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Flamethrower.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UFlamethrower : public UBaseSkill
{
	GENERATED_BODY()

	
public:
	UFlamethrower();
	
	virtual void OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void OnCastTick(float DeltaTime) override;

	virtual void OnCastFire() override;

	void FireFlame();

public:
	FTimerHandle FireTimer;
	float SpreadPitch{5.f};
	float SpreadYaw{10.f};
	float Range{500.f};
};
