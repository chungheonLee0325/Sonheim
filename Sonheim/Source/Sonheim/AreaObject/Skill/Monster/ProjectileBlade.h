// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "ProjectileBlade.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UProjectileBlade : public UBaseSkill
{
	GENERATED_BODY()

public:
	UProjectileBlade();
	
	virtual void OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void OnCastTick(float DeltaTime) override;

	virtual void OnCastFire() override;
	
	float DelayTime = 2.0f;
	float CurrentTime = 0.0f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABladeWind> BladeWindFactory;

	UPROPERTY(EditAnywhere)
	class ABladeWind* BladeWind;
	
};
