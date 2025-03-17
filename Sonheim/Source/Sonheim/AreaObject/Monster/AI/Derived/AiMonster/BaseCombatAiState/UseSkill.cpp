// Fill out your copyright notice in the Description page of Project Settings.


#include "UseSkill.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Monster/BaseSkillRoulette.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/Utilities/LogMacro.h"

void UUseSkill::InitState()
{
}

void UUseSkill::CheckIsValid()
{
}

void UUseSkill::Enter()
{
	FLog::Log("UUseSkill");
	m_Owner->LookAtLocation(m_Owner->GetAggroTarget()->GetActorLocation(),EPMRotationMode::Duration,0.1f);

	int32 ID{SkillRoulette->GetRandomSkillID()};
	FLog::Log("UUseSkill::Enter", ID );
	if (ID != 0)
	{
		m_Owner->NextSkill = m_Owner->GetSkillByID(ID);
	}
	
	if (m_Owner->CanCastSkill(m_Owner->NextSkill, m_Owner->GetAggroTarget()))
	{
		m_Owner->CastSkill(m_Owner->NextSkill, m_Owner->GetAggroTarget());
		m_Owner->RemoveSkillEntryByID(m_Owner->NextSkill->GetSkillData()->SkillID);
	}
}

void UUseSkill::Execute(float dt)
{
	// 스킬 사용 끝나면 selection
	if (!m_Owner->NextSkill->bIsSkillActive)
	{
		ChangeState(m_NextState);
	}
}

void UUseSkill::Exit()
{
}
