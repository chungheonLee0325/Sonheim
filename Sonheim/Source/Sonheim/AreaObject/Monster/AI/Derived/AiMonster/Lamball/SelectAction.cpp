// Fill out your copyright notice in the Description page of Project Settings.


#include "SelectAction.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/Utilities/LogMacro.h"

void USelectAction::InitState()
{
}

void USelectAction::CheckIsValid()
{
}

void USelectAction::Enter()
{
	FLog::Log("USelectAction::Enter");
	
	//m_Owner->Jump();
	// ToDo : WorkCount bForced일때 강제 초기화
	// if (++WorkCount == MaxWorkCount)
	// {
	// 	return;
	// }
	m_Owner->Surprise();
}

void USelectAction::Execute(float dt)
{
	// if (++WorkCount == MaxWorkCount)
	// {
	// 	ChangeState(m_FailState);
	// }
	
	FlowTime += dt;

	if (FlowTime > ReadyTime)
	{
		m_Owner->CalmDown();
		
		ChangeState(m_NextState);
		return;
	}
	
}

void USelectAction::Exit()
{
}

void USelectAction::OnSkillCompleted()
{
	
}

void USelectAction::OnSkillCanceled()
{
	
}
