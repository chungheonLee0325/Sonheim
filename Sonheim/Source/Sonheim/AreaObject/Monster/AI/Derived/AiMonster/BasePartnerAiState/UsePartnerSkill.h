// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Sonheim/AreaObject/Monster/AI/Base/BaseAiState.h"
#include "UsePartnerSkill.generated.h"

/**
 * 
 */
UCLASS()
class SONHEIM_API UUsePartnerSkill : public UBaseAiState
{
	GENERATED_BODY()
	
public:
	virtual void InitState() override;
	virtual void CheckIsValid() override;
	virtual void ServerEnter() override;
	virtual void ServerExecute(float dt) override;
	virtual void ServerExit() override;

	void SetPartnerSkillID(int PartnerSkillID)
	{
		this->m_PartnerSkillID = PartnerSkillID;
	}

private:
	int m_PartnerSkillID = 0;
};
