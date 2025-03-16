// Fill out your copyright notice in the Description page of Project Settings.


#include "WaitState.h"

#include "Sonheim/AreaObject/Monster/BaseMonster.h"

void UWaitState::InitState()
{
}

void UWaitState::CheckIsValid()
{
}

void UWaitState::Enter()
{
	FLog::Log("UWaitState::Enter");
}

void UWaitState::Execute(float dt)
{
	if (m_Owner == nullptr || !m_Owner->GetResourceTarget())
	{
		//FLog::Log("UWaitState::return");
		return;
	}

	//FLog::Log("UWaitState::Execute");
	ChangeState(m_NextState);
}

void UWaitState::Exit()
{
}
