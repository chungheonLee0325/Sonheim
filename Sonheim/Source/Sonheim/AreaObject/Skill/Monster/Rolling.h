// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AITypes.h"
#include "Navigation/PathFollowingComponent.h"
#include "Sonheim/AreaObject/Skill/Common/MeleeAttack.h"
#include "Rolling.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API URolling : public UMeleeAttack
{
	GENERATED_BODY()

public:
	URolling();

	virtual bool Activate(class AAreaObject* Caster, AAreaObject* Target) override;
	virtual void Tick(float DeltaTime) override;

	virtual bool Fire() override;

	void StartRoll();
	void MoveCompleted(FAIRequestID FaiRequestID, const FPathFollowingResult& PathFollowingResult);
	void DizzyEnd();

public:
	UPROPERTY()
	class ALamBall* CastPal;

	FTimerHandle DizzyWaitTimer;
};
