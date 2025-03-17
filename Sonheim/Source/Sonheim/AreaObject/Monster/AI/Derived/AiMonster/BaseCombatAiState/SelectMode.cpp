// Fill out your copyright notice in the Description page of Project Settings.


#include "SelectMode.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

void USelectMode::InitState()
{}

void USelectMode::CheckIsValid()
{}

void USelectMode::Enter()
{
	FLog::Log("USelectMode");
}

void USelectMode::Execute(float dt)
{
	float Dist{static_cast<float>(FVector::Distance(m_Owner->GetActorLocation(), m_Owner->GetAggroTarget()->GetActorLocation()))};
	if (Dist > AttackAggroRange)
	{
		// PatrolMode
		ChangeState(m_FailState);
		return;
	}

	// AttackMode
	ChangeState(m_NextState);
}

void USelectMode::Exit()
{}
