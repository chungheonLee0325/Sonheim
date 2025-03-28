// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackMode.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Monster/BaseSkillRoulette.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/Utilities/LogMacro.h"

void UAttackMode::InitState()
{}

void UAttackMode::CheckIsValid()
{}

void UAttackMode::ServerEnter()
{
	FLog::Log("UAttackMode");
	FlowTime = 0.f;
	m_Owner->ChangeFace(EFaceType::Exciting);


	// ToDo : 파이호 전용 스킬 때문에 .. 나중에 수정 필요
	while (true)
	{
		ID = SkillRoulette->GetRandomSkillID();
		if (ID != 1060)
		{
			break;
		}
	}

	if (ID != 0)
	{
		m_Owner->NextSkill = m_Owner->GetSkillByID(ID);
		AttackMaxRange = m_Owner->NextSkill->GetSkillData()->CastRange;
	}
}

void UAttackMode::ServerExecute(float dt)
{
	if (ID == 0)
	{
		FLog::Log("Has No Skill");
		return;
	}

	if (!m_Owner->GetAggroTarget())
	{
		ChangeState(EAiStateType::SelectMode);
		return;
	}
	
	FlowTime += dt;
	if (FlowTime >= ChooseModeTime)
	{
		// 너무 가까우면
		const float Dist{
			static_cast<float>(FVector::Distance(m_Owner->GetActorLocation(),
			                                     m_Owner->GetAggroTarget()->GetActorLocation()))
		};
		if (Dist < AttackMinRange)
		{
			// PutDistance
			ChangeState(m_NextState);
			return;
		}

		// 적당한 거리면 UseSkill
		if (Dist < AttackMaxRange)
		{
			ChangeState(m_SuccessState);
			return;
		}

		// 멀면 Chase
		// Dist > AttackMaxRange
		ChangeState(m_FailState);
	}
}

void UAttackMode::ServerExit()
{}
