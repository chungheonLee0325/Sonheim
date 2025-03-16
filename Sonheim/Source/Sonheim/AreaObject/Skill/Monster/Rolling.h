// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Rolling.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API URolling : public UBaseSkill
{
	GENERATED_BODY()
	
public:
	URolling();
	
	virtual void OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void OnCastTick(float DeltaTime) override;

	virtual void OnCastFire() override;

	void StartRoll();

public:
	FTimerHandle RollWaitTimer;
};
