// Fill out your copyright notice in the Description page of Project Settings.


#include "UsePartnerSkill.h"

#include "Evaluation/MovieSceneEvaluationState.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"

void UUsePartnerSkill::InitState()
{}

void UUsePartnerSkill::CheckIsValid()
{}

void UUsePartnerSkill::Enter()
{
	FLog::Log("UUsePartnerSkill::Enter");
	
	// ToDo : ID 가져오는 방식 수정 필요, [ 테스트용 ]
	m_Owner->NextSkill = m_Owner->GetSkillByID(m_PartnerSkillID);

	// ToDo : Target에 m_Owner 넣음
	if (m_Owner->CanCastSkill(m_Owner->NextSkill, m_Owner))
	{
		if (!m_Owner->CastSkill(m_Owner->NextSkill, m_Owner))
		{
			return;
		}

		//m_Owner->RemoveSkillEntryByID(m_Owner->NextSkill->GetSkillData()->SkillID);
	}
	
}

void UUsePartnerSkill::Execute(float dt)
{
	if (!m_Owner->NextSkill)
	{
		FLog::Log("No SKill Found");
		return;
	}

	if (!m_Owner->bActivateSkill)
	{
		//m_Owner->RemoveSkillEntryByID(m_Owner->NextSkill->GetSkillData()->SkillID);

		ChangeState(m_NextState);
		return;
	}
	
}

void UUsePartnerSkill::Exit()
{}
