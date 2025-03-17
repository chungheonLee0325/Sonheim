// Fill out your copyright notice in the Description page of Project Settings.


#include "PutDistance.h"

#include "Sonheim/Utilities/LogMacro.h"

void UPutDistance::InitState()
{
}

void UPutDistance::CheckIsValid()
{
}

void UPutDistance::Enter()
{
	FLog::Log("UPutDistance");

}

void UPutDistance::Execute(float dt)
{
	// 일정 거리 멀어지고
	ChangeState(m_NextState);

}

void UPutDistance::Exit()
{
}
