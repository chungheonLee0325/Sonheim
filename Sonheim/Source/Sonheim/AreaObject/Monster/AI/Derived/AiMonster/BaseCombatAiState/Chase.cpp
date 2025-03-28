// Fill out your copyright notice in the Description page of Project Settings.


#include "Chase.h"

#include "AIController.h"
#include "NavigationSystem.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"
#include "Sonheim/Utilities/LogMacro.h"

void UChase::InitState()
{}

void UChase::CheckIsValid()
{}

void UChase::ServerEnter()
{
	FLog::Log("UChase");
	m_Owner->ChangeFace(EFaceType::Sad);
	
	FlowTime = 0.f;
	AttackRange = m_Owner->NextSkill->GetSkillData()->CastRange;
	PatrolRange = m_Owner->NextSkill->GetSkillData()->CastRange + 1'500;
}

void UChase::ServerExecute(float dt)
{
	if (m_Owner->IsCalled || m_Owner->bIsCanCalled)
	{
		ChangeState(EAiStateType::SelectMode);
		return;
	}
	
	if (!m_Owner->GetAggroTarget())
	{
		ChangeState(EAiStateType::SelectMode);
		return;
	}
	
	float Dist{
		static_cast<float>(FVector::Distance(m_Owner->GetActorLocation(),
		                                     m_Owner->GetAggroTarget()->GetActorLocation()))
	};
	FLog::Log("Dist", Dist);
	// 거리가 멀면 Patrol
	if (Dist > PatrolRange)
	{
		m_Owner->SetAggroTarget(nullptr);
		ChangeState(m_NextState);
		return;
	}
	// 거리가 가까우면 UseSkill
	if (Dist < AttackRange)
	{
		m_Owner->AIController->StopMovement();
		ChangeState(m_SuccessState);
		return;
	}
	
	Chase(dt);
}

void UChase::ServerExit()
{}

void UChase::Chase(float dt)
{
	FlowTime += dt;
	if (FlowTime > JumpTime)
	{
		FlowTime = 0.f;
		DoJump();
	}
	
	// Target 쫓기
	UNavigationSystemV1* NavSystem{UNavigationSystemV1::GetCurrent(GetWorld())};
	if (!NavSystem)
	{
		return;
	}

	FVector End{m_Owner->GetAggroTarget()->GetActorLocation()};
	
	m_Owner->AIController->MoveToLocation(End);
}

void UChase::DoJump()
{
	m_Owner->Jump();
}
