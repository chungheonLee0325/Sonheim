// Fill out your copyright notice in the Description page of Project Settings.


#include "SelectMode.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

void USelectMode::InitState()
{}

void USelectMode::CheckIsValid()
{}

void USelectMode::ServerEnter()
{
	FLog::Log("USelectMode");
	FlowTime = 0.f;
	m_Owner->ChangeFace(EFaceType::Sad);
}

void USelectMode::ServerExecute(float dt)
{
	FLog::Log("SelectTick");

	FlowTime += dt;
	if (FlowTime >= ChooseModeTime)
	{
		float Dist{
			static_cast<float>(FVector::Distance(m_Owner->GetActorLocation(),
			                                     m_Owner->GetAggroTarget()->GetActorLocation()))
		};
		if (Dist > AttackAggroRange)
		{
			// PatrolMode
			ChangeState(m_FailState);
			return;
		}

		// AttackMode
		ChangeState(m_NextState);
	}
}

void USelectMode::ServerExit()
{}
