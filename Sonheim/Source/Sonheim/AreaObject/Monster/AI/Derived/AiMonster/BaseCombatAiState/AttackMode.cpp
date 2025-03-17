// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackMode.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

void UAttackMode::InitState()
{
}

void UAttackMode::CheckIsValid()
{
}

void UAttackMode::Enter()
{
	FLog::Log("UAttackMode");
}

void UAttackMode::Execute(float dt)
{
	// 너무 가까우면
	const float Dist{static_cast<float>(FVector::Distance(m_Owner->GetActorLocation(), m_Owner->GetAggroTarget()->GetActorLocation()))};
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
	ChangeState(m_FailState);
	//ChangeState(m_SuccessState);
}

void UAttackMode::Exit()
{
}
