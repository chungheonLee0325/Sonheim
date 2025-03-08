// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "SelectSkill.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API USelectSkill : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void Enter() override;
	virtual void Execute(float DeltaTime) override;
	virtual void Exit() override;

	void SetSkillRoulette(class UBaseSkillRoulette* SkillRoulette) { m_SkillRoulette = SkillRoulette; }
private:
	UPROPERTY()
	class UBaseSkillRoulette* m_SkillRoulette = nullptr;
	UPROPERTY()
	UBaseSkill* Skill = nullptr;
};
