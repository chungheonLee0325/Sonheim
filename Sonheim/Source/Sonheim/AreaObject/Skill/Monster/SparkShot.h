// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "SparkShot.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USparkShot : public UBaseSkill
{
	GENERATED_BODY()

	
public:
	USparkShot();
	
	virtual void Server_OnCastStart(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void Server_OnCastTick(float DeltaTime) override;
	virtual void Server_OnCastFire() override;

	void FireSparkShot();

	float DelayTime = 2.0f;
	float CurrentTime = 0.0f;
	int32 AttackCount{9};
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ALightningBall> LightingBallFactory;

	bool IsFired = false;

	
};
