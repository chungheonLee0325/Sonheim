// Fill out your copyright notice in the Description page of Project Settings.


#include "AttackMode.h"

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
	ChangeState(m_NextState);

	// 적당한 거리면
	ChangeState(m_SuccessState);

	// 멀면
	ChangeState(m_FailState);

}

void UAttackMode::Exit()
{
}
