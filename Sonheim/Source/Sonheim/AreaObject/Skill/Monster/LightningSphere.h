// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "LightningSphere.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API ULightningSphere : public UBaseSkill
{
	GENERATED_BODY()

public:
	ULightningSphere();
	
	virtual void Server_OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void Server_OnCastTick(float DeltaTime) override;

	virtual void Server_OnCastFire() override;

	void FireElectricBall();

	float DelayTime = 2.0f;
	float CurrentTime = 0.0f;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AElectricBall> ElectricBallFactory;

	bool IsFired = false;

};
