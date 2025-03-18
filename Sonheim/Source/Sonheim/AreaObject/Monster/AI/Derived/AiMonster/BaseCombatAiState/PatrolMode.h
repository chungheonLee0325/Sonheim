// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "PatrolMode.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UPatrolMode : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;

	void Patrol();
	
	float PatrolTime{3.f};
	float FlowTime{};
};
