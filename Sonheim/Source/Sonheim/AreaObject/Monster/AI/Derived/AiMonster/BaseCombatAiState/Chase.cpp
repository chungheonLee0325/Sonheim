// Fill out your copyright notice in the Description page of Project Settings.


#include "Chase.h"

#include "Sonheim/Utilities/LogMacro.h"

void UChase::InitState()
{
}

void UChase::CheckIsValid()
{
}

void UChase::Enter()
{
	FLog::Log("UChase");

}

void UChase::Execute(float dt)
{
	// 적당한 거리면
	// {
	// 	ChangeState(m_SuccessState);
	// 	return;
	// }
	
	ChangeState(m_NextState);
}

void UChase::Exit()
{
}
