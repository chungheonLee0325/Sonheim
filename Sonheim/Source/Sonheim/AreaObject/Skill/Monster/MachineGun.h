// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "MachineGun.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UMachineGun : public UBaseSkill
{
	GENERATED_BODY()

public:
	UMachineGun();
	
	virtual void OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void OnCastTick(float DeltaTime) override;

	virtual void OnCastFire() override;

	void FireGun();

public:
	FTimerHandle FireTimer;
	float Spread{1.f};
	float Range{10000.f};
};
