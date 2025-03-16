// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "GatherToPlayer.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UGatherToPlayer : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;
	
	UFUNCTION()
	void OnSkillCompleted();

	UFUNCTION()
	void OnSkillCanceled();

	bool bFlow = false;
	bool bTarget = false;
	bool bArrive = false;
	AActor* Target;
	float FlowTime{};
	float ReadyTime{1.5f};

	const int MaxWorkCount = 5;
	int WorkCount = 0;
};
