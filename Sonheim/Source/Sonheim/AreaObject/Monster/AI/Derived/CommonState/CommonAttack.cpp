// Fill out your copyright notice in the Description page of Project Settings.


#include "CommonAttack.h"
#include "Sonheim/AreaObject/Monster/BaseMonster.h"
#include "Sonheim/AreaObject/Skill/Base/BaseSkill.h"

void UCommonAttack::InitState()
{
}

void UCommonAttack::CheckIsValid()
{
	if (m_SuccessState == EAiStateType::None) LOG_PRINT(TEXT("Please Set m_SuccessState"));
	if (m_FailState == EAiStateType::None) LOG_PRINT(TEXT("Please Set m_FailState"));
}

void UCommonAttack::Enter()
{
	bHasFailed = false;

	// LOG_PRINT(TEXT("넥스트스킬: %s"),*m_Owner->NextSkill->GetName());
	UBaseSkill* skill = m_Owner->NextSkill;

	if (m_Owner->CanCastSkill(skill, m_Owner->GetAggroTarget()))
	{
		skill->OnSkillComplete.BindUObject(this, &UCommonAttack::OnSkillCompleted);
		skill->OnSkillCancel.BindUObject(this, &UCommonAttack::OnSkillCanceled);
		m_Owner->CastSkill(skill, m_Owner->GetAggroTarget());
		m_Owner->RemoveSkillEntryByID(skill->GetSkillData()->SkillID);
	}
	else
	{
		LOG_PRINT(TEXT("스킬 실행 실패"));
		bHasFailed = true;
	}
}

void UCommonAttack::Execute(float dt)
{
	if (bHasFailed)
	{
		ChangeState(m_FailState);
	}
}

void UCommonAttack::Exit()
{
}

void UCommonAttack::OnSkillCompleted()
{
	ChangeState(m_SuccessState);
}

void UCommonAttack::OnSkillCanceled()
{
	//ChangeState(m_FailState);
}
