// Fill out your copyright notice in the Description page of Project Settings.


#include "SelectSkill.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Monster/BaseSkillRoulette.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"

void USelectSkill::InitState()
{
}

void USelectSkill::CheckIsValid()
{
	if (m_NextState == EAiStateType::None) LOG_PRINT(TEXT("Please Set m_NextState"));
	if (m_SkillRoulette == nullptr) LOG_PRINT(TEXT("Please Set m_SkillRoulette"));
}

void USelectSkill::Enter()
{
	//float distance = m_Owner->GetDistToTarget();
	//FVector dir = m_Owner->GetDirToTarget();
	//dir.Normalize();
	//float forwardDot = FVector::DotProduct(dir,m_Owner->GetActorForwardVector());
	//
	//m_NextState = EAiStateType::Chase;
	//
	//// 플레이어가 몬스터 뒤에 있으면 백어택
	//if (forwardDot > -1.0f && forwardDot < -0.75f) 
	//{
	//	//LOG_SCREEN("내 뒤");
	//}
	int id = m_SkillRoulette->GetRandomSkillID();
	if (id != 0)
	{
		m_Owner->NextSkill = m_Owner->GetSkillByID(id);
	}
}

void USelectSkill::Execute(float DeltaTime)
{
	ChangeState(m_NextState);
	return;
}

void USelectSkill::Exit()
{
}