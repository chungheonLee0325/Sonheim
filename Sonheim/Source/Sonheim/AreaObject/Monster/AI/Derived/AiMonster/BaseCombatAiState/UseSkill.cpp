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
	m_CanAttack = true;
	
	m_Owner->LookAtLocation(m_Owner->GetAggroTarget()->GetActorLocation(),EPMRotationMode::Duration,0.1f);

	int32 ID{SkillRoulette->GetRandomSkillID()};
	if (ID != 0)
	{
		m_Owner->NextSkill = m_Owner->GetSkillByID(ID);
	}
	
	if (m_Owner->CanCastSkill(m_Owner->NextSkill, m_Owner->GetAggroTarget()))
	{
		m_Owner->NextSkill->OnSkillComplete.BindUObject(this, &UUseSkill::OnSkillCompleted);

		if (!m_Owner->CastSkill(m_Owner->NextSkill, m_Owner->GetAggroTarget()))
		{
			m_CanAttack = false;
			return;
		}

		m_Owner->RemoveSkillEntryByID(ID);
	}
	else
	{
		m_CanAttack = false;
	}
}

void UUseSkill::Execute(float dt)
{
	if (!m_CanAttack)
	{
		ChangeState(m_NextState);
	}
}

void UUseSkill::Exit()
{
}

void UUseSkill::OnSkillCompleted()
{
	// 스킬 사용 끝나면 selection
	FLog::Log("SKillComplete");

	ChangeState(m_NextState);
}
