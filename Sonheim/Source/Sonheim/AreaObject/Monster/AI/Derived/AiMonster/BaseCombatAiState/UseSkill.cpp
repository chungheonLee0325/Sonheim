// Fill out your copyright notice in the Description page of Project Settings.


#include "UseSkill.h"

#include "Sonheim/Utilities/LogMacro.h"

void UUseSkill::InitState()
{
}

void UUseSkill::CheckIsValid()
{
}

void UUseSkill::Enter()
{
	FLog::Log("UUseSkill");

}

void UUseSkill::Execute(float dt)
{
	// 스킬 사용 후
	ChangeState(m_NextState);
}

void UUseSkill::Exit()
{
}
