// Fill out your copyright notice in the Description page of Project Settings.


#include "SelectMode.h"

#include "Sonheim/Utilities/LogMacro.h"

void USelectMode::InitState()
{
}

void USelectMode::CheckIsValid()
{
}

void USelectMode::Enter()
{
	FLog::Log("USelectMode");

}

void USelectMode::Execute(float dt)
{
	// 엄청 멀리 있으면 patrol
	// {
	// 	ChangeState(m_FailState);
	// 	return;
	// }
	
	ChangeState(m_NextState);
}

void USelectMode::Exit()
{
}
