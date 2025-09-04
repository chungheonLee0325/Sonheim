// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "ProjectileAA.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UProjectileAA : public UBaseSkill
{
	GENERATED_BODY()

public:
	UProjectileAA();

	virtual void Activate(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void Tick(float DeltaTime) override;

	virtual void Fire() override;
	virtual void Complete() override;

	void FireSandBlast();
	UFUNCTION(Server, Reliable)
	void SeverRPC_FireSandBlast();

	float DelayTime = 2.0f;
	float CurrentTime = 0.0f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<class ASandBlast> SandBlastFactory;
};
