// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "UseSkill.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UUseSkill : public UBaseAiState
{
	GENERATED_BODY()

public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void ServerEnter() override;
	virtual void ServerExecute(float dt) override;
	virtual void ServerExit() override;
	
	UFUNCTION()
	void OnSkillCompleted();
	
	float AttackTime{};
private:
	bool m_CanAttack = true;

};
