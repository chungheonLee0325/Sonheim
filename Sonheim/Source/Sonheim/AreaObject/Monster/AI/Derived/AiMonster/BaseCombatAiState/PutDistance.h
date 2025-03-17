// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "PutDistance.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UPutDistance : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;

	void MoveToAttack();
	void MoveCompleted(struct FAIRequestID RequestID, const struct FPathFollowingResult& Result);
	bool CheckAttackEnable(const FVector& StartLoc, const FVector& EndLoc);
};
