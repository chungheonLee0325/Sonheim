// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "AttackMode.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UAttackMode : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float dt) override;
	virtual void Exit() override;
	
	void SetSkillRoulette(class UBaseSkillRoulette* BaseSkillRoulette) { SkillRoulette = BaseSkillRoulette; }

	UPROPERTY()
	class UBaseSkillRoulette* SkillRoulette;
	
	float AttackMinRange{500.f};
	float AttackMaxRange{};

	float ChooseModeTime{1.5f};
	float FlowTime{};
	
	int32 ID{};

};
